import * as ast from "./ast";
import { MError } from "./error";
import { MType } from "./type";
import * as type from "./type";
import { MScope } from "./scope";
import { MSymbolUsage } from "./symbol";
import { MLocation } from "./location";

export class TypeSolver {
  private readonly cache: Map<ast.TypeExpression | ast.Expression, MType> = new Map();
  private readonly errors: MError[];
  private readonly symbolUsages: MSymbolUsage[];

  constructor(errors: MError[], symbolUsages: MSymbolUsage[]) {
    this.errors = errors;
    this.symbolUsages = symbolUsages;
  }

  solve(e: ast.TypeExpression | ast.Expression, scope: MScope): MType {
    const cached = this.cache.get(e);
    if (cached) {
      return cached;
    }
    const solved = this.solveNoCache(e, scope);
    this.cache.set(e, solved);
    return solved;
  }

  private solveNoCache(e: ast.TypeExpression | ast.Expression, scope: MScope): MType {
    if (e instanceof ast.TypeExpression) {
      return this.solveTypeExpression(e, scope);
    }
    return this.solveExpression(e, scope);
  }

  private checkTypeArgc(te: ast.TypeExpression, argc: number) {
    if (te.args.length !== argc) {
      this.errors.push(new MError(
        te.location,
        `Expected ${argc} type arguments but got ${te.args.length}`));
    }
  }

  private solveTypeExpression(te: ast.TypeExpression, scope: MScope): MType {
    if (te.identifier.parent && te.identifier.parent.parent === null) {
      const parentIdentifier = te.identifier.parent.identifier;
      const parentName = parentIdentifier.name;
      const memberIdentifier = te.identifier.identifier;
      const memberName = memberIdentifier.name;
      const parentSymbol = scope.get(parentName);
      if (!parentSymbol) {
        this.errors.push(new MError(
          parentIdentifier.location,
          `Name '${parentName}' not found in type expression`));
        return type.Any;
      }
      const parentUsage = new MSymbolUsage(parentIdentifier.location, parentSymbol);
      parentSymbol.usages.push(parentUsage);
      this.symbolUsages.push(parentUsage);
      const memberSymbol = parentSymbol.members.get(memberName);
      if (!memberSymbol) {
        this.errors.push(new MError(
          memberIdentifier.location,
          `Name '${memberName}' not found in '${parentName}`));
        return type.Any;
      }
      const memberUsage = new MSymbolUsage(memberIdentifier.location, memberSymbol);
      memberSymbol.usages.push(memberUsage);
      this.symbolUsages.push(memberUsage);
      if (memberSymbol.type instanceof type.Class) {
        return type.Instance.of(memberSymbol.type.symbol);
      }
      return memberSymbol.type;
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
        return type.List.of(this.solveTypeExpression(te.args[0], scope));
      case 'dict':
        if (te.args.length === 0) {
          return type.UntypedDict;
        }
        this.checkTypeArgc(te, 2);
        return type.Dict.of(
          this.solveTypeExpression(te.args[0], scope),
          this.solveTypeExpression(te.args[1], scope));
      case 'optional':
        if (te.args.length === 0) {
          return type.UntypedOptional;
        }
        this.checkTypeArgc(te, 1);
        return type.Optional.of(this.solveTypeExpression(te.args[0], scope));
      case 'function':
        if (te.args.length === 0) {
          return type.UntypedFunction;
        }
        const parameters: MType[] = [];
        for (let i = 0; i + 1 < te.args.length; i++) {
          parameters.push(this.solveTypeExpression(te.args[i], scope));
        }
        const returnType = this.solveTypeExpression(te.args[te.args.length - 1], scope);
        return type.Function.of(parameters, 0, returnType);
    }
    this.errors.push(new MError(
      te.identifier.location, `unrecognized type name ${te.identifier}`));
    return type.Any;
  }

  solveExpression(
      e: ast.Expression,
      scope: MScope,
      typeHint: MType = type.NoReturn): MType {
    const result = e.accept(new ExpressionTypeSolver(this, scope, typeHint, this.errors));
    this.cache.set(e, result);
    return result;
  }
}

class ExpressionTypeSolver extends ast.ExpressionVisitor<MType> {
  private readonly typeSolver: TypeSolver
  private readonly scope: MScope
  private readonly typeHint: MType
  private readonly errors: MError[]
  constructor(typeSolver: TypeSolver, scope: MScope, typeHint: MType, errors: MError[]) {
    super();
    this.typeSolver = typeSolver;
    this.scope = scope;
    this.typeHint = typeHint;
    this.errors = errors;
  }

  visitGetVariable(e: ast.GetVariable): MType {
    const symbol = this.scope.get(e.identifier.name);
    if (!symbol) {
      return type.Any;
    }
    return symbol.type;
  }

