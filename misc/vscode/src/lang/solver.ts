import * as ast from "./ast";
import { CompletionPoint, TypeChildCompletionPoint, TypeParentCompletionPoint } from "./completion";
import { MError } from "./error";
import { MLocation } from "./location";
import { MScope } from "./scope";
import { MSignatureHelper } from "./sighelp";
import { MSymbol, MSymbolUsage } from "./symbol";
import * as type from "./type";
import { MType } from "./type";



export class Solver {
  readonly scope: MScope;
  readonly errors: MError[];
  readonly symbolUsages: MSymbolUsage[];
  readonly completionPoints: CompletionPoint[];
  readonly signatureHelpers: MSignatureHelper[];
  private readonly typeVisitor: TypeVisitor;
  readonly statementVisitor: StatementVisitor;
  basesMap: Map<string, type.Class[]> | null = null

  constructor(
      scope: MScope,
      errors: MError[],
      symbolUsages: MSymbolUsage[],
      completionPoints: CompletionPoint[],
      signatureHelpers: MSignatureHelper[]) {
    this.scope = scope;
    this.errors = errors;
    this.symbolUsages = symbolUsages;
    this.completionPoints = completionPoints;
    this.signatureHelpers = signatureHelpers;
    this.typeVisitor = new TypeVisitor(this);
    this.statementVisitor = new StatementVisitor(this);
  }

  withScope(scope: MScope): Solver {
    return new Solver(
      scope,
      this.errors,
      this.symbolUsages,
      this.completionPoints,
      this.signatureHelpers);
  }

  solveFile(file: ast.File) {
    // PREPARE CLASS DECLARATIONS
    for (const cdecl of file.statements) {
      if (cdecl instanceof ast.Class) {
        const classSymbol = this.recordSymbolDefinition(cdecl.identifier, true);
        classSymbol.typeType = type.Instance.of(classSymbol);
        classSymbol.valueType = type.Class.of(classSymbol);
      }
    }

    // PREPARE METHOD DECLARATIONS
    this.basesMap = new Map();
    for (const cdecl of file.statements) {
      if (cdecl instanceof ast.Class) {
        const classSymbol = this.scope.get(cdecl.identifier.name);
        if (!classSymbol) {
          throw new Error(`Assertion Error: class symbol not found (${cdecl.identifier.name})`);
        }
        const baseValueTypes: type.Class[] = [];
        for (const be of cdecl.bases) {
          const baseValueType = this.solveExpression(be);
          if (baseValueType instanceof type.Class) {
            for (const [key, value] of baseValueType.symbol.members) {
              if (!classSymbol.members.has(key)) {
                classSymbol.members.set(key, value);
              }
            }
            baseValueTypes.push(baseValueType);
          } else {
            this.errors.push(new MError(
              be.location,
              `Expected Class but got ${baseValueType}`));
          }
        }
        this.basesMap.set(cdecl.identifier.name, baseValueTypes);
        classSymbol.staticMembers = new Map();
        for (const staticMethod of cdecl.staticMethods) {
          const staticMethodSymbol = this.recordSymbolDefinition(
            staticMethod.identifier, false, true);
          classSymbol.staticMembers.set(staticMethodSymbol.name, staticMethodSymbol);
          this.statementVisitor.computeFunctionSignature(
            staticMethod, staticMethodSymbol);
        }
        for (const field of cdecl.fields) {
          const fieldSymbol = this.recordSymbolDefinition(
            field.identifier, false, field.final);
          classSymbol.members.set(fieldSymbol.name, fieldSymbol);
          const fieldType = this.solveType(field.typeExpression);
          fieldSymbol.valueType = fieldType;
        }
        for (const method of cdecl.methods) {
          const methodSymbol = this.recordSymbolDefinition(method.identifier, false, true);
          classSymbol.members.set(methodSymbol.name, methodSymbol);
          this.statementVisitor.computeFunctionSignature(method, methodSymbol);
        }
      }
    }

    // PREPARE FUNCTION DECLARATIONS
    for (const fdecl of file.statements) {
      if (fdecl instanceof ast.Function) {
        const functionSymbol = this.recordSymbolDefinition(fdecl.identifier, true, true);
        this.statementVisitor.computeFunctionSignature(fdecl, functionSymbol);
      }
    }

    // MAIN SOLVE LOOP
    for (const statement of file.statements) {
      this.solveStatement(statement);
    }
  }

  solveType(typeExpression: ast.TypeExpression): MType {
    return this.typeVisitor.solveTypeExpression(typeExpression);
  }

  solveExpression(expression: ast.Expression, typeHint: MType = type.Any): MType {
    return expression.accept(new ExpressionVisitor(this, typeHint));
  }

  solveStatement(statement: ast.Statement) {
    statement.accept(this.statementVisitor);
  }

  recordSymbolDefinition(
      identifier: ast.Identifier,
      addToScope: boolean,
      final: boolean = true,
      isImport: boolean = false): MSymbol {
    const symbol = new MSymbol(identifier.name, identifier.location, final, null, isImport);
    if (addToScope) {
      const previous = this.scope.map.get(identifier.name);
      if (previous) {
        this.errors.push(new MError(
          identifier.location,
          `'${previous.name}' is already defined in this scope`));
      }
      this.scope.set(symbol);
    }
    if (symbol.definition) {
      this.symbolUsages.push(symbol.definition);
    }
    return symbol;
  }

