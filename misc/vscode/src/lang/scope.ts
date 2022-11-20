import { MSymbol } from "./symbol";



export class MScope {
  map: Map<string, MSymbol>;
  parent: MScope | null;
  constructor(parent: MScope | null = null) {
    this.map = new Map();
    this.parent = parent;
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
