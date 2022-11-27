import { MScope } from "./scope";
import * as ast from "./ast";
import * as type from "./type";
import { Uri } from "vscode";
import { MScanner } from "./scanner";
import { MParser } from "./parser";
import { Solver } from "./solver";
import { MError } from "./error";

export type SourceFinder = (
  path: string,
  oldUriAndVersion: [Uri, number] | null) => Promise<
    { uri: Uri, contents: string, version: number} | 'useCached' | null>;

export class ParseContext {
  readonly sourceFinder: SourceFinder;
  private readonly fileCache: Map<string, [ast.File, [Uri, number]]> = new Map();
  readonly builtinScope: MScope = new MScope();
  constructor(sourceFinder: SourceFinder) {
    this.sourceFinder = sourceFinder;
    this.loadBuiltin();
  }

  async loadBuiltin() {
    const preludeModule = await this.loadModule('__builtin__', new Map());
    if (!preludeModule) {
      return; // TODO: indicate error
    }
    for (const symbol of preludeModule.scope.map.values()) {
      this.builtinScope.set(symbol);
    }
  }

  async loadModuleWithContents(uri: Uri, contents: string) {
    const scanner = new MScanner(uri, contents);
    const parser = new MParser(scanner);
    const file = parser.parseFile();
    const module = await this.solveFile(file, new Map());
    return module;
  }

  private async loadFile(moduleName: string): Promise<ast.File | null> {
    let oldUriAndVersion: [Uri, number] | null = null;
    let cachedAst: ast.File | null = null;
    const cacheEntry = this.fileCache.get(moduleName);
    if (cacheEntry) {
      [cachedAst, oldUriAndVersion] = cacheEntry;
    }
    const finderResult = await this.sourceFinder(moduleName, oldUriAndVersion);
    if (finderResult === 'useCached') {
      return cachedAst;
    }
    if (finderResult === null) {
      this.fileCache.delete(moduleName);
      return null;
    }
    const { uri, contents, version } = finderResult;
    const scanner = new MScanner(uri, contents);
    const parser = new MParser(scanner);
    const file = parser.parseFile();
    this.fileCache.set(moduleName, [file, [uri, version]]);
    return file;
  }

  private async loadModule(
      moduleName: string,
      moduleCache: Map<string, ast.Module>): Promise<ast.Module | null> {
    const cachedModule = moduleCache.get(moduleName);
    if (cachedModule) {
      return cachedModule;
    }
    const file = await this.loadFile(moduleName);
    if (file === null) {
      return null;
    }
    const module = await this.solveFile(file, moduleCache);
    moduleCache.set(moduleName, module);
    return module;
  }

  private async solveFile(
      file: ast.File,
      moduleCache: Map<string, ast.Module>): Promise<ast.Module> {
    const module = new ast.Module(file, new MScope(this.builtinScope));
    const solver = new Solver(
      module.scope,
      module.errors,
      module.symbolUsages,
      module.completionPoints);
    for (const imp of module.file.imports) {
      const importSymbol = solver.recordSymbolDefinition(imp.alias, true);
      importSymbol.valueType = new type.Module(importSymbol, imp.module);
      const importedModule = await this.loadModule(imp.module.toString(), moduleCache);
      if (importedModule) {
        importSymbol.members = importedModule.scope.map;
        if (importedModule.errors.length > 0) {
          solver.errors.push(new MError(imp.location, `imported module has errors`));
        }
      } else {
        solver.errors.push(new MError(
          imp.location, `Module ${imp.module} not found`));
      }
    }
    for (const statement of module.file.statements) {
      solver.solveStatement(statement);
    }
    return module;
  }

  async reset() {
    this.builtinScope.map.clear();
    await this.loadBuiltin();
  }
}
