import { MSymbol } from "./symbol";

export abstract class MType {
  private static nextID: number = 0;

  readonly id: number;

  constructor() {
    this.id = MType.nextID++;
  }

  abstract isSubclassOf(type: MType): boolean;
}

export class BuiltinPrimitive extends MType {
  readonly parent: BuiltinPrimitive | null;
  readonly name: string;

  static Any = new BuiltinPrimitive('any', null);
  static Nil = new BuiltinPrimitive('nil', BuiltinPrimitive.Any);
  static Bool = new BuiltinPrimitive('bool', BuiltinPrimitive.Any);
  static Float = new BuiltinPrimitive('float', BuiltinPrimitive.Any);
  static Int = new BuiltinPrimitive('int', BuiltinPrimitive.Float);
  static String = new BuiltinPrimitive('string', BuiltinPrimitive.Any);
  static UntypedList = new BuiltinPrimitive('list', BuiltinPrimitive.Any);
  static UntypedFunction = new BuiltinPrimitive('function', BuiltinPrimitive.Any);
  static UntypedOptional = new BuiltinPrimitive('optional', BuiltinPrimitive.Any);

  private constructor(name: string, parent: BuiltinPrimitive | null) {
    super();
    this.parent = parent;
    this.name = name;
  }

  getParent(): MType | null {
    return this.parent;
  }

  isSubclassOf(type: MType): boolean {
    return this === type || !!this.parent && this.parent.isSubclassOf(type);
  }

  toString() {
    return this.name;
  }
}

export const Any = BuiltinPrimitive.Any;
export const Nil = BuiltinPrimitive.Nil;
export const Bool = BuiltinPrimitive.Bool;
export const Float = BuiltinPrimitive.Float;
export const Int = BuiltinPrimitive.Int;
export const String = BuiltinPrimitive.String;
export const UntypedList = BuiltinPrimitive.UntypedList;
export const UntypedFunction = BuiltinPrimitive.UntypedFunction;
export const UntypedOptional = BuiltinPrimitive.UntypedOptional;

export class List extends MType {
  private static readonly map: Map<MType, List> = new Map();

  static of(itemType: MType) {
    const foundList = this.map.get(itemType);
    if (foundList) {
      return foundList;
    }
    const newList = new List(itemType);
    this.map.set(itemType, newList);
    return newList;
  }

  itemType: MType;
  private constructor(itemType: MType) {
    super();
    this.itemType = itemType;
  }

  isSubclassOf(type: MType): boolean {
    return this === type || UntypedList.isSubclassOf(type);
  }

  toString() {
    return `list[${this.itemType}]`
  }
}

export class Optional extends MType {
  private static readonly map: Map<MType, Optional> = new Map();

  static of(itemType: MType) {
    const cached = this.map.get(itemType);
    if (cached) {
      return cached;
    }
    const optional = new Optional(itemType);
    this.map.set(itemType, optional);
    return optional;
  }

  readonly itemType: MType;
  private constructor(itemType: MType) {
    super();
    this.itemType = itemType;
  }

  isSubclassOf(type: MType): boolean {
    return this === type || UntypedOptional.isSubclassOf(type);
  }

  toString() {
    return this.itemType + '?';
  }
}

export class Function extends MType {
  private static readonly map: Map<string, Function> = new Map();

  static of(parameters: MType[], returnType: MType): Function {
    const key = parameters.map(p => p.id).join(',') + ':' + returnType.id;
    const cached = this.map.get(key);
    if (cached) {
      return cached;
    }
    const func = new Function(parameters, returnType);
    this.map.set(key, func);
    return func;
  }

  readonly parameters: MType[];
  readonly returnType: MType;
  private constructor(parameters: MType[], returnType: MType) {
    super();
    this.parameters = parameters;
    this.returnType = returnType;
  }
  isSubclassOf(type: MType): boolean {
    return this === type || UntypedFunction.isSubclassOf(type);
  }

  toString() {
    if (this.parameters.length) {
      return `function[${this.parameters.join(',')},${this.returnType}]`;
    }
    return `function[${this.returnType}]`;
  }
}

export class UserDefined extends MType {
  private static readonly map: Map<MSymbol, UserDefined> = new Map();
  static of (symbol: MSymbol): UserDefined {
    const cached = this.map.get(symbol);
    if (cached) {
      return cached;
    }
    const userDefined = new UserDefined(symbol);
    this.map.set(symbol, userDefined);
    return userDefined;
  }

  readonly symbol: MSymbol;
  private constructor(symbol: MSymbol) {
    super();
    this.symbol = symbol;
  }
  isSubclassOf(type: MType): boolean {
    // TODO: base classes
    return this === type || Any.isSubclassOf(type);
  }
  toString() {
    // TODO: qualify the name
    return this.symbol.name;
  }
}
