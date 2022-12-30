import { MLocation } from "./location";
import type * as type from "./type";

export class MSymbol {
  /** The name of the MSymbol as it appears in its definition */
  readonly name: string;

  /** location of the MSymbol's definition */
  readonly location: MLocation | null;

  /** whether or not this symbol should be considered immutable */
  readonly final: boolean;

  /** Whether or not this symbol is from an import statement */
  private readonly isImport: boolean;

  /** If available, documentation related to the MSymbol */
  documentation: string | null;

  /** The type of this symbol when it appears in a value expression */
  valueType: type.MType | null;

  /** The type of this symbol when it appears in a type expression.
   * Defaults to null (these are cases where it does not make sense for the given
   * symbol to appear in a type expression, e.g. varable name)
   */
  typeType: type.MType | null;

  /** The members of this MSymbol, if provided */
  members: Map<string, MSymbol>;

  /** The static members of this MSymbol, if provided (only for Class Symbols) */
  staticMembers: Map<string, MSymbol> | null;

  /**
   * If this symbol is a function, this field contains the
   * function signature.
   */
  functionSignature: type.FunctionSignature | null;

  /**
   * The usage that also corresponds to the definition of this MSymbol.
   * Some builtin symbols will not have a 'definition' to go to.
   */
  readonly definition: MSymbolUsage | null;

  constructor(
      name: string,
      definitionLocation: MLocation | null,
      final: boolean = true,
      members: Map<string, MSymbol> | null = null,
      isImport: boolean = false) {
    this.name = name;
    this.final = final;
    this.isImport = isImport;
    this.location = definitionLocation;
    this.documentation = null;
    this.valueType = null;
    this.typeType = null;
    this.members = members || new Map();
    this.staticMembers = null;
    this.functionSignature = null;
    this.definition = definitionLocation ? new MSymbolUsage(definitionLocation, this) : null;
  }

  isImportSymbol(): boolean {
    return this.isImport;
  }

  isTypeSymbol(): boolean {
    return !!this.typeType;
  }
}

export class MSymbolUsage {
  readonly location: MLocation;
  readonly symbol: MSymbol;
  readonly bindings: Map<string, type.MType | null> | null
  constructor(
      location: MLocation,
      symbol: MSymbol,
      bindings: Map<string, type.MType | null> | null = null) {
    this.location = location;
    this.symbol = symbol;
    this.bindings = bindings;
  }
}