  recordSymbolUsage(
      identifier: ast.Identifier,
      symbol: MSymbol,
      bindings: Map<string, MType | null> | null = null) {
    const usage = new MSymbolUsage(identifier.location, symbol, bindings);
    this.symbolUsages.push(usage);
  }
}

class TypeVisitor {
  readonly solver: Solver;
  private readonly errors: MError[];
  private readonly symbolUsages: MSymbolUsage[];

  constructor(solver: Solver) {
    this.solver = solver;
    this.errors = solver.errors;
    this.symbolUsages = solver.symbolUsages;
  }

  private checkTypeArgc(te: ast.TypeExpression, argc: number) {
    if (te.args.length !== argc) {
      this.errors.push(new MError(
        te.location,
        `Expected ${argc} type arguments but got ${te.args.length}`));
    }
  }

  solveTypeExpression(te: ast.TypeExpression): MType {
    const scope = this.solver.scope;

    if (te.parentIdentifier) {
      this.solver.completionPoints.push(new TypeParentCompletionPoint(
        te.parentIdentifier.location, scope));

      // Qualified type name
      const parentIdentifier = te.parentIdentifier;
      const parentName = parentIdentifier.name;
      const memberIdentifier = te.baseIdentifier;
      const memberName = memberIdentifier.name;
      const parentSymbol = scope.get(parentName);
      if (!parentSymbol) {
        this.errors.push(new MError(
          parentIdentifier.location,
          `Type qualifier '${parentName}' not found`));
        return type.Any;
      }
      this.solver.recordSymbolUsage(parentIdentifier, parentSymbol);
      if (!(parentSymbol.valueType instanceof type.Module)) {
        this.errors.push(new MError(
          parentIdentifier.location,
          `'${parentName}' is not a module`));
        return type.Any;
      }
      this.solver.completionPoints.push(new TypeChildCompletionPoint(
        te.baseIdentifier.location, MScope.new(null, parentSymbol.members)));
      const memberSymbol = parentSymbol.members.get(memberName);
      if (!memberSymbol) {
        this.errors.push(new MError(
          memberIdentifier.location,
          `Type '${memberName}' not found in '${parentName}`));
        return type.Any;
      }
      const memberUsage = new MSymbolUsage(memberIdentifier.location, memberSymbol);
      this.symbolUsages.push(memberUsage);
      const symbolTypeType = memberSymbol.typeType;
      if (symbolTypeType === null) {
        this.errors.push(new MError(
          memberIdentifier.location,
          `${memberName} is not a type`));
        return type.Any;
      }
      return symbolTypeType;
    }
    this.solver.completionPoints.push(new TypeParentCompletionPoint(
      te.baseIdentifier.location, scope));
    const name = te.baseIdentifier.name;
    switch (name) {
      case 'StopIteration': this.checkTypeArgc(te, 0); return type.StopIteration;
      case 'nil': this.checkTypeArgc(te, 0); return type.Nil;
      case 'List':
        const listSymbol = scope.get('List');
        if (listSymbol) {
          const typeUsage = new MSymbolUsage(te.baseIdentifier.location, listSymbol);
          this.symbolUsages.push(typeUsage);
        }
        if (te.args.length === 0) {
          return type.UntypedList;
        }
        this.checkTypeArgc(te, 1);
        return type.List.of(this.solveTypeExpression(te.args[0]));
      case 'Tuple':
        const tupleSymbol = scope.get('Tuple');
        if (tupleSymbol) {
          const typeUsage = new MSymbolUsage(te.baseIdentifier.location, tupleSymbol);
          this.symbolUsages.push(typeUsage);
        }
        if (te.args.length === 0) {
          return type.UntypedTuple;
        }
        this.checkTypeArgc(te, 1);
        return type.Tuple.of(this.solveTypeExpression(te.args[0]));
      case 'Dict':
        const dictSymbol = scope.get('Dict');
        if (dictSymbol) {
          const typeUsage = new MSymbolUsage(te.baseIdentifier.location, dictSymbol);
          this.symbolUsages.push(typeUsage);
        }
        if (te.args.length === 0) {
          return type.UntypedDict;
        }
        this.checkTypeArgc(te, 2);
        return type.Dict.of(
          this.solveTypeExpression(te.args[0]),
          this.solveTypeExpression(te.args[1]));
      case 'FrozenDict':
        const frozenDictSymbol = scope.get('FrozenDict');
        if (frozenDictSymbol) {
          const typeUsage = new MSymbolUsage(te.baseIdentifier.location, frozenDictSymbol);
          this.symbolUsages.push(typeUsage);
        }
        if (te.args.length === 0) {
          return type.UntypedFrozenDict;
        }
        this.checkTypeArgc(te, 2);
        return type.FrozenDict.of(
          this.solveTypeExpression(te.args[0]),
          this.solveTypeExpression(te.args[1]));
      case 'Optional':
        const optionalSymbol = scope.get('Optional');
        if (optionalSymbol) {
          const typeUsage = new MSymbolUsage(te.baseIdentifier.location, optionalSymbol);
          this.symbolUsages.push(typeUsage);
        }
        this.checkTypeArgc(te, 1);
        return type.Optional.of(this.solveTypeExpression(te.args[0]));
      case 'Iteration':
        const iterationSymbol = scope.get('Iteration');
        if (iterationSymbol) {
          const typeUsage = new MSymbolUsage(te.baseIdentifier.location, iterationSymbol);
          this.symbolUsages.push(typeUsage);
        }
        this.checkTypeArgc(te, 1);
        return type.Iteration.of(this.solveTypeExpression(te.args[0]));
      case 'Iterable':
        const iterableSymbol = scope.get('Iterable');
        if (iterableSymbol) {
          const typeUsage = new MSymbolUsage(te.baseIdentifier.location, iterableSymbol);
          this.symbolUsages.push(typeUsage);
        }
        this.checkTypeArgc(te, 1);
        return type.Iterable.of(this.solveTypeExpression(te.args[0]));
      case 'Function':
        const functionSymbol = scope.get('Function');
        if (functionSymbol) {
          const typeUsage = new MSymbolUsage(te.baseIdentifier.location, functionSymbol);
          this.symbolUsages.push(typeUsage);
        }
        if (te.args.length === 0) {
          return type.UntypedFunction;
        }
        const parameters: MType[] = [];
        for (let i = 0; i + 1 < te.args.length; i++) {
          parameters.push(this.solveTypeExpression(te.args[i]));
        }
        const returnType = this.solveTypeExpression(te.args[te.args.length - 1]);
        return type.Function.of(parameters, 0, returnType);
    }
    if (te.args.length === 0) {
      // Simple type name
      const typeIdentifier = te.baseIdentifier;
      const typeName = typeIdentifier.name;
      const typeSymbol = scope.get(typeName);
      if (typeSymbol) {
        const typeUsage = new MSymbolUsage(typeIdentifier.location, typeSymbol);
        this.symbolUsages.push(typeUsage);
        const typeType = typeSymbol.typeType;
        if (typeType === null) {
          this.errors.push(new MError(
            typeIdentifier.location, `${typeName} is not a type`));
          return type.Any;
        }
        return typeType;
      } else {
        this.errors.push(new MError(
          typeIdentifier.location, `Type ${typeName} not found`));
      }
    }
    this.errors.push(new MError(
      te.location, `unrecognized type name ${te.baseIdentifier.name}`));
    return type.Any;
  }
}

