import { MSymbol } from "./symbol";



export class MScope {
  map: Map<string, MSymbol>;
  parent: MScope | null;
  constructor(parent: MScope | null = null) {
    this.map = new Map();
    this.parent = parent;
  }

  get(name: string) {
  }
}
