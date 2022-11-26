import * as vscode from "vscode";
import * as converter from "./converter";
import { MLocation } from "./lang/location";
import { MParser, ParseContext } from "./lang/parser";
import { MScanner } from "./lang/scanner";
import { MSymbol } from "./lang/symbol";
import { DefaultSourceFinder } from "./sourcefinder";


export const definitionProvider: vscode.DefinitionProvider = {
  async provideDefinition(document, position, token) {
    const ctx = new ParseContext(DefaultSourceFinder);
    const module = await ctx.loadModule('__main__', [document.uri, document.getText()]);
    if (!module) {
      return;
    }
    const usage = module.findUsage(converter.convertPosition(position));
    if (usage) {
      const definition = usage.symbol.definition;
      if (definition) {
        return converter.convertMLocation(definition.location);
      }
    }
    return null;
  },
}
