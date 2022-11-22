import { MSymbol } from "./symbol";

export abstract class MType {
  private static nextID: number = 0;

  readonly id: number;

  constructor() {
    this.id = MType.nextID++;
  }

  /** i.e. is `this` a subtype of `type` */
  abstract isAssignableTo(other: MType): boolean;

  /**
   * i.e. given two types, what is the most specific base type
   * shared between two types?
   */
  abstract closestCommonType(other: MType): MType;

  abstract toString(): string;
}

/** aka "Top Type" */
class AnyType extends MType {
  toString() {
    return 'any';
  }
  isAssignableTo(other: MType): boolean {
    return this === other;
  }
  closestCommonType(other: MType): MType {
    return this;
  }
}

export const Any = new AnyType();

/** aka 'bottom' or 'never' type */
class NoReturnType extends MType {
  toString() {
    return 'noreturn';
  }
  isAssignableTo(other: MType): boolean {
    return true;
  }
  closestCommonType(other: MType): MType {
    return other;
  }
}

export const NoReturn = new NoReturnType();

export class BuiltinPrimitive extends MType {
  readonly parent: MType;
  readonly name: string;

  static Nil = new BuiltinPrimitive('nil', Any);
  static Bool = new BuiltinPrimitive('bool', Any);
  static Float = new BuiltinPrimitive('float', Any);
  static Int = new BuiltinPrimitive('int', BuiltinPrimitive.Float);
  static String = new BuiltinPrimitive('string', Any);
  static UntypedList = new BuiltinPrimitive('list', Any);
  static UntypedDict = new BuiltinPrimitive('dict', Any);
  static UntypedFunction = new BuiltinPrimitive('function', Any);
  static UntypedOptional = new BuiltinPrimitive('optional', Any);

  private constructor(name: string, parent: MType) {
    super();
    this.parent = parent;
    this.name = name;
  }

  isAssignableTo(other: MType): boolean {
    return this === other || this.parent.isAssignableTo(other);
  }

  closestCommonType(other: MType): MType {
    return this === other ? this : this.parent.closestCommonType(other);
  }

  toString() {
    return this.name;
  }
}

export const Nil = BuiltinPrimitive.Nil;
export const Bool = BuiltinPrimitive.Bool;
export const Float = BuiltinPrimitive.Float;
export const Int = BuiltinPrimitive.Int;
export const String = BuiltinPrimitive.String;
export const UntypedList = BuiltinPrimitive.UntypedList;
export const UntypedDict = BuiltinPrimitive.UntypedDict;
export const UntypedFunction = BuiltinPrimitive.UntypedFunction;
export const UntypedOptional = BuiltinPrimitive.UntypedOptional;

export class List extends MType {
  private static readonly map: Map<MType, List> = new Map();

  static of(itemType: MType) {
    const cached = this.map.get(itemType);
    if (cached) {
      return cached;
    }
    const newList = new List(itemType);
    this.map.set(itemType, newList);
    return newList;
  }

  readonly itemType: MType;
  private constructor(itemType: MType) {
    super();
    this.itemType = itemType;
  }

  isAssignableTo(other: MType): boolean {
    return this === other || UntypedList.isAssignableTo(other);
  }

  closestCommonType(other: MType): MType {
    return this === other ? this : UntypedList.closestCommonType(other);
  }

  toString() {
    return `list[${this.itemType}]`;
  }
}

export class Dict extends MType {
  private static readonly map: Map<string, Dict> = new Map();

  static of(keyType: MType, valueType: MType): Dict {
    const key = keyType.id + ':' + valueType.id;
    const cached = this.map.get(key);
    if (cached) {
      return cached;
    }
    const newDict = new Dict(keyType, valueType);
    this.map.set(key, newDict);
    return newDict;
  }

  readonly keyType: MType;
  readonly valueType: MType
  private constructor(keyType: MType, valueType: MType) {
    super();
    this.keyType = keyType;
    this.valueType = valueType;
  }

  isAssignableTo(other: MType): boolean {
    return this === other || UntypedDict.isAssignableTo(other);
  }

  closestCommonType(other: MType): MType {
    return this === other ? this : UntypedDict.closestCommonType(other);
  }

  toString() {
    return `dict[${this.keyType},${this.valueType}]`;
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

  isAssignableTo(other: MType): boolean {
    return this === other || UntypedOptional.isAssignableTo(other);
  }

  closestCommonType(other: MType): MType {
    return this === other ? this : UntypedOptional.closestCommonType(other);
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

  isAssignableTo(other: MType): boolean {
    return this === other || UntypedFunction.isAssignableTo(other);
  }

  closestCommonType(other: MType): MType {
    return this === other ? this : UntypedFunction.closestCommonType(other);
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

  isAssignableTo(other: MType): boolean {
    // TODO: consider base classes
    return this === other || Any.isAssignableTo(other);
  }

  closestCommonType(other: MType): MType {
    // TODO: consider base classes
    return this === other ? this : Any;
  }

  toString() {
    // TODO: qualify the name
    return this.symbol.name;
  }
}
