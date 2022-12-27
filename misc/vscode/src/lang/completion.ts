import { MLocation } from "./location";
import { MScope } from "./scope";
import { MSymbol } from "./symbol";
import * as type from "./type";



export class CompletionPoint {
  readonly location: MLocation;
  readonly scope: MScope;
  constructor(location: MLocation, scope: MScope) {
    this.location = location;
    this.scope = scope;
  }

  protected additionalNames(): string[] {
    return [];
  }

  protected includeSymbol(symbol: MSymbol): boolean {
    return true;
  }

  getCompletions(): string[] {
    const allKeys = new Set<string>(this.additionalNames());
    for (let scope: MScope | null = this.scope; scope; scope = scope.parent) {
      for (const [key, symbol] of scope.map.entries()) {
        if (this.includeSymbol(symbol)) {
          allKeys.add(key);
        }
      }
    }
    const sortedKeys = Array.from(allKeys);
    sortedKeys.sort();
    return sortedKeys;
  }
}

/**
 * CompletionPoint where a type is expected.
 * In this case, only types and import names are allowed.
 * Also, builtin type names are explicitly included
 */
export class TypeParentCompletionPoint extends CompletionPoint {
  protected additionalNames(): string[] {
    return [
      'Any',
      'Bool',
      'Number',
      'String',
      'List',
      'Dict',
    ]
  }

  protected includeSymbol(symbol: MSymbol): boolean {
    return symbol.isTypeSymbol() || symbol.isImportSymbol();
  }
}

/**
 * CompletionPoint where a type in an explicitly listed module is expected.
 * In this case, only type names are allowed.
 */
export class TypeChildCompletionPoint extends CompletionPoint {
  protected includeSymbol(symbol: MSymbol): boolean {
    return symbol.isTypeSymbol();
  }
}
