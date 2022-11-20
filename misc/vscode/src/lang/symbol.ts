import { MLocation } from "./location";

export class MSymbol {
  name: string;
  definition: MSymbolDefinition | null;
  usages: MSymbolUsage[];
  constructor(name: string) {
    this.name = name;
    this.definition = null;
    this.usages = [];
  }
}

export class MSymbolDefinition {
  location: MLocation;
  constructor(location: MLocation) {
    this.location = location;
  }
}

export class MSymbolUsage {
  location: MLocation;
  constructor(location: MLocation) {
    this.location = location;
  }
}
