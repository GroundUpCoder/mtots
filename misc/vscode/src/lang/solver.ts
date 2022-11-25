import * as ast from "./ast";
import { MError } from "./error";
import { MLocation } from "./location";
import { MScope } from "./scope";
import { MSymbol, MSymbolUsage } from "./symbol";
import * as type from "./type";
import { MType } from "./type";



export class Solver {
  readonly scope: MScope;
  readonly errors: MError[];
  readonly symbolUsages: MSymbolUsage[];
  private readonly typeVisitor: TypeVisitor;
  private readonly statementVisitor: StatementVisitor;

  constructor(scope: MScope, errors: MError[], symbolUsages: MSymbolUsage[]) {
    this.scope = scope;
    this.errors = errors;
    this.symbolUsages = symbolUsages;
    this.typeVisitor = new TypeVisitor(this);
    this.statementVisitor = new StatementVisitor(this);
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
      identifier: ast.Identifier, addToScope: boolean, final: boolean = true): MSymbol {
    const symbol = new MSymbol(identifier.name, identifier.location, final);
    if (addToScope) {
      const previous = this.scope.map.get(identifier.name);
      if (previous) {
        this.errors.push(new MError(
          identifier.location,
          `'${previous.name}' is already defined in this scope`));
      }
      this.scope.set(symbol);
    }
    this.symbolUsages.push(symbol.definition);
    return symbol;
  }

