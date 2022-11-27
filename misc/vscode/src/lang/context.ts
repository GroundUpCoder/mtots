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
  private readonly moduleCache: Map<string, ast.Module> = new Map();
  private currentVersion: number = 1;
  readonly builtinScope: MScope = new MScope();
  constructor(sourceFinder: SourceFinder) {
    this.sourceFinder = sourceFinder;
    this.loadBuiltin();
  }

  private async loadBuiltin() {
    const preludeModule = await this.loadModule('__builtin__', new Map());
    if (!preludeModule) {
      return; // TODO: indicate error
    }
    for (const symbol of preludeModule.scope.map.values()) {
      this.builtinScope.set(symbol);
    }
  }

  async loadModuleWithContents(uri: Uri, contents: string) {
    this.currentVersion++;
    const scanner = new MScanner(uri, contents);
    const parser = new MParser(scanner);
    const file = parser.parseFile();
    const module = await this.solveFile(file, new Map());
    return module;
  }

  private async loadFile(moduleName: string): Promise<
      {file: ast.File, fromCache: boolean} | null> {
    let oldUriAndVersion: [Uri, number] | null = null;
    let cachedAst: ast.File | null = null;
    const cacheEntry = this.fileCache.get(moduleName);
    if (cacheEntry) {
      [cachedAst, oldUriAndVersion] = cacheEntry;
    }
    const finderResult = await this.sourceFinder(moduleName, oldUriAndVersion);
    if (finderResult === 'useCached') {
      if (cachedAst) {
        return {file: cachedAst, fromCache: true};
      } else {
        throw Error(`Assertion Error`);
      }
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
    return {file, fromCache: false};
  }

  private async loadModule(
      moduleName: string,
      localCache: Map<string, ast.Module>): Promise<ast.Module | null> {
    const locallyCachedModule = localCache.get(moduleName);
    if (locallyCachedModule) {
      return locallyCachedModule;
    }
    const loadFileResult = await this.loadFile(moduleName);
    if (loadFileResult === null) {
      return null;
    }
    const { file, fromCache } = loadFileResult;
    if (fromCache) {
      // If the File was cached, there's a chance that we can use the
      // cached Module as well.
      const cachedModule = this.moduleCache.get(moduleName);
      if (cachedModule) {
        const cachedVersion = cachedModule.version;
        let refreshRequired = false;
        for (const imp of file.imports) {
          const importPath = imp.module.toString();
          const importedModule = await this.loadModule(importPath, localCache);
          if (importedModule) {
            if (importedModule.version > cachedVersion) {
              // We have a dependency that is newer than the last time this
              // module was cached. This means that we have to re-solve the
              // module.
              refreshRequired = true;
              break;
            }
          } else {
            refreshRequired = true;
            break;
          }
        }
        if (!refreshRequired) {
          localCache.set(moduleName, cachedModule);
          return cachedModule;
        }
      }
    }
    const module = await this.solveFile(file, localCache);
    localCache.set(moduleName, module);
    return module;
  }

  private async solveFile(
      file: ast.File,
      localCache: Map<string, ast.Module>): Promise<ast.Module> {
    const module = new ast.Module(this.currentVersion, file, new MScope(this.builtinScope));
    const solver = new Solver(
      module.scope,
      module.errors,
      module.symbolUsages,
      module.completionPoints);

    // PREPARE IMPORTS
    for (const imp of module.file.imports) {
      const importSymbol = solver.recordSymbolDefinition(imp.alias, true);
      importSymbol.valueType = new type.Module(importSymbol, imp.module);
      const importedModule = await this.loadModule(imp.module.toString(), localCache);
      if (importedModule) {
        importSymbol.documentation = importedModule.file.documentation;
        importSymbol.members = importedModule.scope.map;
        if (importedModule.errors.length > 0) {
          solver.errors.push(new MError(imp.location, `imported module has errors`));
        }
      } else {
        solver.errors.push(new MError(
          imp.location, `Module ${imp.module} not found`));
      }
    }

    // PREPARE CLASS DEFINITIONS
    for (const cdecl of module.file.statements) {
      if (cdecl instanceof ast.Class) {
        const classSymbol = solver.recordSymbolDefinition(cdecl.identifier, true);
        classSymbol.typeType = type.Instance.of(classSymbol);
        classSymbol.valueType = type.Class.of(classSymbol);
      }
    }

    // MAIN SOLVE LOOP
    for (const statement of module.file.statements) {
      solver.solveStatement(statement);
    }
    return module;
  }
}
