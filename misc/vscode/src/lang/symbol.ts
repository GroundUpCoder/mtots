import * as ast from "./ast";
import { MLocation } from "./location";
import * as type from "./type";

export class MSymbol {
  /** The name of the MSymbol as it appears in its definition */
  readonly name: string;

  /** location of the MSymbol's definition */
  readonly location: MLocation;

  /** whether or not this symbol should be considered immutable */
  readonly final: boolean;

  /** If available, documentation related to the MSymbol */
  documentation: ast.StringLiteral | null;

  /** The type of this symbol when it appears in a value expression. Defaults to Any */
  valueType: type.MType;

  /** The type of this symbol when it appears in a type expression.
   * Defaults to null (these are cases where it does not make sense for the given
   * symbol to appear in a type expression, e.g. varable name)
   */
  typeType: type.MType | null;

  /** The members of this MSymbol, if provided */
  members: Map<string, MSymbol>;

  /** The usage that also corresponds to the definition of this MSymbol */
  readonly definition: MSymbolUsage;

  /** A list of all usages of this MSymbol */
  readonly usages: MSymbolUsage[];

  constructor(
      name: string,
      definitionLocation: MLocation,
      final: boolean = true,
      members: Map<string, MSymbol> | null = null) {
    this.name = name;
    this.final = final;
    this.location = definitionLocation;
    this.documentation = null;
    this.valueType = type.Any;
    this.typeType = null;
    this.members = members || new Map();
    this.usages = [];
    this.definition = new MSymbolUsage(definitionLocation, this);
    this.usages.push(this.definition);
  }
}

export class MSymbolUsage {
  readonly location: MLocation;
  readonly symbol: MSymbol;
  constructor(location: MLocation, symbol: MSymbol) {
    this.location = location;
    this.symbol = symbol;
  }
}