export class TypeBinder {
  readonly bindings: Map<string, MType | null>
  readonly allowNewBindings: boolean
  constructor(bindings: Map<string, MType | null>, allowNewBindings: boolean) {
    this.bindings = bindings;
    this.allowNewBindings = allowNewBindings;
  }

  /** Try binding the parameter type to the actual argument type,
   * Return the updated parameter type with bindings resolved.
   */
  bind(parameterType: MType, actualType: MType): MType {
    if (parameterType instanceof type.TypeParameter) {
      const binding = this.bindings.get(parameterType.symbol.name);
      if (binding === undefined) {
        // TODO: weird that this parameter type was not found. Error on this.
        return type.Any;
      } else if (binding === null) {
        if (this.allowNewBindings) {
          this.bindings.set(parameterType.symbol.name, actualType);
          return actualType;
        } else {
          // New bindings are not allowed, but we found a variable with no
          // TODO: add error
          return type.Any;
        }
      }
      return binding;
    }
    if (parameterType === type.Any ||
        parameterType === type.Never ||
        parameterType instanceof type.BuiltinPrimitive ||
        parameterType instanceof type.Class ||
        parameterType instanceof type.Instance ||
        parameterType instanceof type.Module) {
      return parameterType;
    }
    if (parameterType instanceof type.Optional) {
      if (actualType instanceof type.Optional) {
        return type.Optional.of(this.bind(parameterType.itemType, actualType.itemType));
      }
      return type.Optional.of(this.bind(parameterType.itemType, actualType));
    }
    if (parameterType instanceof type.Iteration) {
      if (actualType instanceof type.Iteration) {
        return type.Iteration.of(this.bind(parameterType.itemType, actualType.itemType));
      }
      return type.Iteration.of(this.bind(parameterType.itemType, actualType));
    }
    if (parameterType instanceof type.Iterable) {
      const itemType = actualType.getForInItemType();
      if (itemType) {
        return type.Iterable.of(this.bind(parameterType.itemType, itemType));
      }
      return type.Iterable.of(this.bind(parameterType.itemType, type.Any));
    }
    if (parameterType instanceof type.List) {
      if (actualType instanceof type.List) {
        return type.List.of(this.bind(parameterType.itemType, actualType.itemType));
      }
      return type.List.of(this.bind(parameterType.itemType, type.Any));
    }
    if (parameterType instanceof type.Tuple) {
      if (actualType instanceof type.Tuple) {
        return type.Tuple.of(this.bind(parameterType.itemType, actualType.itemType));
      }
      return type.Tuple.of(this.bind(parameterType.itemType, type.Any));
    }
    if (parameterType instanceof type.Dict) {
      if (actualType instanceof type.Dict) {
        return type.Dict.of(
          this.bind(parameterType.keyType, actualType.keyType),
          this.bind(parameterType.valueType, actualType.valueType));
      }
      return type.Dict.of(
        this.bind(parameterType.keyType, type.Any),
        this.bind(parameterType.valueType, type.Any));
    }
    if (parameterType instanceof type.FrozenDict) {
      if (actualType instanceof type.FrozenDict) {
        return type.FrozenDict.of(
          this.bind(parameterType.keyType, actualType.keyType),
          this.bind(parameterType.valueType, actualType.valueType));
      }
      return type.FrozenDict.of(
        this.bind(parameterType.keyType, type.Any),
        this.bind(parameterType.valueType, type.Any));
    }
    if (parameterType instanceof type.Function) {
      if (actualType instanceof type.Function) {
        return type.Function.of(
          parameterType.parameters.map((p, i) => this.bind(
            p, i < actualType.parameters.length ? actualType.parameters[i] : type.Any)),
          parameterType.optionalCount,
          this.bind(parameterType.returnType, actualType.returnType));
      }
      return type.Function.of(
        parameterType.parameters.map(p => this.bind(p, type.Any)),
        parameterType.optionalCount,
        this.bind(parameterType.returnType, type.Any));
    }
    // TODO: Additional types
    return type.Any;
  }
}

