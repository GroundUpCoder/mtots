import * as vscode from "vscode";
import * as converter from "./converter";
import { MLocation } from "./lang/location";
import { MParser, ParseContext } from "./lang/parser";
import { MScanner } from "./lang/scanner";
import { MSymbol } from "./lang/symbol";
import { DefaultSourceFinder } from "./sourcefinder";


export const definitionProvider: vscode.DefinitionProvider = {
  async provideDefinition(document, position, token) {
    const moduleSymbol = new MSymbol('__main__', MLocation.of(document.uri));
    const scanner = new MScanner(document.uri, document.getText());
    const parser = new MParser(scanner, moduleSymbol, new ParseContext(DefaultSourceFinder));
    const module = await parser.parseModule();
    const usage = module.findUsage(converter.convertPosition(position));
    if (usage) {
      return converter.convertMLocation(usage.symbol.definition.location);
    }
    return null;
  },
}
