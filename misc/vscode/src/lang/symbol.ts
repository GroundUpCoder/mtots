import * as ast from "./ast";
import { MLocation } from "./location";
import * as type from "./type";

export class MSymbol {
  readonly name: string;
  readonly definition: MSymbolDefinition;
  readonly usages: MSymbolUsage[];
  constructor(name: string, definitionLocation: MLocation) {
    this.name = name;
    this.definition = new MSymbolDefinition(definitionLocation, this);
    this.usages = [];
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

export class MSymbolDefinition extends MSymbolUsage {
  documentation: ast.StringLiteral | null;
  type: type.MType;
  constructor(location: MLocation, symbol: MSymbol) {
    super(location, symbol);
    this.documentation = null;
    this.type = type.Any;
  }
}
