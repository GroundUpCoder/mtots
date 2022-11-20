import { MLocation } from "./location";
import * as type from "./type";

export class MSymbol {
  readonly name: string;
  readonly definition: MSymbolDefinition;
  readonly usages: MSymbolUsage[];
  constructor(name: string, definitionLocation: MLocation, symbolInfo: MSymbolInfo) {
    this.name = name;
    this.definition = new MSymbolDefinition(definitionLocation, this, symbolInfo);
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
  readonly info: MSymbolInfo;
  constructor(location: MLocation, symbol: MSymbol, info: MSymbolInfo) {
    super(location, symbol);
    this.info = info;
  }
}

export class MSymbolInfo {}

export class MUnknownSymbolInfo extends MSymbolInfo {}

export class MModuleSymbolInfo extends MSymbolInfo {
  modulePath: string;
  constructor(modulePath: string) {
    super();
    this.modulePath = modulePath;
  }
}
