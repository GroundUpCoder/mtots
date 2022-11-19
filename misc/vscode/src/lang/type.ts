


export abstract class MType {
  static nextID: number = 0;

  id: number;

  constructor() {
    this.id = MType.nextID++;
  }

  abstract isSubclassOf(type: MType): boolean;
}

export class BuiltinPrimitive extends MType {
  parent: BuiltinPrimitive | null;
  name: string;

  static Any = new BuiltinPrimitive('any', null);
  static Nil = new BuiltinPrimitive('nil', BuiltinPrimitive.Any);
  static Bool = new BuiltinPrimitive('bool', BuiltinPrimitive.Any);
  static Float = new BuiltinPrimitive('float', BuiltinPrimitive.Any);
  static Int = new BuiltinPrimitive('int', BuiltinPrimitive.Float);
  static String = new BuiltinPrimitive('string', BuiltinPrimitive.Any);
  static UntypedList = new BuiltinPrimitive('list', BuiltinPrimitive.Any);

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
}

export const Any = BuiltinPrimitive.Any;
export const Nil = BuiltinPrimitive.Nil;
export const Bool = BuiltinPrimitive.Bool;
export const Float = BuiltinPrimitive.Float;
export const Int = BuiltinPrimitive.Int;
export const String = BuiltinPrimitive.String;
export const UntypedList = BuiltinPrimitive.UntypedList;


export class List extends MType {
  private static map: Map<MType, List> = new Map();

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
}
