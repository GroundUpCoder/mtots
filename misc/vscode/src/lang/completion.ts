import { MLocation } from "./location";
import { MScope } from "./scope";



export class CompletionPoint {
  readonly location: MLocation;
  readonly scope: MScope;
  constructor(location: MLocation, scope: MScope) {
    this.location = location;
    this.scope = scope;
  }

  getCompletions() {
    const allKeys = new Set<string>();
    for (let scope: MScope | null = this.scope; scope; scope = scope.parent) {
      for (const key of scope.map.keys()) {
        allKeys.add(key);
      }
    }
    const sortedKeys = Array.from(allKeys);
    sortedKeys.sort();
    return sortedKeys;
  }
}
