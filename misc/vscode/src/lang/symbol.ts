import { MLocation } from "./location";

let nextSymbolID = 0;

export class MSymbol {
  id: number;
  name: string;
  definition: MSymbolDefinition | null;
  usages: MSymbolUsage[];
  constructor(name: string) {
    this.id = nextSymbolID++;
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
