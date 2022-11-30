import { QualifiedIdentifier } from "./ast";
import { MScope } from "./scope";
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

  getFieldSymbol(fieldName: string): MSymbol | null {
    return null;
  }

  getMethodSymbol(methodName: string): MSymbol | null {
    return AnyMap.get(methodName) || null;
  }

  /**
   * If this type is iterated over with a for loop,
   * what would the loop variable's type be?
   */
  getForInItemType(): MType | null {
    return null;
  }

  /**
   * When you put '.' after an expression of this type, return the scope
   * that can be used to find all possible candidates for auto-complete.
   */
  getCompletionScope(): MScope | null {
    return null;
  }

  abstract toString(): string;

  format(isFinal: boolean, variableName: string): string {
    return `${isFinal ? 'final' : 'var'} ${variableName} ${this}`;
  }
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
  getFieldSymbol(fieldName: string): MSymbol | null {
    return AnyUnknownSymbol;
  }
  getMethodSymbol(methodName: string): MSymbol | null {
    return AnyUnknownSymbol;
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
  getFieldSymbol(fieldName: string): MSymbol | null {
    return null;
  }
  getMethodSymbol(methodName: string): MSymbol | null {
    return null;
  }
}

export const NoReturn = new NoReturnType();

export class BuiltinPrimitive extends MType {
  readonly parent: MType;
  readonly name: string;

  static Nil = new BuiltinPrimitive('nil', Any);
  static Bool = new BuiltinPrimitive('bool', Any);
  static Number = new BuiltinPrimitive('number', Any);
  static String = new BuiltinPrimitive('string', Any);
  static UntypedModule = new BuiltinPrimitive('module', Any);
  static UntypedList = new BuiltinPrimitive('list', Any);
  static UntypedDict = new BuiltinPrimitive('dict', Any);
  static UntypedFunction = new BuiltinPrimitive('function', Any);
  static UntypedClass = new BuiltinPrimitive('class', Any);

  private constructor(name: string, parent: MType) {
    super();
    this.parent = parent;
    this.name = name;
  }

  isAssignableTo(other: MType): boolean {
    if (this === Nil && other instanceof Optional) {
      return true;
    }
    if (other instanceof Optional && this === other.itemType) {
      return true;
    }
    return this === other || this.parent.isAssignableTo(other);
  }

  closestCommonType(other: MType): MType {
    return other.isAssignableTo(this) ? this : this.parent.closestCommonType(other);
  }

  getFieldSymbol(fieldName: string): MSymbol | null {
    switch (this) {
      case UntypedDict:
        return AnyUnknownSymbol;
    }
    return null;
  }

  getMethodSymbol(methodName: string): MSymbol | null {
    const map = BuiltinMap.get(this);
    if (!map) {
      return null;
    }
    return map.get(methodName) || super.getMethodSymbol(methodName);
  }

  getForInItemType(): MType | null {
    switch (this) {
      case UntypedDict:
      case UntypedList:
      case UntypedFunction:
        return Any;
    }
    return null;
  }

  toString() {
    return this.name;
  }
}

export const Nil = BuiltinPrimitive.Nil;
export const Bool = BuiltinPrimitive.Bool;
export const Number = BuiltinPrimitive.Number;
export const String = BuiltinPrimitive.String;
export const UntypedModule = BuiltinPrimitive.UntypedModule;
export const UntypedList = BuiltinPrimitive.UntypedList;
export const UntypedDict = BuiltinPrimitive.UntypedDict;
export const UntypedFunction = BuiltinPrimitive.UntypedFunction;
export const UntypedClass = BuiltinPrimitive.UntypedClass;

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
  private readonly methodMap: Map<string, MSymbol>;
  private constructor(itemType: MType) {
    super();
    this.itemType = itemType;
    this.methodMap = makeListMethodMap(this);
  }

  isAssignableTo(other: MType): boolean {
    return this === other || UntypedList.isAssignableTo(other);
  }

  closestCommonType(other: MType): MType {
    return this === other ? this : UntypedList.closestCommonType(other);
  }

  getMethodSymbol(methodName: string): MSymbol | null {
    return this.methodMap.get(methodName) || super.getMethodSymbol(methodName);
  }

  getForInItemType(): MType | null {
    return this.itemType;
  }

  getCompletionScope(): MScope | null {
    return new MScope(null, this.methodMap);
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
  readonly valueType: MType;
  private readonly methodMap: Map<string, MSymbol>;
  private readonly valueSymbol: MSymbol | null;
  private constructor(keyType: MType, valueType: MType) {
    super();
    this.keyType = keyType;
    this.valueType = valueType;
    this.methodMap = makeDictMethodMap(this);
    if (keyType === String) {
      const valueSymbol = new MSymbol('_', null, false, null);
      valueSymbol.valueType = valueType;
      this.valueSymbol = valueSymbol;
    } else {
      this.valueSymbol = null;
    }
  }

  isAssignableTo(other: MType): boolean {
    return this === other || UntypedDict.isAssignableTo(other);
  }

  closestCommonType(other: MType): MType {
    return this === other ? this : UntypedDict.closestCommonType(other);
  }

  getFieldSymbol(fieldName: string): MSymbol | null {
    return this.valueSymbol;
  }

  getMethodSymbol(methodName: string): MSymbol | null {
    return this.methodMap.get(methodName) || super.getMethodSymbol(methodName);
  }

  getForInItemType(): MType | null {
    return this.keyType;
  }

  toString() {
    return `dict[${this.keyType},${this.valueType}]`;
  }
}