  visitSetVariable(e: ast.SetVariable): MType {
    const symbol = this.scope.get(e.identifier.name);
    if (!symbol) {
      return type.Any;
    }
    if (symbol.final) {
      this.errors.push(new MError(
        e.location,
        `Cannot assign to a final variable`));
    }
    const symbolType = symbol.type;
    const rhsType = this.typeSolver.solveExpression(
      e.value, this.scope, symbol.type);
    if (!rhsType.isAssignableTo(symbolType)) {
      this.errors.push(new MError(
        e.location,
        `Cannot assign ${rhsType} to ${symbolType}`));
    }
    return symbolType;
  }

  visitNilLiteral(e: ast.NilLiteral): MType {
    return type.Nil;
  }

  visitBoolLiteral(e: ast.BoolLiteral): MType {
    return type.Bool;
  }

  visitNumberLiteral(e: ast.NumberLiteral): MType {
    return type.Number;
  }

  visitStringLiteral(e: ast.StringLiteral): MType {
    return type.String;
  }

  visitListDisplay(e: ast.ListDisplay): MType {
    let itemType: MType = type.NoReturn;
    if (this.typeHint instanceof type.List) {
      itemType = this.typeHint.itemType;
    }
    for (let item of e.items) {
      itemType = itemType.closestCommonType(this.typeSolver.solve(item, this.scope));
    }
    return type.List.of(itemType);
  }

  visitDictDisplay(e: ast.DictDisplay): MType {
    let keyType: MType = type.NoReturn;
    let valueType: MType = type.NoReturn;
    if (this.typeHint instanceof type.Dict) {
      keyType = this.typeHint.keyType;
      valueType = this.typeHint.valueType;
    }
    for (let [key, value] of e.pairs) {
      keyType = keyType.closestCommonType(this.typeSolver.solve(key, this.scope));
      valueType = valueType.closestCommonType(this.typeSolver.solve(value, this.scope));
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
      const argType = this.typeSolver.solveExpression(args[i], this.scope, parameterType);
      if (!argType.isAssignableTo(parameterType)) {
        this.errors.push(new MError(
          args[i].location,
          `Argument ${argType} is not assignable to ${parameterType}`));
      }
    }
  }

  visitFunctionCall(e: ast.FunctionCall): MType {
    const funcType = this.typeSolver.solveExpression(e.func, this.scope);
    if (funcType === type.Any) {
      for (const arg of e.args) {
        this.typeSolver.solveExpression(arg, this.scope);
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
      this.typeSolver.solveExpression(arg, this.scope);
    }
    return type.Any;
  }

  visitMethodCall(e: ast.MethodCall): MType {
    const ownerType = this.typeSolver.solveExpression(e.owner, this.scope);
    if (ownerType === type.Any) {
      for (const arg of e.args) {
        this.typeSolver.solveExpression(arg, this.scope);
      }
      return type.Any;
    }
    const memberType = ownerType.getMethodType(e.identifier.name);
    if (!memberType) {
      this.errors.push(new MError(
        e.identifier.location,
        `Member ${e.identifier.name} not found in ${ownerType}`));
      for (const arg of e.args) {
        this.typeSolver.solveExpression(arg, this.scope);
      }
      return type.Any;
    }
    if (memberType === type.Any) {
      for (const arg of e.args) {
        this.typeSolver.solveExpression(arg, this.scope);
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
      this.typeSolver.solveExpression(arg, this.scope);
    }
    return type.Any;
  }

  visitGetField(e: ast.GetField): MType {
    const ownerType = this.typeSolver.solveExpression(e.owner, this.scope);
    if (ownerType === type.Any) {
      return type.Any;
    }
    const memberType = ownerType.getFieldType(e.identifier.name);
    if (!memberType) {
      this.errors.push(new MError(
        e.identifier.location,
        `Member ${e.identifier.name} not found in ${ownerType}`));
      return type.Any;
    }
    return memberType;
  }

  visitSetField(e: ast.SetField): MType {
    const ownerType = this.typeSolver.solveExpression(e.owner, this.scope);
    if (ownerType === type.Any) {
      return type.Any;
    }
    const memberType = ownerType.getFieldType(e.identifier.name);
    if (!memberType) {
      this.errors.push(new MError(
        e.identifier.location,
        `Member ${e.identifier.name} not found in ${ownerType}`));
      return type.Any;
    }
    const valueType = this.typeSolver.solveExpression(e.value, this.scope);
    if (!valueType.isAssignableTo(memberType)) {
      this.errors.push(new MError(
        e.value.location,
        `${valueType} is not assignable to ${memberType}`));
    }
    return memberType;
  }

  visitLogical(e: ast.Logical): MType {
    switch (e.op) {
      case 'not':
        if (e.args.length !== 0) {
          throw new Error(`assertion error ${e.op}`);
        }
        this.typeSolver.solveExpression(e.args[0], this.scope);
        return type.Bool;
      case 'and':
      case 'or':
        if (e.args.length !== 2) {
          throw new Error(`assertion error ${e.op}, ${e.args.length}`);
        }
        const lhsType = this.typeSolver.solveExpression(e.args[0], this.scope);
        const rhsType = this.typeSolver.solveExpression(e.args[1], this.scope);
        return lhsType.closestCommonType(rhsType);
    }
  }

  visitRaise(e: ast.Raise): MType {
    return type.NoReturn;
  }
}
