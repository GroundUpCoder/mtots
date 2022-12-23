

export interface SymbolLike {
  name: string
}

export abstract class BaseScope<Symbol extends SymbolLike> {
  parent: this | null;
  map: Map<string, Symbol>;

  protected constructor(map: Map<string, Symbol> | null = null) {
    this.parent = null;
    this.map = map || new Map();
  }

  get(name: string): Symbol | null {
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

  set(symbol: Symbol) {
    this.map.set(symbol.name, symbol);
  }
}