class ExpressionVisitor extends ast.ExpressionVisitor<MType> {
  readonly solver: Solver;
  readonly typeHint: MType;
  private readonly scope: MScope;
  private readonly errors: MError[];

  constructor(solver: Solver, typeHint: MType) {
    super();
    this.solver = solver;
    this.typeHint = typeHint;
    this.scope = solver.scope;
    this.errors = solver.errors;
  }

  private solveExpression(e: ast.Expression, typeHint: MType = type.Any) {
    return e.accept(new ExpressionVisitor(this.solver, typeHint));
  }

  visitGetVariable(e: ast.GetVariable): type.MType {
    this.solver.completionPoints.push(new CompletionPoint(e.identifier.location, this.scope));
    const symbol = this.scope.get(e.identifier.name);
    if (!symbol) {
      this.errors.push(new MError(
        e.location,
        `Variable '${e.identifier.name}' not found`));
      return type.Any;
    }
    this.solver.recordSymbolUsage(e.identifier, symbol);
    return symbol.valueType || type.Any;
  }

  visitSetVariable(e: ast.SetVariable): type.MType {
    const symbol = this.scope.get(e.identifier.name);
    if (!symbol) {
      this.errors.push(new MError(
        e.location,
        `Variable '${e.identifier.name}' not found`));
      return type.Any;
    }
    this.solver.recordSymbolUsage(e.identifier, symbol);
    if (symbol.final) {
      this.errors.push(new MError(
        e.location,
        `Cannot assign to a final variable`));
    }
    const symbolType = symbol.valueType || type.Any;
    const rhsType = this.solveExpression(e.value, symbolType);
    if (!rhsType.isAssignableTo(symbolType)) {
      this.errors.push(new MError(
        e.location,
        `Cannot assign ${rhsType} to ${symbolType}`));
    }
    return symbolType;
  }

  visitNilLiteral(e: ast.NilLiteral): type.MType {
    return type.Nil;
  }

  visitBoolLiteral(e: ast.BoolLiteral): type.MType {
    return type.Bool;
  }

  visitNumberLiteral(e: ast.NumberLiteral): type.MType {
    return type.Number;
  }

  visitStringLiteral(e: ast.StringLiteral): type.MType {
    return type.String;
  }

  visitTypeAssertion(e: ast.TypeAssertion): type.MType {
    const assertType = this.solver.solveType(e.typeExpression);
    const innerType = this.solveExpression(e.expression, assertType);
    if (innerType.isAssignableTo(assertType)) {
      // The type assertion here might not be necessary, but in some cases, the hint from
      // this type assertion may have been necessary to get the proper result.
      // TODO: consider emitting a warning here for some cases when we can prove
      // that this type assertion is unnecessary.
    } else if (assertType.isAssignableTo(innerType)) {
      // This is more or less the main case we expect
    } else {
      this.errors.push(new MError(
        e.location,
        `Assertion from ${innerType} to ${assertType} can never succeed`));
    }
    return assertType;
  }

  visitListDisplay(e: ast.ListDisplay): type.MType {
    let itemType: MType = type.Never;
    if (this.typeHint instanceof type.List) {
      itemType = this.typeHint.itemType;
    }
    for (let item of e.items) {
      itemType = itemType.closestCommonType(this.solveExpression(item));
    }
    return type.List.of(itemType);
  }

