import * as vscode from "vscode";
import * as converter from "./converter";
import { MGotoDefinitionException } from "./lang/error";
import { MLocation } from "./lang/location";
import { MParser, ParseContext } from "./lang/parser";
import { MScanner } from "./lang/scanner";
import { MSymbol } from "./lang/symbol";
import { DefaultSourceFinder } from "./sourcefinder";


export const definitionProvider: vscode.DefinitionProvider = {
  async provideDefinition(document, position, token) {
    const moduleSymbol = new MSymbol('__main__', MLocation.of(document.uri));
    const scanner = new MScanner(document.uri, document.getText());
    try {
      const parser = new MParser(scanner, moduleSymbol, new ParseContext(DefaultSourceFinder));
      parser.gotoDefinitionTrigger = converter.convertPosition(position);
      await parser.parseModule();
    } catch (e) {
      if (e instanceof MGotoDefinitionException) {
        return converter.convertMLocation(e.location);
      }
    }
    return null;
  },
}
