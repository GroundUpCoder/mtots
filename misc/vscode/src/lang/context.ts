import { MScope } from "./scope";
import * as ast from "./ast";
import { Uri } from "vscode";
import { MScanner } from "./scanner";
import { MParser } from "./parser";

export type SourceFinder = (path: string) => Promise<[string | Uri, string] | null>;

export class ParseContext {
  readonly sourceFinder: SourceFinder;
  private readonly moduleCache: Map<string, ast.Module> = new Map();
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

  async loadModule(
      moduleName: string,
      uriAndContents: [Uri, string] | null = null): Promise<ast.Module | null> {
    const cached = this.moduleCache.get(moduleName);
    if (cached) {
      return cached;
    }
    let filePath: string | Uri = '';
    let contents: string = '';
    if (uriAndContents) {
      [filePath, contents] = uriAndContents;
    } else {
      const finderResult = await this.sourceFinder(moduleName);
      if (!finderResult) {
        return null;
      }
      const [foundFilePath, foundContents] = finderResult;
      filePath = foundFilePath;
      contents = foundContents;
    }
    const scanner = new MScanner(filePath, contents);
    const parser = new MParser(scanner, this);
    const module = await parser.parseModule();
    this.moduleCache.set(moduleName, module);
    return module;
  }
}
