import { MSymbol } from "./symbol";



export class MScope {
  parent: this | null;
  map: Map<string, MSymbol>;

  static new(parent: MScope | null = null, map: Map<string, MSymbol> | null = null): MScope {
    const scope = new MScope(map);
    scope.parent = parent;
    return scope;
  }

  private constructor(map: Map<string, MSymbol> | null = null) {
    this.parent = null;
    this.map = map || new Map();
  }

  get(name: string): MSymbol | null {
    let scope: this | null = this;
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