/**
 * Union type between the given item type and nil
 */
export class Optional extends MType {
  private static readonly map: Map<MType, Optional> = new Map();

  static of(itemType: MType): MType {
    if (itemType instanceof Optional) {
      return itemType;
    }
    if (itemType instanceof Iterate) {
      return Iterate.of(Optional.of(itemType.itemType));
    }
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

  /** The optional is truly covariant, unlike List or Dict */
  isAssignableTo(other: MType): boolean {
    return this === other || (
      other instanceof Optional && this.itemType.isAssignableTo(other.itemType)) ||
      Any.isAssignableTo(other);
  }

  closestCommonType(other: MType): MType {
    if (this === other) {
      return this;
    }
    if (other === Nil) {
      return this;
    }
    if (other instanceof Optional) {
      return Optional.of(this.itemType.closestCommonType(other.itemType));
    }
    return Any;
  }

  toString() {
    if (this.itemType instanceof Function) {
      return `(${this.itemType})?`
    }
    return this.itemType + '?';
  }
}

export class StopIterationType extends MType {
  static Instance = new StopIterationType();

  private constructor() {
    super();
  }

  isAssignableTo(other: MType): boolean {
    if (this === other) {
      return true;
    }
    if (other instanceof Iterate) {
      return true;
    }
    return Any.isAssignableTo(other);
  }

  closestCommonType(other: MType): MType {
    if (this === other) {
      return this;
    }
    if (other instanceof Iterate) {
      return other;
    }
    return Any;
  }

  toString(): string {
    return 'StopIteration';
  }
}

export const StopIteration = StopIterationType.Instance;

/**
 * Union type between the given item type and StopIteration
 */
export class Iterate extends MType {
  private static readonly map: Map<MType, Iterate> = new Map();

  static of(itemType: MType) {
    if (itemType instanceof Iterate) {
      return itemType;
    }
    const cached = this.map.get(itemType);
    if (cached) {
      return cached;
    }
    const optional = new Iterate(itemType);
    this.map.set(itemType, optional);
    return optional;
  }

  readonly itemType: MType;
  private constructor(itemType: MType) {
    super();
    this.itemType = itemType;
  }

  isAssignableTo(other: MType): boolean {
    return this === other || (
      other instanceof Iterate && this.itemType.isAssignableTo(other.itemType)) ||
      Any.isAssignableTo(other);
  }

  closestCommonType(other: MType): MType {
    if (this === other || other === StopIteration) {
      return this;
    }
    if (other instanceof Iterate) {
      return Iterate.of(this.itemType.closestCommonType(other.itemType));
    }
    return Any;
  }

  getForInItemType(): MType | null {
    return this.itemType;
  }

  toString(): string {
    return `iterate[${this.itemType}]`;
  }
}

export class Function extends MType {
  private static readonly map: Map<string, Function> = new Map();

  static of(parameters: MType[], optionalCount: number, returnType: MType): Function {
    const key = parameters.map(p => p.id).join(',') + ':' + optionalCount + ':' + returnType.id;
    const cached = this.map.get(key);
    if (cached) {
      return cached;
    }
    const func = new Function(parameters, optionalCount, returnType);
    this.map.set(key, func);
    return func;
  }

  readonly parameters: MType[];
  readonly optionalCount: number;
  readonly returnType: MType;
  private constructor(parameters: MType[], optionalCount: number, returnType: MType) {
    super();
    this.parameters = parameters;
    this.optionalCount = optionalCount;
    this.returnType = returnType;
  }

