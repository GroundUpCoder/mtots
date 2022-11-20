import { MLocation } from "./location";

export class MSymbol {
  id: number;
  name: string;
  definition: MSymbolDefinition | null;
  usages: MSymbolUsage[];
  constructor(id: number, name: string) {
    this.id = id;
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

export class MSymbolTable {
  nextID: number;
  constructor() {
    this.nextID = 0;
  }
  newSymbol(name: string): MSymbol {
    return new MSymbol(this.nextID++, name);
  }
}