  visitTupleDisplay(e: ast.TupleDisplay): type.MType {
    let itemType: MType = type.Never;
    if (this.typeHint instanceof type.Tuple) {
      itemType = this.typeHint.itemType;
    }
    for (let item of e.items) {
      itemType = itemType.closestCommonType(this.solveExpression(item));
    }
    return type.Tuple.of(itemType);
  }

  visitDictDisplay(e: ast.DictDisplay): type.MType {
    let keyType: MType = type.Never;
    let valueType: MType = type.Never;
    if (this.typeHint instanceof type.Dict) {
      keyType = this.typeHint.keyType;
      valueType = this.typeHint.valueType;
    }
    for (let [key, value] of e.pairs) {
      keyType = keyType.closestCommonType(this.solveExpression(key));
      valueType = valueType.closestCommonType(this.solveExpression(value));
    }
    return type.Dict.of(keyType, valueType);
  }

  visitFrozenDictDisplay(e: ast.FrozenDictDisplay): type.MType {
    let keyType: MType = type.Never;
    let valueType: MType = type.Never;
    if (this.typeHint instanceof type.FrozenDict) {
      keyType = this.typeHint.keyType;
      valueType = this.typeHint.valueType;
    }
    if (e.pairs.every(pair => pair[0] instanceof ast.StringLiteral)) {
      const symbols: MSymbol[] = [];
      for (const [key, value] of e.pairs) {
        const currentKeyType = this.solveExpression(key);
        if (!(key instanceof ast.StringLiteral)) {
          throw new Error(`Assertion Failed: not all keys are string literals`);
        }
        const currentValueType = this.solveExpression(value);
        const symbol = this.solver.recordSymbolDefinition(
          new ast.Identifier(key.location, key.value), false, true);
        symbol.valueType = currentValueType;
        symbols.push(symbol);
        keyType = keyType.closestCommonType(currentKeyType);
        valueType = valueType.closestCommonType(currentValueType);
      }
      return type.KeyedFrozenDict.of(symbols, valueType);
    }
    for (let [key, value] of e.pairs) {
      keyType = keyType.closestCommonType(this.solveExpression(key));
      valueType = valueType.closestCommonType(this.solveExpression(value));
    }
    return type.FrozenDict.of(keyType, valueType);
  }

  private checkArgTypes(
      callLocation: MLocation,
      functionName: string | null,
      functionDocumentation: string | null,
      args: ast.Expression[],
      argLocations: MLocation[] | null,
      parameterTypes: MType[],
      parameterNames: string[] | null,
      optionalCount: number,
      returnType: MType | null,
      bindings: Map<string, MType | null>) {
    if (argLocations) {
      for (let i = 0; i < argLocations.length; i++) {
        this.solver.signatureHelpers.push(new MSignatureHelper(
          argLocations[i],
          functionName,
          functionDocumentation,
          parameterTypes,
          parameterNames,
          i,
          returnType));
      }
    }
    if (args.length < parameterTypes.length - optionalCount ||
        args.length > parameterTypes.length) {
      this.errors.push(new MError(
        callLocation,
        optionalCount === 0 ?
          `${parameterTypes.length} args expected but got ${args.length}` :
          `${parameterTypes.length - optionalCount} to ${parameterTypes.length} args ` +
          `expected but got ${args.length}`));
      return;
    }
    for (let i = 0; i < args.length; i++) {
      const binder = new TypeBinder(bindings, true);
      let parameterType = parameterTypes[i];
      const argType =
        this.solveExpression(args[i], bindings.size > 0 ? type.Any : parameterType);
      if (bindings.size > 0) {
        parameterType = binder.bind(parameterType, argType);
      }
      if (!argType.isAssignableTo(parameterType)) {
        this.errors.push(new MError(
          args[i].location,
          `Argument ${argType} is not assignable to ${parameterType}`));
      }
    }
  }

  private handleCall(
      callLocation: MLocation,
      funcType: MType,
      funcSymbol: MSymbol | null,
      args: ast.Expression[],
      argLocations: MLocation[] | null,
      bindingsContainer: Map<string, MType | null>[] | null = null): type.MType {
    const signature = funcSymbol?.functionSignature || null;
    if (signature) {
      const bindings = new Map<string, MType | null>(
        signature.typeParameters.map(tp => [tp.symbol.name, null]));
      if (bindingsContainer) {
        bindingsContainer.push(bindings);
      }
      const allParameters = signature.parameters.concat(signature.optionalParameters);
      const allParameterNames = allParameters.map(p => p[0]);
      const allParameterTypes = allParameters.map(p => p[1]);
      const returnType = signature.returnType;
      this.checkArgTypes(
        callLocation,
        funcSymbol?.name || null,
        funcSymbol?.documentation || null,
        args,
        argLocations,
        allParameterTypes,
        allParameterNames,
        signature.optionalParameters.length,
        returnType,
        bindings);
      return bindings.size > 0 ?
        new TypeBinder(bindings, false).bind(returnType, type.Any) :
        returnType;
    }
    if (funcType === type.Any) {
      for (const arg of args) {
        this.solveExpression(arg);
      }
      return type.Any;
    }
    if (funcType instanceof type.Class) {
      const instanceType = type.Instance.of(funcType.symbol);
      const initMethodSymbol = instanceType.getMethodSymbol('__init__');
      const [parameterTypes, optCount] =
        initMethodSymbol !== null && initMethodSymbol.valueType instanceof type.Function ?
          [initMethodSymbol.valueType.parameters, initMethodSymbol.valueType.optionalCount] :
          [[], 0];
      let parameterNames: string[] | null = null;
      if (initMethodSymbol?.functionSignature?.parameters) {
        const parameters = initMethodSymbol.functionSignature.parameters;
        parameterNames = parameters.map(p => p[0]);
      }
      this.checkArgTypes(
        callLocation,
        funcType.symbol.name,
        initMethodSymbol?.documentation || null,
        args,
        argLocations,
        parameterTypes,
        parameterNames,
        optCount,
        null,
        new Map());
      return instanceType;
    }
    if (funcType instanceof type.Function) {
      let parameterNames: string[] | null = null;
      this.checkArgTypes(
        callLocation,
        funcSymbol?.name || null,
        funcSymbol?.documentation || null,
        args,
        argLocations,
        funcType.parameters,
        parameterNames,
        funcType.optionalCount,
        funcType.returnType,
        new Map());
      return funcType.returnType;
    }
    this.errors.push(new MError(callLocation, `${funcType} is not callable`));
    for (const arg of args) {
      this.solveExpression(arg);
    }
    return type.Any;
  }

