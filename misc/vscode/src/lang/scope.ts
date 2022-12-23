import { BaseScope } from "./solver/base/scope";
import { MSymbol } from "./symbol";



export class MScope extends BaseScope<MSymbol> {
  static new(parent: MScope | null = null, map: Map<string, MSymbol> | null = null): MScope {
    const scope = new MScope(map);
    scope.parent = parent;
    return scope;
  }
}
