import { QualifiedIdentifier } from "./ast";
import { MScope } from "./scope";
import { MSymbol } from "./symbol";

export abstract class MType {
  private static nextID: number = 0;

  readonly id: number;

  constructor() {
    this.id = MType.nextID++;
  }

  isAssignableTo(other: MType): boolean {
    if (this === other || other === Any) {
      return true;
    }
    if (!(this instanceof Optional) &&
        other instanceof Optional &&
        this.isAssignableTo(other.itemType)) {
      return true;
    }
    return this._isAssignableTo(other);
  }

  /** i.e. is `this` a subtype of `type` */
  protected abstract _isAssignableTo(other: MType): boolean;

  /**
   * i.e. given two types, what is the most specific base type
   * shared between two types?
   */
  closestCommonType(other: MType): MType {
    const self: MType = this;
    if (self === other) {
      return self;
    }
    if (self === Any || other === Any) {
      return Any;
    }
    if (self === Never) {
      return other;
    }
    if (other === Never) {
      return self;
    }
    if (self instanceof Optional && self.itemType === other) {
      return self;
    }
    if (other instanceof Optional && other.itemType === self) {
      return other;
    }
    return self._closestCommonType(other);
  }

  protected abstract _closestCommonType(other: MType): MType;

  getFieldSymbol(fieldName: string): MSymbol | null {
    return null;
  }

  getMethodSymbol(methodName: string): MSymbol | null {
    return AnyMap.get(methodName) || null;
  }