  visitFunctionCall(e: ast.FunctionCall): type.MType {
    const funcType = this.solveExpression(e.func);
    let funcSymbol: MSymbol | null = null;
    let funcIdentifier: ast.Identifier | null = null;
    if (e.func instanceof ast.GetVariable) {
      funcIdentifier = e.func.identifier;
      const foundSymbol = this.scope.get(funcIdentifier.name);
      if (foundSymbol) {
        funcSymbol = foundSymbol;
      }
    }
    const bindingsContainer: Map<string, MType | null>[] = [];
    const result = this.handleCall(
      e.location, funcType, funcSymbol, e.args, e.argLocations, bindingsContainer);
    if (funcIdentifier && funcSymbol && bindingsContainer.length === 1) {
      this.solver.recordSymbolUsage(funcIdentifier, funcSymbol, bindingsContainer[0]);
    }
    return result;
  }

  visitMethodCall(e: ast.MethodCall): type.MType {
    const ownerType = this.solveExpression(e.owner);
    const methodSymbol = ownerType.getMethodSymbol(e.identifier.name);
    if (!methodSymbol) {
      this.errors.push(new MError(
        e.identifier.location,
        `Member ${e.identifier.name} not found in ${ownerType}`));
      for (const arg of e.args) {
        this.solveExpression(arg);
      }
      return type.Any;
    }
    this.solver.recordSymbolUsage(e.identifier, methodSymbol);
    const methodType = methodSymbol.valueType || type.Any;
    return this.handleCall(e.location, methodType, methodSymbol, e.args, e.argLocations);
  }

  private getFieldSymbolAndRecordUsage(
      ownerType: MType, identifier: ast.Identifier): MSymbol | null {
    const fieldSymbol = ownerType.getFieldSymbol(identifier.name);
    if (!fieldSymbol) {
      this.errors.push(new MError(
        identifier.location,
        `Field ${identifier.name} not found in ${ownerType}`));
      return null;
    }
    this.solver.recordSymbolUsage(identifier, fieldSymbol);
    return fieldSymbol;
  }

  visitGetField(e: ast.GetField): type.MType {
    const ownerType = this.solveExpression(e.owner);
    const memberScope = ownerType.getCompletionScope();
    if (memberScope) {
      this.solver.completionPoints.push(new CompletionPoint(e.identifier.location, memberScope));
    }
    const fieldSymbol = this.getFieldSymbolAndRecordUsage(ownerType, e.identifier);
    return fieldSymbol?.valueType || type.Any;
  }

  visitSetField(e: ast.SetField): type.MType {
    const ownerType = this.solveExpression(e.owner);
    const fieldSymbol = this.getFieldSymbolAndRecordUsage(ownerType, e.identifier);
    const fieldType = fieldSymbol?.valueType || type.Any;
    const valueType = this.solveExpression(e.value);
    if (!valueType.isAssignableTo(fieldType)) {
      this.errors.push(new MError(
        e.value.location,
        `${valueType} is not assignable to ${fieldType}`));
    }
    return fieldType;
  }

  visitDot(e: ast.Dot): type.MType {
    const ownerType = this.solveExpression(e.owner);
    const memberScope = ownerType.getCompletionScope();
    if (memberScope) {
      this.solver.completionPoints.push(new CompletionPoint(e.dotLocation, memberScope));
    }
    this.errors.push(new MError(e.followLocation, `Expected member identifier`));
    return type.Any;
  }

