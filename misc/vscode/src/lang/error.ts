import { MLocation } from "./location";
import { MSymbol } from "./symbol";


export class MError {
  location: MLocation;
  message: string;
  constructor(location: MLocation, message: string) {
    this.location = location;
    this.message = message;
  }
}

export class MGotoDefinitionException {
  location: MLocation;
  constructor(location: MLocation) {
    this.location = location;
  }
}

export class MProvideHoverException {
  symbol: MSymbol;
  constructor(symbol: MSymbol) {
    this.symbol = symbol;
  }
}