  /**
   * Return a subset of this that has a possibility of being truthy.
   * (i.e. nil, StopIteration and other falsy values will be filtered out)
   */
  filterTruthy(): MType {
    return this;
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
    return 'Any';
  }
  _isAssignableTo(other: MType): boolean {
    return this === other;
  }
  _closestCommonType(other: MType): MType {
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

/** aka 'bottom' or 'noreturn' type */
class NeverType extends MType {
  toString() {
    return 'Never';
  }
  _isAssignableTo(other: MType): boolean {
    return true;
  }
  _closestCommonType(other: MType): MType {
    return other;
  }
  getFieldSymbol(fieldName: string): MSymbol | null {
    return null;
  }
  getMethodSymbol(methodName: string): MSymbol | null {
    return null;
  }
}

export const Never = new NeverType();

export class BuiltinPrimitive extends MType {
  readonly parent: MType;
  readonly name: string;

  static Nil = new BuiltinPrimitive('nil', Any);
  static Bool = new BuiltinPrimitive('Bool', Any);
  static Number = new BuiltinPrimitive('Number', Any);
  static String = new BuiltinPrimitive('String', Any);
  static UntypedModule = new BuiltinPrimitive('Module', Any);
  static UntypedList = new BuiltinPrimitive('List', Any);
  static UntypedTuple = new BuiltinPrimitive('Tuple', Any);
  static UntypedDict = new BuiltinPrimitive('Dict', Any);
  static UntypedFrozenDict = new BuiltinPrimitive('Dict', Any);
  static UntypedOptional = new BuiltinPrimitive('Optional', Any);
  static UntypedIterable = new BuiltinPrimitive('Iterable', Any);
  static UntypedIterate = new BuiltinPrimitive('Iteration', Any);
  static UntypedFunction = new BuiltinPrimitive('Function', Any);
  static UntypedClass = new BuiltinPrimitive('Class', Any);

  private constructor(name: string, parent: MType) {
    super();
    this.parent = parent;
    this.name = name;
  }

  _isAssignableTo(other: MType): boolean {
    if (other instanceof Optional) {
      return this === Nil || this.isAssignableTo(other.itemType);
    }
    return this === other || this.parent.isAssignableTo(other);
  }

  _closestCommonType(other: MType): MType {
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
export const UntypedTuple = BuiltinPrimitive.UntypedTuple;
export const UntypedDict = BuiltinPrimitive.UntypedDict;
export const UntypedFrozenDict = BuiltinPrimitive.UntypedFrozenDict;
export const UntypedOptional = BuiltinPrimitive.UntypedOptional;
export const UntypedIterable = BuiltinPrimitive.UntypedIterable;
export const UntypedIterate = BuiltinPrimitive.UntypedIterate;
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

  _isAssignableTo(other: MType): boolean {
    if (this === other) {
      return true;
    }
    if (other instanceof Iterable) {
      return this.itemType.isAssignableTo(other.itemType);
    }
    return UntypedList.isAssignableTo(other);
  }

  _closestCommonType(other: MType): MType {
    return this === other ? this : UntypedList.closestCommonType(other);
  }

  getMethodSymbol(methodName: string): MSymbol | null {
    return this.methodMap.get(methodName) || super.getMethodSymbol(methodName);
  }

  getForInItemType(): MType | null {
    return this.itemType;
  }

  getCompletionScope(): MScope | null {
    return MScope.new(null, this.methodMap);
  }

  toString() {
    return `List[${this.itemType}]`;
  }
}

export class Tuple extends MType {
  private static readonly map: Map<MType, Tuple> = new Map();

  static of(itemType: MType) {
    const cached = this.map.get(itemType);
    if (cached) {
      return cached;
    }
    const newTuple = new Tuple(itemType);
    this.map.set(itemType, newTuple);
    return newTuple;
  }

  readonly itemType: MType;
  private readonly methodMap: Map<string, MSymbol>;
  private constructor(itemType: MType) {
    super();
    this.itemType = itemType;
    this.methodMap = makeTupleMethodMap(this);
  }

  _isAssignableTo(other: MType): boolean {
    if (this === other) {
      return true;
    }
    if (other instanceof Iterable) {
      return this.itemType.isAssignableTo(other.itemType);
    }
    return UntypedTuple.isAssignableTo(other);
  }

  _closestCommonType(other: MType): MType {
    return this === other ? this : UntypedTuple.closestCommonType(other);
  }

  getMethodSymbol(methodName: string): MSymbol | null {
    return this.methodMap.get(methodName) || super.getMethodSymbol(methodName);
  }

  getForInItemType(): MType | null {
    return this.itemType;
  }

  getCompletionScope(): MScope | null {
    return MScope.new(null, this.methodMap);
  }

  toString() {
    return `Tuple[${this.itemType}]`;
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

  _isAssignableTo(other: MType): boolean {
    if (this === other) {
      return true;
    }
    if (other instanceof Iterable) {
      return this.keyType.isAssignableTo(other.itemType);
    }
    return UntypedDict.isAssignableTo(other);
  }

  _closestCommonType(other: MType): MType {
    return this === other ? this : UntypedDict.closestCommonType(other);
  }

  getFieldSymbol(fieldName: string): MSymbol | null {
    return this.valueSymbol;
  }

  getMethodSymbol(methodName: string): MSymbol | null {
    return this.methodMap.get(methodName) || super.getMethodSymbol(methodName);
  }

  getCompletionScope(): MScope | null {
    return MScope.new(null, this.methodMap);
  }

  getForInItemType(): MType | null {
    return this.keyType;
  }

  toString() {
    return `Dict[${this.keyType},${this.valueType}]`;
  }
}

export class FrozenDict extends MType {
  private static readonly map: Map<string, FrozenDict> = new Map();

  static of(keyType: MType, valueType: MType): FrozenDict {
    const key = keyType.id + ':' + valueType.id;
    const cached = this.map.get(key);
    if (cached) {
      return cached;
    }
    const newFrozenDict = new FrozenDict(keyType, valueType);
    this.map.set(key, newFrozenDict);
    return newFrozenDict;
  }

  readonly keyType: MType;
  readonly valueType: MType;
  private readonly methodMap: Map<string, MSymbol>;
  private readonly valueSymbol: MSymbol | null;
  private constructor(keyType: MType, valueType: MType) {
    super();
    this.keyType = keyType;
    this.valueType = valueType;
    this.methodMap = makeFrozenDictMethodMap(this);
    if (keyType === String) {
      const valueSymbol = new MSymbol('_', null, false, null);
      valueSymbol.valueType = valueType;
      this.valueSymbol = valueSymbol;
    } else {
      this.valueSymbol = null;
    }
  }

  _isAssignableTo(other: MType): boolean {
    if (this === other) {
      return true;
    }
    if (other instanceof Iterable) {
      return this.keyType.isAssignableTo(other.itemType);
    }
    return UntypedFrozenDict.isAssignableTo(other);
  }

  _closestCommonType(other: MType): MType {
    return this === other ? this : UntypedFrozenDict.closestCommonType(other);
  }

  getFieldSymbol(fieldName: string): MSymbol | null {
    return this.valueSymbol;
  }

  getMethodSymbol(methodName: string): MSymbol | null {
    return this.methodMap.get(methodName) || super.getMethodSymbol(methodName);
  }

  getCompletionScope(): MScope | null {
    return MScope.new(null, this.methodMap);
  }

  getForInItemType(): MType | null {
    return this.keyType;
  }

  toString() {
    return `FrozenDict[${this.keyType},${this.valueType}]`;
  }
}

export class KeyedFrozenDict extends MType {
  static of(keySymbols: MSymbol[], valueType: MType): KeyedFrozenDict {
    return new KeyedFrozenDict(keySymbols, valueType);
  }

  readonly baseType: FrozenDict
  readonly keySymbols: MSymbol[]
  private readonly keySymbolMap: Map<string, MSymbol>
  private readonly wordKeySymbolMap: Map<string, MSymbol>
  private constructor(keySymbols: MSymbol[], valueType: MType) {
    super();
    this.baseType = FrozenDict.of(String, valueType);
    this.keySymbols = keySymbols;
    this.keySymbolMap = new Map(keySymbols.map(s => [s.name, s]));
    this.wordKeySymbolMap = new Map(
      keySymbols.filter(s => /^[A-Za-z_][A-Za-z0-9_]*$/.test(s.name)).map(s => [s.name, s]));
  }

  _isAssignableTo(other: MType): boolean {
    return other === this || this.baseType.isAssignableTo(other);
  }

  _closestCommonType(other: MType): MType {
    return this.baseType.closestCommonType(other);
  }

  getFieldSymbol(fieldName: string): MSymbol | null {
    return this.keySymbolMap.get(fieldName) || null;
  }

  getMethodSymbol(methodName: string): MSymbol | null {
    return this.baseType.getMethodSymbol(methodName);
  }

  getCompletionScope(): MScope | null {
    return MScope.new(null, this.wordKeySymbolMap);
  }

  getForInItemType(): MType | null {
    return this.baseType.getForInItemType();
  }

  toString(): string {
    return this.baseType.toString();
  }
}

export class TypeParameter extends MType {
  private static readonly map: Map<MSymbol, TypeParameter> = new Map();

  static of(symbol: MSymbol, baseType: MType): TypeParameter {
    const cached = this.map.get(symbol);
    if (cached) {
      throw new Error(`Cached TypeParameter already found for ${symbol.name}`);
    }
    const tp = new this(symbol, baseType);
    this.map.set(symbol, tp);
    return tp;
  }

  readonly symbol: MSymbol;
  readonly baseType: MType;
  constructor(symbol: MSymbol, baseType: MType) {
    super();
    this.symbol = symbol;
    this.baseType = baseType;
  }

  _isAssignableTo(other: MType): boolean {
    return other === this || this.baseType.isAssignableTo(other);
  }

  _closestCommonType(other: MType): MType {
    return this.baseType.closestCommonType(other);
  }

  toString(): string {
    return this.symbol.name;
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
    if (itemType instanceof Iteration) {
      return Iteration.of(Optional.of(itemType.itemType));
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
  _isAssignableTo(other: MType): boolean {
    return this === other || (
      other instanceof Optional && this.itemType.isAssignableTo(other.itemType)) ||
      UntypedOptional.isAssignableTo(other);
  }

  filterTruthy(): MType {
    return this.itemType;
  }

  _closestCommonType(other: MType): MType {
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

/**
 * Union type between the given item type and nil
 */
export class Iterable extends MType {
  private static readonly map: Map<MType, Iterable> = new Map();

  static of(itemType: MType): MType {
    if (itemType instanceof Iterable) {
      return itemType;
    }
    const cached = this.map.get(itemType);
    if (cached) {
      return cached;
    }
    const optional = new Iterable(itemType);
    this.map.set(itemType, optional);
    return optional;
  }

  readonly itemType: MType;
  private constructor(itemType: MType) {
    super();
    this.itemType = itemType;
  }

  /** The optional is truly covariant, unlike List or Dict */
  _isAssignableTo(other: MType): boolean {
    return this === other || (
      other instanceof Iterable && this.itemType.isAssignableTo(other.itemType)) ||
      Any.isAssignableTo(other);
  }

  filterTruthy(): MType {
    return this.itemType;
  }

  _closestCommonType(other: MType): MType {
    if (this === other) {
      return this;
    }
    if (other === Nil) {
      return this;
    }
    if (other instanceof Iterable) {
      return Iterable.of(this.itemType.closestCommonType(other.itemType));
    }
    return Any;
  }

  getForInItemType(): MType | null {
    return this.itemType;
  }

  toString() {
    return `Iterable[${this.itemType}]`;
  }
}

export class StopIterationType extends MType {
  static Instance = new StopIterationType();

  private constructor() {
    super();
  }

  _isAssignableTo(other: MType): boolean {
    if (this === other) {
      return true;
    }
    if (other instanceof Iteration) {
      return true;
    }
    return Any.isAssignableTo(other);
  }

  _closestCommonType(other: MType): MType {
    if (this === other) {
      return this;
    }
    if (other instanceof Iteration) {
      return other;
    }
    return Any;
  }

  filterTruthy(): MType {
    return Never;
  }

  toString(): string {
    return 'StopIteration';
  }
}

export const StopIteration = StopIterationType.Instance;

/**
 * Union type between the given item type and StopIteration
 */
export class Iteration extends MType {
  private static readonly map: Map<MType, Iteration> = new Map();

  static of(itemType: MType) {
    if (itemType instanceof Iteration) {
      return itemType;
    }
    const cached = this.map.get(itemType);
    if (cached) {
      return cached;
    }
    const optional = new Iteration(itemType);
    this.map.set(itemType, optional);
    return optional;
  }

  readonly itemType: MType;
  private constructor(itemType: MType) {
    super();
    this.itemType = itemType;
  }

  _isAssignableTo(other: MType): boolean {
    return this === other || (
      other instanceof Iteration && this.itemType.isAssignableTo(other.itemType)) ||
      Any.isAssignableTo(other);
  }

  _closestCommonType(other: MType): MType {
    if (this === other || other === StopIteration) {
      return this;
    }
    if (other instanceof Iteration) {
      return Iteration.of(this.itemType.closestCommonType(other.itemType));
    }
    return Any;
  }

  filterTruthy(): MType {
    return this.itemType;
  }

  getForInItemType(): MType | null {
    return this.itemType;
  }

  toString(): string {
    return `Iteration[${this.itemType}]`;
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

  _isAssignableTo(other: MType): boolean {
    if (this === other) {
      return true;
    }
    if (other instanceof Function) {
      return other.parameters.length <= this.parameters.length &&
        other.parameters.length - other.optionalCount >=
          this.parameters.length - this.optionalCount &&
        other.parameters.every((p, i) => p.isAssignableTo(this.parameters[i])) &&
        this.returnType.isAssignableTo(other.returnType) &&
        this.optionalCount >= other.optionalCount;
    }
    return UntypedFunction.isAssignableTo(other);
  }

  _closestCommonType(other: MType): MType {
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

  _isAssignableTo(other: MType): boolean {
    if (this === other) {
      return true;
    }
    const initMethodSymbol = this.symbol.members.get('__init__');
    const signature = initMethodSymbol && initMethodSymbol.functionSignature ?
      initMethodSymbol.functionSignature : null;
    const functionType = Function.of(
      signature ?
        signature.parameters.concat(signature.optionalParameters).map(p => p[1]) :
        [],
      signature ? signature.optionalParameters.length : 0,
      Instance.of(this.symbol));
    if (functionType.isAssignableTo(other)) {
      return true;
    }
    return UntypedClass.isAssignableTo(other);
  }

  _closestCommonType(other: MType): MType {
    return this === other ? this : UntypedClass.closestCommonType(other);
  }

  getCompletionScope(): MScope | null {
    return MScope.new(null, this.symbol.staticMembers);
  }

  getMethodSymbol(methodName: string): MSymbol | null {
    return this.symbol.staticMembers?.get(methodName) || null;
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

  _isAssignableTo(other: MType): boolean {
    if (this === other) {
      return true;
    }
    const bases = this.symbol.bases;
    if (bases) {
      return bases.some(base => Instance.of(base.symbol).isAssignableTo(other));
    }
    return Any.isAssignableTo(other);
  }

  _closestCommonType(other: MType): MType {
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

  getForInItemType(): MType | null {
    const iterMethodSymbol = this.symbol.members.get('__iter__');
    const signature = iterMethodSymbol?.functionSignature || null;
    if (!signature || signature.typeParameters.length > 0 || signature.parameters.length > 0) {
      return null;
    }
    return signature.returnType.getForInItemType();
  }

  getCompletionScope(): MScope | null {
    return MScope.new(null, this.symbol.members);
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

  _isAssignableTo(other: MType): boolean {
    return this === other;
  }

  _closestCommonType(other: MType): MType {
    return this === other ? this : UntypedModule.closestCommonType(other);
  }

  getFieldSymbol(fieldName: string): MSymbol | null {
    return this.symbol.members.get(fieldName) || null;
  }

  getMethodSymbol(methodName: string): MSymbol | null {
    return this.symbol.members.get(methodName) || null;
  }

  getCompletionScope(): MScope | null {
    return MScope.new(null, this.symbol.members);
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
  readonly typeParameters: TypeParameter[];

  /** Required parameters (does not include optional parameters) */
  readonly parameters: [string, MType][];

  readonly optionalParameters: [string, MType][];
  readonly returnType: MType;
  constructor(
      typeParameters: TypeParameter[],
      parameters: [string, MType][],
      optionalParameters: [string, MType][],
      returnType: MType) {
    this.typeParameters = typeParameters;
    this.parameters = parameters;
    this.optionalParameters = optionalParameters;
    this.returnType = returnType;
  }
  toType(): MType {
    if (this.typeParameters.length > 0) {
      // TODO: Consider extending the Function type to support generics
      return UntypedFunction;
    }
    return Function.of(
      this.parameters.concat(this.optionalParameters).map(p => p[1]),
      this.optionalParameters.length,
      this.returnType);
  }
  format(functionName: string): string {
    const typeargs = this.typeParameters.map(p => `${p.symbol.name}`).join(', ');
    const typeargsStr = typeargs ? `[${typeargs}]` : '';
    const reqargs = this.parameters.map(p => `${p[0]} ${p[1]}`).join(', ');
    const optargs = this.optionalParameters.map(p => `${p[0]} ${p[1]} =...`).join(', ');
    const args =
      reqargs.length === 0 ? optargs :
      optargs.length === 0 ? reqargs :
      reqargs + ', ' + optargs;
    return `def ${functionName}${typeargsStr}(${args}) ${this.returnType}`;
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

export const AnySymbol = new MSymbol('Any', null, true);
AnySymbol.typeType = Any;
AnySymbol.valueType = Class.of(AnySymbol);

export const NeverSymbol = new MSymbol('Never', null, true);
NeverSymbol.typeType = Never;
NeverSymbol.valueType = Class.of(NeverSymbol);

export const BoolSymbol = new MSymbol('Bool', null, true);
BoolSymbol.typeType = Bool;
BoolSymbol.valueType = Class.of(BoolSymbol);

export const NumberSymbol = new MSymbol('Number', null, true);
NumberSymbol.typeType = Number;
NumberSymbol.valueType = Class.of(NumberSymbol);
NumberSymbol.documentation =
  '(NOTE: Int and Float are just aliases to Number and are mostly ' +
  'for documentation purposes only)';

export const StringSymbol = new MSymbol('String', null, true);
StringSymbol.typeType = String;
StringSymbol.valueType = Class.of(StringSymbol);

export const ListSymbol = new MSymbol('List', null, true);
ListSymbol.typeType = UntypedList;
ListSymbol.valueType = Class.of(ListSymbol);

export const TupleSymbol = new MSymbol('Tuple', null, true);
TupleSymbol.typeType = UntypedTuple;
TupleSymbol.valueType = Class.of(TupleSymbol);

export const DictSymbol = new MSymbol('Dict', null, true);
DictSymbol.typeType = UntypedDict;
DictSymbol.valueType = Class.of(DictSymbol);

export const FrozenDictSymbol = new MSymbol('FrozenDict', null, true);
FrozenDictSymbol.typeType = UntypedFrozenDict;
FrozenDictSymbol.valueType = Class.of(FrozenDictSymbol);

export const OptionalSymbol = new MSymbol('Optional', null, true);
OptionalSymbol.typeType = UntypedOptional;
OptionalSymbol.valueType = Class.of(OptionalSymbol);

export const IterableSymbol = new MSymbol('Iterable', null, true);
IterableSymbol.typeType = UntypedIterable;
IterableSymbol.valueType = Class.of(IterableSymbol);

export const IterateSymbol = new MSymbol('Iteration', null, true);
IterateSymbol.typeType = UntypedIterate;
IterateSymbol.valueType = Class.of(IterateSymbol);

export const FunctionSymbol = new MSymbol('Function', null, true);
FunctionSymbol.typeType = UntypedFunction;
FunctionSymbol.valueType = Class.of(FunctionSymbol);

export const ClassSymbol = new MSymbol('Class', null, true);
ClassSymbol.typeType = UntypedClass;
ClassSymbol.valueType = Class.of(ClassSymbol);
ClassSymbol.staticMembers = mkmap([
  mkmethod('getName', Function.of([UntypedClass], 0, String)),
]);

export const TypeSymbols = [
  AnySymbol,
  NeverSymbol,
  BoolSymbol,
  NumberSymbol,
  StringSymbol,
  ListSymbol,
  TupleSymbol,
  DictSymbol,
  FrozenDictSymbol,
  OptionalSymbol,
  IterableSymbol,
  IterateSymbol,
  FunctionSymbol,
  ClassSymbol,
];

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
    mkmethod('__iter__', Function.of([], 0, Function.of([], 0, Iteration.of(self.itemType)))),
  ]);
}

export function makeTupleMethodMap(self: Tuple): Map<string, MSymbol> {
  return mkmap([
    mkmethod('__mul__', Function.of([Number], 0, self)),
    mkmethod('__getitem__', Function.of([Number], 0, self.itemType)),
    mkmethod('__contains__', Function.of([self.itemType], 0, Bool)),
    mkmethod('__notcontains__', Function.of([self.itemType], 0, Bool)),
    mkmethod('__iter__', Function.of([], 0, Function.of([], 0, Iteration.of(self.itemType)))),
  ]);
}

export function makeDictMethodMap(self: Dict): Map<string, MSymbol> {
  return mkmap([
    mkmethod('__getitem__', Function.of([self.keyType], 0, self.valueType)),
    mkmethod('__setitem__', Function.of([self.keyType, self.valueType], 0, Nil)),
    mkmethod('__contains__', Function.of([self.keyType], 0, Bool)),
    mkmethod('__notcontains__', Function.of([self.keyType], 0, Bool)),
    mkmethod('__iter__', Function.of([], 0, Function.of([], 0, Iteration.of(self.keyType)))),
    mkmethod('delete', Function.of([self.keyType], 0, Bool)),
    mkmethod('rget', Function.of([self.valueType, self.keyType], 1, self.keyType)),
    mkmethod('freeze', Function.of([], 0, FrozenDict.of(self.keyType, self.valueType))),
  ]);
}

export function makeFrozenDictMethodMap(self: FrozenDict): Map<string, MSymbol> {
  return mkmap([
    mkmethod('__getitem__', Function.of([self.keyType], 0, self.valueType)),
    mkmethod('__contains__', Function.of([self.keyType], 0, Bool)),
    mkmethod('__notcontains__', Function.of([self.keyType], 0, Bool)),
    mkmethod('__iter__', Function.of([], 0, Function.of([], 0, Iteration.of(self.keyType)))),
    mkmethod('rget', Function.of([self.valueType, self.keyType], 1, self.keyType)),
  ]);
}