  visitLogical(e: ast.Logical): type.MType {
    switch (e.op) {
      case 'not':
        if (e.args.length !== 1) {
          throw new Error(`assertion error ${e.op}, ${e.args.length}`);
        }
        this.solveExpression(e.args[0]);
        return type.Bool;
      case 'and': {
        if (e.args.length !== 2) {
          throw new Error(`assertion error ${e.op}, ${e.args.length}`);
        }
        const lhsType = this.solveExpression(e.args[0]);
        const rhsType = this.solveExpression(e.args[1]);
        return lhsType.closestCommonType(rhsType);
      }
      case 'or': {
        if (e.args.length !== 2) {
          throw new Error(`assertion error ${e.op}, ${e.args.length}`);
        }
        const lhsType = this.solveExpression(e.args[0]).filterTruthy();
        const rhsType = this.solveExpression(e.args[1]);
        return lhsType.closestCommonType(rhsType);
      }
    }
  }

  visitRaise(e: ast.Raise): type.MType {
    this.solveExpression(e.exception);
    return type.Never;
  }
}

class StatementVisitor extends ast.StatementVisitor<void> {
  readonly solver: Solver;

  constructor(solver: Solver) {
    super();
    this.solver = solver;
  }

  private withScope(scope: MScope): Solver {
    return this.solver.withScope(scope);
  }

  private solveStatement(s: ast.Statement, scope: MScope | null = null) {
    if (scope) {
      return this.withScope(scope).solveStatement(s);
    }
    return s.accept(this);
  }

  private solveType(t: ast.TypeExpression, scope: MScope | null = null): MType {
    if (scope) {
      return this.withScope(scope).solveType(t);
    }
    return this.solver.solveType(t);
  }

  private solveExpression(
      e: ast.Expression,
      typeHint: MType = type.Any,
      scope: MScope | null = null): MType {
    if (scope) {
      return this.withScope(scope).solveExpression(e, typeHint);
    }
    return this.solver.solveExpression(e, typeHint);
  }

  visitNop(s: ast.Nop) {
  }

  private checkParameters(parameters: ast.Parameter[]) {
    let i = 0;
    for (; i < parameters.length; i++) {
      if (parameters[i].defaultValue !== null) {
        break;
      }
    }
    for (; i < parameters.length; i++) {
      if (parameters[i].defaultValue === null) {
        this.solver.errors.push(new MError(
          parameters[i].location,
          `non-optional parameter may not follow an optional parameter`));
      }
    }
  }

  computeFunctionSignature(s: ast.Function, symbol: MSymbol) {
    symbol.documentation = s.documentation?.value || null;
    this.checkParameters(s.parameters);
    const typeParameters: type.TypeParameter[] = [];
    const parameters: [string, MType][] = [];
    const optionalParameters: [string, MType][] = [];

    // We need a temporary scope to hold the type parameter types as we solve
    // the parameter and return type type expressions.
    // When we visit the function again, we'll need to add these to the scope again.
    const innerSolver = this.solver.withScope(MScope.new(this.solver.scope));
    for (const tp of s.typeParameters) {
      const tpSymbol = innerSolver.recordSymbolDefinition(
        tp.identifier, true, true);
      const typeParameterType = new type.TypeParameter(tpSymbol, type.Any);
      tpSymbol.typeType = typeParameterType;
      typeParameters.push(typeParameterType);
    }
    for (const parameter of s.parameters) {
      const parameterType = innerSolver.solveType(parameter.typeExpression);
      if (parameter.defaultValue === null) {
        parameters.push([parameter.identifier.name, parameterType]);
      } else {
        optionalParameters.push([parameter.identifier.name, parameterType]);
      }
    }
    const returnType = innerSolver.solveType(s.returnType);
    const signature = new type.FunctionSignature(
      typeParameters, parameters, optionalParameters, returnType);
    symbol.functionSignature = signature;
    symbol.valueType = signature.toType();
  }

  private visitFunctionOrMethod(s: ast.Function, symbol: MSymbol) {
    if (symbol.functionSignature === null) {
      this.computeFunctionSignature(s, symbol);
    }
    if (symbol.functionSignature === null) {
      throw Error(`Assertion error: symbol.functionSignature is null`);
    }
    const functionScope = MScope.new(this.solver.scope);
    const functionSolver = this.withScope(functionScope);
    const paramCount = s.parameters.filter(p => p.defaultValue === null).length;
    const optCount = s.parameters.filter(p => p.defaultValue !== null).length;
    for (const typeParameterType of symbol.functionSignature.typeParameters) {
      functionScope.set(typeParameterType.symbol);
    }
    if (paramCount !== symbol.functionSignature.parameters.length ||
        optCount !== symbol.functionSignature.optionalParameters.length) {
      throw Error(`Assertion error: function parameter length does not match`);
    }
    for (let i = 0; i < s.parameters.length; i++) {
      const parameter = s.parameters[i];
      const [name, parameterType] =
        i < symbol.functionSignature.parameters.length ?
          symbol.functionSignature.parameters[i] :
          symbol.functionSignature.optionalParameters[
            i - symbol.functionSignature.parameters.length];
      if (parameter.identifier.name !== name) {
        throw Error(`Assertion error: parameter name does not match`);
      }
      const parameterSymbol = functionSolver.recordSymbolDefinition(
        parameter.identifier, true, false);
      parameterSymbol.valueType = parameterType;
    }
    functionSolver.solveStatement(s.body);
  }

