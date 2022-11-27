import { MScope } from "./scope";
import * as ast from "./ast";
import { Uri } from "vscode";
import { MScanner } from "./scanner";
import { MParser } from "./parser";

export type SourceFinder = (
  path: string,
  oldVersion: number | null) => Promise<
    { uri: Uri, contents: string, version: number} | 'useCached' | null>;

export class ParseContext {
  readonly sourceFinder: SourceFinder;
  private readonly moduleCache: Map<string, [ast.Module, number]> = new Map();
  readonly builtinScope: MScope = new MScope();
  constructor(sourceFinder: SourceFinder) {
    this.sourceFinder = sourceFinder;
    this.loadBuiltin();
  }

  async loadBuiltin() {
    const preludeModule = await this.loadModule('__builtin__');
    if (!preludeModule) {
      return; // TODO: indicate error
    }
    for (const symbol of preludeModule.scope.map.values()) {
      this.builtinScope.set(symbol);
    }
  }

  async loadModuleWithContents(uri: Uri, contents: string) {
    const scanner = new MScanner(uri, contents);
    const parser = new MParser(scanner, this);
    const module = await parser.parseModule();
    return module;
  }

  async loadModule(moduleName: string): Promise<ast.Module | null> {
    let oldVersion: number | null = null;
    const cacheEntry = this.moduleCache.get(moduleName);
    let cachedAst: ast.Module | null = null;
    if (cacheEntry) {
      [cachedAst, oldVersion] = cacheEntry;
    }
    const finderResult = await this.sourceFinder(moduleName, oldVersion);
    if (finderResult === 'useCached') {
      return cachedAst;
    }
    if (finderResult === null) {
      return null;
    }
    const { uri, contents, version } = finderResult;
    const scanner = new MScanner(uri, contents);
    const parser = new MParser(scanner, this);
    const module = await parser.parseModule();
    this.moduleCache.set(moduleName, [module, version]);
    return module;
  }

  async reset() {
    this.moduleCache.clear();
    this.builtinScope.map.clear();
    await this.loadBuiltin();
  }
}
