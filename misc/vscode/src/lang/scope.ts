import { MSymbol } from "./symbol";



export class MScope {
  parent: MScope | null;
  map: Map<string, MSymbol>;
  constructor(parent: MScope | null = null, map: Map<string, MSymbol> | null = null) {
    this.parent = parent;
    this.map = map || new Map();
  }

  get(name: string): MSymbol | null {
    let scope: MScope | null = this;
    while (scope) {
      const value = scope.map.get(name);
      if (value) {
        return value;
      }
      scope = scope.parent;
    }
    return null;
  }

  set(symbol: MSymbol) {
    this.map.set(symbol.name, symbol);
  }
}