  isAssignableTo(other: MType): boolean {
    return this === other || UntypedFunction.isAssignableTo(other);
  }

  closestCommonType(other: MType): MType {
    return this === other ? this : UntypedFunction.closestCommonType(other);
  }

  toString() {
    return `(${this.parameters.join(',')})${this.returnType}`;
  }
}

export class Class extends MType {
  private static readonly map: Map<MSymbol, Class> = new Map();
  static of(symbol: MSymbol): Class {
    const cached = this.map.get(symbol);
    if (cached) {
      return cached;
    }
    const klass = new Class(symbol);
    this.map.set(symbol, klass);
    return klass;
  }

  readonly symbol: MSymbol;
  private constructor(symbol: MSymbol) {
    super();
    this.symbol = symbol;
  }

  isAssignableTo(other: MType): boolean {
    return this === other || UntypedClass.isAssignableTo(other);
  }

  closestCommonType(other: MType): MType {
    return this === other ? this : UntypedClass.closestCommonType(other);
  }

  toString() {
    // TODO: qualify the name
    return `class[${this.symbol.name}]`;
  }

  format(isFinal: boolean, variableName: string): string {
    return `class ${variableName}`;
  }
}

export class Instance extends MType {
  private static readonly map: Map<MSymbol, Instance> = new Map();
  static of(symbol: MSymbol): Instance {
    const cached = this.map.get(symbol);
    if (cached) {
      return cached;
    }
    const instance = new Instance(symbol);
    this.map.set(symbol, instance);
    return instance;
  }

  readonly symbol: MSymbol;
  private constructor(symbol: MSymbol) {
    super();
    this.symbol = symbol;
  }

  isAssignableTo(other: MType): boolean {
    // TODO: consider base classes
    return this === other || other === Optional.of(this) || Any.isAssignableTo(other);
  }

  closestCommonType(other: MType): MType {
    // TODO: consider base classes
    return this === other ? this : Any;
  }

  getFieldSymbol(fieldName: string): MSymbol | null {
    // TODO: consider base classes
    return this.symbol.members.get(fieldName) || null;
  }

  getMethodSymbol(methodName: string): MSymbol | null {
    // TODO: consider base classes
    return this.symbol.members.get(methodName) || super.getMethodSymbol(methodName);
  }

  getCompletionScope(): MScope | null {
    return new MScope(null, this.symbol.members);
  }

  toString() {
    // TODO: qualify the name
    return this.symbol.name;
  }
}

export class Module extends MType {
  private static readonly map: Map<MSymbol, Module> = new Map();
  static of(symbol: MSymbol, path: QualifiedIdentifier): Module {
    const cached = this.map.get(symbol);
    if (cached) {
      return cached;
    }
    const module = new Module(symbol, path);
    this.map.set(symbol, module);
    return module;
  }

  readonly symbol: MSymbol;
  readonly path: QualifiedIdentifier;
  constructor(symbol: MSymbol, path: QualifiedIdentifier) {
    super();
    this.symbol = symbol;
    this.path = path;
  }

  isAssignableTo(other: MType): boolean {
    return this === other;
  }

  closestCommonType(other: MType): MType {
    return this === other ? this : UntypedModule.closestCommonType(other);
  }

  getFieldSymbol(fieldName: string): MSymbol | null {
    return this.symbol.members.get(fieldName) || null;
  }

  getMethodSymbol(methodName: string): MSymbol | null {
    return this.symbol.members.get(methodName) || null;
  }

  getCompletionScope(): MScope | null {
    return new MScope(null, this.symbol.members);
  }

  toString(): string {
    return `module[${this.symbol.name}]`
  }