  recordSymbolUsage(identifier: ast.Identifier, symbol: MSymbol) {
    const usage = new MSymbolUsage(identifier.location, symbol);
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
    if (te.identifier.parent && te.identifier.parent.parent === null) {
      // Qualified type name
      const parentIdentifier = te.identifier.parent.identifier;
      const parentName = parentIdentifier.name;
      const memberIdentifier = te.identifier.identifier;
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
      const memberSymbol = parentSymbol.members.get(memberName);
      if (!memberSymbol) {
        this.errors.push(new MError(
          memberIdentifier.location,
          `Type '${memberName}' not found in '${parentName}`));
        return type.Any;
      }
      const memberUsage = new MSymbolUsage(memberIdentifier.location, memberSymbol);
      memberSymbol.usages.push(memberUsage);
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
    const name = te.identifier.toString();
    switch (name) {
      case 'any': this.checkTypeArgc(te, 0); return type.Any;
      case 'noreturn': this.checkTypeArgc(te, 0); return type.NoReturn;
      case 'nil': this.checkTypeArgc(te, 0); return type.Nil;
      case 'bool': this.checkTypeArgc(te, 0); return type.Bool;
      case 'float': // TODO: specialized float and int types
      case 'int':
      case 'number': this.checkTypeArgc(te, 0); return type.Number;
      case 'string': this.checkTypeArgc(te, 0); return type.String;
      case 'list':
        if (te.args.length === 0) {
          return type.UntypedList;
        }
        this.checkTypeArgc(te, 1);
        return type.List.of(this.solveTypeExpression(te.args[0]));
      case 'dict':
        if (te.args.length === 0) {
          return type.UntypedDict;
        }
        this.checkTypeArgc(te, 2);
        return type.Dict.of(
          this.solveTypeExpression(te.args[0]),
          this.solveTypeExpression(te.args[1]));
      case 'optional':
        this.checkTypeArgc(te, 1);
        return type.Optional.of(this.solveTypeExpression(te.args[0]));
      case 'iterate':
        this.checkTypeArgc(te, 1);
        return type.Iterate.of(this.solveTypeExpression(te.args[0]));
      case 'function':
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
    if (te.identifier.parent === null && te.args.length === 0) {
      // Simple type name
      const typeIdentifier = te.identifier.identifier;
      const typeName = typeIdentifier.name;
      const typeSymbol = scope.get(typeName);
      if (typeSymbol) {
        const typeUsage = new MSymbolUsage(typeIdentifier.location, typeSymbol);
        typeSymbol.usages.push(typeUsage);
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
      te.identifier.location, `unrecognized type name ${te.identifier}`));
    return type.Any;
  }
}

class ExpressionVisitor extends ast.ExpressionVisitor<MType> {
  readonly solver: Solver;
  readonly typeHint: MType;
  private readonly scope: MScope;
  private readonly errors: MError[];
  private readonly symbolUsages: MSymbolUsage[];

  constructor(solver: Solver, typeHint: MType) {
    super();
    this.solver = solver;
    this.typeHint = typeHint;
    this.scope = solver.scope;
    this.errors = solver.errors;
    this.symbolUsages = solver.symbolUsages;
  }

  private solveExpression(e: ast.Expression, typeHint: MType = type.Any) {
    return e.accept(new ExpressionVisitor(this.solver, typeHint));
  }

  visitGetVariable(e: ast.GetVariable): type.MType {
    const symbol = this.scope.get(e.identifier.name);
    if (!symbol) {
      return type.Any;
    }
    this.solver.recordSymbolUsage(e.identifier, symbol);
    return symbol.valueType;
  }

  visitSetVariable(e: ast.SetVariable): type.MType {
    const symbol = this.scope.get(e.identifier.name);
    if (!symbol) {
      return type.Any;
    }
    this.solver.recordSymbolUsage(e.identifier, symbol);
    if (symbol.final) {
      this.errors.push(new MError(
        e.location,
        `Cannot assign to a final variable`));
    }
    const symbolType = symbol.valueType;
    const rhsType = this.solveExpression(e.value, symbol.valueType);
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
    let itemType: MType = type.NoReturn;
    if (this.typeHint instanceof type.List) {
      itemType = this.typeHint.itemType;
    }
    for (let item of e.items) {
      itemType = itemType.closestCommonType(this.solveExpression(item));
    }
    return type.List.of(itemType);
  }

  visitDictDisplay(e: ast.DictDisplay): type.MType {
    let keyType: MType = type.NoReturn;
    let valueType: MType = type.NoReturn;
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

  private checkArgTypes(
      functionLocation: MLocation,
      args: ast.Expression[],
      parameterTypes: MType[],
      optionalCount: number) {
    if (args.length < parameterTypes.length - optionalCount ||
        args.length > parameterTypes.length) {
      this.errors.push(new MError(
        functionLocation,
        optionalCount === 0 ?
          `${parameterTypes.length} args expected but got ${args.length}` :
          `${parameterTypes.length - optionalCount} to ${parameterTypes.length} args ` +
          `expected but got ${args.length}`));
      return;
    }
    for (let i = 0; i < args.length; i++) {
      const parameterType = parameterTypes[i];
      const argType = this.solveExpression(args[i], parameterType);
      if (!argType.isAssignableTo(parameterType)) {
        this.errors.push(new MError(
          args[i].location,
          `Argument ${argType} is not assignable to ${parameterType}`));
      }
    }
  }

  visitFunctionCall(e: ast.FunctionCall): type.MType {
    const funcType = this.solveExpression(e.func);
    if (funcType === type.Any) {
      for (const arg of e.args) {
        this.solveExpression(arg);
      }
      return type.Any;
    }
    if (funcType instanceof type.Class) {
      const instanceType = type.Instance.of(funcType.symbol);
      const initMethodType = instanceType.getMethodType('__init__');
      const [parameterTypes, optCount] =
        initMethodType !== null && initMethodType instanceof type.Function ?
          [initMethodType.parameters, initMethodType.optionalCount] :
          [[], 0];
      this.checkArgTypes(
        e.func.location,
        e.args,
        parameterTypes,
        optCount);
      return instanceType;
    }
    if (funcType instanceof type.Function) {
      this.checkArgTypes(e.func.location, e.args, funcType.parameters, funcType.optionalCount);
      return funcType.returnType;
    }
    this.errors.push(new MError(e.func.location, `${funcType} is not callable`));
    for (const arg of e.args) {
      this.solveExpression(arg);
    }
    return type.Any;
  }

  visitMethodCall(e: ast.MethodCall): type.MType {
    const ownerType = this.solveExpression(e.owner);
    if (ownerType === type.Any) {
      for (const arg of e.args) {
        this.solveExpression(arg);
      }
      return type.Any;
    }
    this.checkMemberUsage(ownerType, e.identifier);
    const memberType = ownerType.getMethodType(e.identifier.name);
    if (!memberType) {
      this.errors.push(new MError(
        e.identifier.location,
        `Member ${e.identifier.name} not found in ${ownerType}`));
      for (const arg of e.args) {
        this.solveExpression(arg);
      }
      return type.Any;
    }
    if (memberType === type.Any) {
      for (const arg of e.args) {
        this.solveExpression(arg);
      }
      return type.Any;
    }
    if (memberType instanceof type.Class) {
      const instanceType = type.Instance.of(memberType.symbol);
      const initMethodType = instanceType.getMethodType('__init__');
      const [parameterTypes, optCount] =
        initMethodType !== null && initMethodType instanceof type.Function ?
          [initMethodType.parameters, initMethodType.optionalCount] :
          [[], 0];
      this.checkArgTypes(
        e.identifier.location,
        e.args,
        parameterTypes,
        optCount);
      return type.Instance.of(memberType.symbol);
    }
    if (memberType instanceof type.Function) {
      this.checkArgTypes(
        e.identifier.location, e.args, memberType.parameters, memberType.optionalCount);
      return memberType.returnType;
    }
    this.errors.push(new MError(e.identifier.location, `${memberType} is not callable`));
    for (const arg of e.args) {
      this.solveExpression(arg);
    }
    return type.Any;
  }

  private checkMemberUsage(ownerType: MType, memberIdentifier: ast.Identifier) {
    if (ownerType instanceof type.Instance) {
      const memberSymbol = ownerType.symbol.members.get(memberIdentifier.name);
      if (memberSymbol) {
        this.solver.recordSymbolUsage(memberIdentifier, memberSymbol);
      }
    } else if (ownerType instanceof type.Module) {
      const memberSymbol = ownerType.symbol.members.get(memberIdentifier.name);
      if (memberSymbol) {
        this.solver.recordSymbolUsage(memberIdentifier, memberSymbol);
      }
    }
  }

  visitGetField(e: ast.GetField): type.MType {
    const ownerType = this.solveExpression(e.owner);
    if (ownerType === type.Any) {
      return type.Any;
    }
    this.checkMemberUsage(ownerType, e.identifier);
    const memberType = ownerType.getFieldType(e.identifier.name);
    if (!memberType) {
      this.errors.push(new MError(
        e.identifier.location,
        `Member ${e.identifier.name} not found in ${ownerType}`));
      return type.Any;
    }
    return memberType;
  }

  visitSetField(e: ast.SetField): type.MType {
    const ownerType = this.solveExpression(e.owner);
    if (ownerType === type.Any) {
      return type.Any;
    }
    this.checkMemberUsage(ownerType, e.identifier);
    const memberType = ownerType.getFieldType(e.identifier.name);
    if (!memberType) {
      this.errors.push(new MError(
        e.identifier.location,
        `Member ${e.identifier.name} not found in ${ownerType}`));
      return type.Any;
    }
    const valueType = this.solveExpression(e.value);
    if (!valueType.isAssignableTo(memberType)) {
      this.errors.push(new MError(
        e.value.location,
        `${valueType} is not assignable to ${memberType}`));
    }
    return memberType;
  }

  visitLogical(e: ast.Logical): type.MType {
    switch (e.op) {
      case 'not':
        if (e.args.length !== 1) {
          throw new Error(`assertion error ${e.op}, ${e.args.length}`);
        }
        this.solveExpression(e.args[0]);
        return type.Bool;
      case 'and':
      case 'or':
        if (e.args.length !== 2) {
          throw new Error(`assertion error ${e.op}, ${e.args.length}`);
        }
        const lhsType = this.solveExpression(e.args[0]);
        const rhsType = this.solveExpression(e.args[1]);
        return lhsType.closestCommonType(rhsType);
    }
  }

  visitRaise(e: ast.Raise): type.MType {
    this.solveExpression(e.exception);
    return type.NoReturn;
  }
}

class StatementVisitor extends ast.StatementVisitor<void> {
  readonly solver: Solver;

  constructor(solver: Solver) {
    super();
    this.solver = solver;
  }

  private withScope(scope: MScope): Solver {
    return new Solver(scope, this.solver.errors, this.solver.symbolUsages);
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

  private visitFunctionOrMethod(s: ast.Function, symbol: MSymbol) {
    const functionScope = new MScope(this.solver.scope);
    const functionSolver = this.withScope(functionScope);
    this.checkParameters(s.parameters);
    const parameterTypes: MType[] = [];
    for (const parameter of s.parameters) {
      const parameterSymbol = functionSolver.recordSymbolDefinition(
        parameter.identifier, true, false);
      const parameterType = functionSolver.solveType(parameter.typeExpression);
      parameterSymbol.valueType = parameterType;
      parameterTypes.push(parameterType);
    }
    const returnType = functionSolver.solveType(s.returnType);
    const functionType = type.Function.of(
      parameterTypes,
      s.parameters.filter(p => p.defaultValue !== null).length,
      returnType);
    symbol.valueType = functionType;
    functionSolver.solveStatement(s.body);
  }

  visitFunction(s: ast.Function) {
    const functionSymbol = this.solver.recordSymbolDefinition(s.identifier, true);
    functionSymbol.documentation = s.documentation;
    this.visitFunctionOrMethod(s, functionSymbol);
  }

  visitClass(s: ast.Class) {
    const classSymbol = this.solver.recordSymbolDefinition(s.identifier, true);
    classSymbol.typeType = type.Instance.of(classSymbol);
    classSymbol.valueType = type.Class.of(classSymbol);
    const classScope = new MScope(this.solver.scope);
    const classSolver = this.withScope(classScope);
    const baseValueTypes: MType[] = s.bases.map(be => classSolver.solveExpression(be));
    for (const baseValueType of baseValueTypes) {
      if (baseValueType instanceof type.Class) {
        for (const [key, value] of baseValueType.symbol.members) {
          classSymbol.members.set(key, value);
        }
      }
    }
    classSymbol.documentation = s.documentation;
    for (const field of s.fields) {
      const fieldSymbol = classSolver.recordSymbolDefinition(
        field.identifier, false, field.final);
      classSymbol.members.set(fieldSymbol.name, fieldSymbol);
      const fieldType = classSolver.solveType(field.typeExpression);
      fieldSymbol.valueType = fieldType;
    }
    for (const method of s.methods) {
      const methodSymbol = classSolver.recordSymbolDefinition(method.identifier, false);
      classSymbol.members.set(methodSymbol.name, methodSymbol);
      this.visitFunctionOrMethod(method, methodSymbol);
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
    const variableSymbol = this.solver.recordSymbolDefinition(s.variable, true);
    const containerType = this.solveExpression(s.container);
    const variableType = containerType.getForInItemType();
    if (variableType) {
      variableSymbol.valueType = variableType;
    } else {
      this.solver.errors.push(new MError(
        s.container.location,
        `${containerType} is not iterable`));
    }
    this.solveStatement(s.body);
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
    const blockScope = new MScope(this.solver.scope);
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