  visitFunction(s: ast.Function) {
    const previous = this.solver.scope.get(s.identifier.name);
    if (previous) {
      // If a symbol with the same name has previously been defined,
      // and this symbol was just a forward declare for this function,
      // we want to use the same symbol.
      // It's a bit of a hack, but we check this by checking if the
      // symbol location exactly matches the identifier location.
      if (previous.location === s.identifier.location) {
        this.visitFunctionOrMethod(s, previous);
      } else {
        this.solver.errors.push(new MError(
          s.identifier.location,
          `Duplicate definition of ${s.identifier.name}`));
      }
    } else {
      const functionSymbol = this.solver.recordSymbolDefinition(s.identifier, true);
      this.visitFunctionOrMethod(s, functionSymbol);
    }
  }

  visitClass(s: ast.Class) {
    const classSymbol = this.solver.scope.get(s.identifier.name);
    if (!classSymbol) {
      throw new Error(`Assertion Error: class symbol not found (${s.identifier.name})`);
    }
    if (!this.solver.basesMap) {
      throw new Error(`Assertion Error: baseMap is missing`);
    }
    const baseValueTypes = this.solver.basesMap.get(s.identifier.name);
    if (!baseValueTypes) {
      throw new Error(`Assertion Error: baseValueTypes not found`);
    }
    if (!classSymbol.staticMembers) {
      throw new Error(`Assertion Error: classSymbol.staticMembers is missing`);
    }
    for (const staticMethod of s.staticMethods) {
      const staticMethodSymbol = classSymbol.staticMembers.get(staticMethod.identifier.name);
      if (!staticMethodSymbol) {
        throw Error(
          `Assertion Error: static method not found ` +
          `(${classSymbol.name}.${staticMethod.identifier.name})`);
      }
      this.solver.statementVisitor.visitFunctionOrMethod(
        staticMethod, staticMethodSymbol);
    }
    const classScope = MScope.new(this.solver.scope);
    const classSolver = this.withScope(classScope);
    classSymbol.documentation = s.documentation?.value || null;
    const thisIdentifier = new ast.Identifier(s.identifier.location, 'this');
    const thisSymbol = classSolver.recordSymbolDefinition(thisIdentifier, true);
    thisSymbol.valueType = type.Instance.of(classSymbol);
    if (baseValueTypes.length > 0) {
      const superIdentifier = new ast.Identifier(s.identifier.location, 'super');
      const superSymbol = classSolver.recordSymbolDefinition(superIdentifier, true);
      superSymbol.valueType = type.Instance.of(baseValueTypes[0].symbol);
    }
    for (const method of s.methods) {
      const methodSymbol = classSymbol.members.get(method.identifier.name);
      if (!methodSymbol) {
        throw Error(`Assertion Error: method not found`);
      }
      classSolver.statementVisitor.visitFunctionOrMethod(method, methodSymbol);
    }
  }

  visitImport(s: ast.Import) {
    // IGNORE - assume that imports were added to the scope during parse
  }

  visitVariable(s: ast.Variable) {
    const variableSymbol = this.solver.recordSymbolDefinition(s.identifier, true, s.final);
    const explicitType = s.typeExpression ? this.solveType(s.typeExpression) : null;
    const valueType = this.solver.solveExpression(s.valueExpression, explicitType || type.Any);
    const variableType = explicitType ? explicitType : valueType;
    variableSymbol.valueType = variableType;
    variableSymbol.documentation = s.documentation;
    if (explicitType && !valueType.isAssignableTo(explicitType)) {
      this.solver.errors.push(new MError(
        s.location,
        `Cannot assign ${valueType} to ${explicitType}`));
    }
  }

  visitWhile(s: ast.While) {
    this.solveExpression(s.condition);
    this.solveStatement(s.body);
  }

  visitFor(s: ast.For) {
    const forScope = MScope.new(this.solver.scope);
    const forSolver = this.withScope(forScope);
    const variableSymbol = forSolver.recordSymbolDefinition(s.variable, true);
    const containerType = forSolver.solveExpression(s.container);
    const variableType = containerType.getForInItemType();
    if (variableType) {
      variableSymbol.valueType = variableType;
    } else {
      forSolver.errors.push(new MError(
        s.container.location,
        `${containerType} is not iterable`));
    }
    forSolver.solveStatement(s.body);
  }

  visitIf(s: ast.If) {
    for (const [cond, body] of s.pairs) {
      this.solveExpression(cond, type.Bool);
      this.solveStatement(body);
    }
    if (s.fallback) {
      this.solveStatement(s.fallback);
    }
  }

  visitBlock(s: ast.Block) {
    const blockScope = MScope.new(this.solver.scope);
    const newSolver = this.withScope(blockScope);
    for (const statement of s.statements) {
      newSolver.solveStatement(statement);
    }
  }

  visitReturn(s: ast.Return) {
    // TODO: check return type
    this.solveExpression(s.expression);
  }

  visitExpressionStatement(s: ast.ExpressionStatement) {
    this.solveExpression(s.expression);
  }
}