  format(isFinal: boolean, variableName: string): string {
    const path = this.path.toString();
    if (path === variableName) {
      return `import ${path}`;
    }
    return `import ${this.path} as ${variableName}`;
  }
}

export class FunctionSignature {
  readonly parameters: [string, MType][];
  readonly optionalParameters: [string, MType][];
  readonly returnType: MType;
  constructor(
      parameters: [string, MType][],
      optionalParameters: [string, MType][],
      returnType: MType) {
    this.parameters = parameters;
    this.optionalParameters = optionalParameters;
    this.returnType = returnType;
  }
  toType(): MType {
    return Function.of(
      this.parameters.concat(this.optionalParameters).map(p => p[1]),
      this.optionalParameters.length,
      this.returnType);
  }
  format(functionName: string): string {
    const reqargs = this.parameters.map(p => `${p[0]} ${p[1]}`).join(', ');
    const optargs = this.optionalParameters.map(p => `${p[0]} ${p[1]} =...`).join(', ');
    const args =
      reqargs.length === 0 ? optargs :
      optargs.length === 0 ? reqargs :
      reqargs + ', ' + optargs;
    return `def ${functionName}(${args}) ${this.returnType}`;
  }
}


function mkmethod(
    name: string,
    typ: Function,
    documentation: string | null = null): MSymbol {
  const symbol = new MSymbol(name, null, true, null);
  symbol.valueType = typ;
  symbol.documentation = documentation;
  return symbol;
}

function mkmap(symbols: MSymbol[]): Map<string, MSymbol> {
  return new Map(symbols.map(s => [s.name, s]));
}

export const AnyUnknownSymbol = new MSymbol('_', null, true, null);
AnyUnknownSymbol.valueType = Any;
AnyUnknownSymbol.typeType = Any;

export const AnyMap = mkmap([
  mkmethod('__is__', Function.of([Any], 0, Bool)),
  mkmethod('__isnot__', Function.of([Any], 0, Bool)),
  mkmethod('__eq__', Function.of([Any], 0, Bool)),
  mkmethod('__ne__', Function.of([Any], 0, Bool)),
  mkmethod('__lt__', Function.of([Any], 0, Bool)),
  mkmethod('__le__', Function.of([Any], 0, Bool)),
  mkmethod('__gt__', Function.of([Any], 0, Bool)),
  mkmethod('__ge__', Function.of([Any], 0, Bool)),
]);

export const BuiltinMap = new Map<BuiltinPrimitive, Map<string, MSymbol>>([
  [Number, mkmap([
    mkmethod('__add__', Function.of([Number], 0, Number)),
    mkmethod('__sub__', Function.of([Number], 0, Number)),
    mkmethod('__mul__', Function.of([Number], 0, Number)),
    mkmethod('__mod__', Function.of([Number], 0, Number)),
    mkmethod('__div__', Function.of([Number], 0, Number)),
    mkmethod('__floordiv__', Function.of([Number], 0, Number)),
    mkmethod('__neg__', Function.of([], 0, Number)),
    mkmethod('__and__', Function.of([Number], 0, Number)),
    mkmethod('__xor__', Function.of([Number], 0, Number)),
    mkmethod('__or__', Function.of([Number], 0, Number)),
  ])],
  [String, mkmap([
    mkmethod('__add__', Function.of([String], 0, String)),
    mkmethod('__mul__', Function.of([Number], 0, String)),
    mkmethod('__getitem__', Function.of([Number], 0, String)),
    mkmethod('__mod__', Function.of([UntypedList], 0, String)),
    mkmethod('__slice__', Function.of([Optional.of(Number), Optional.of(Number)], 2, String)),
    mkmethod('strip', Function.of([String], 1, String)),
    mkmethod('replace', Function.of([String, String], 0, String)),
    mkmethod('join', Function.of([List.of(String)], 0, String)),
  ])],
]);

export function makeListMethodMap(self: List): Map<string, MSymbol> {
  return mkmap([
    mkmethod('__mul__', Function.of([Number], 0, self)),
    mkmethod('__getitem__', Function.of([Number], 0, self.itemType)),
    mkmethod('__setitem__', Function.of([Number, self.itemType], 0, Nil)),
    mkmethod('__contains__', Function.of([self.itemType], 0, Bool)),
    mkmethod('__notcontains__', Function.of([self.itemType], 0, Bool)),
    mkmethod('append', Function.of([self.itemType], 0, Nil)),
    mkmethod('pop', Function.of([], 0, self.itemType)),
    mkmethod('__iter__', Function.of([], 0, Function.of([], 0, Iterate.of(self.itemType)))),
  ]);
}

export function makeDictMethodMap(self: Dict): Map<string, MSymbol> {
  return mkmap([
    mkmethod('__getitem__', Function.of([self.keyType], 0, self.valueType)),
    mkmethod('__setitem__', Function.of([self.keyType, self.valueType], 0, Nil)),
    mkmethod('__contains__', Function.of([self.keyType], 0, Bool)),
    mkmethod('__notcontains__', Function.of([self.keyType], 0, Bool)),
    mkmethod('__iter__', Function.of([], 0, Function.of([], 0, Iterate.of(self.keyType)))),
    mkmethod('delete', Function.of([self.keyType], 0, Bool)),
    mkmethod('rget', Function.of([self.valueType, self.keyType], 1, self.keyType)),
  ]);
}
