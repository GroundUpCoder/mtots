import * as vscode from "vscode";
import * as converter from "./converter";
import { MGotoDefinitionException } from "./lang/error";
import { MParser } from "./lang/parser";
import { MScanner } from "./lang/scanner";


export const definitionProvider: vscode.DefinitionProvider = {
  provideDefinition(document, position, token) {
    const scanner = new MScanner(document.uri, document.getText());
    try {
      const parser = new MParser(scanner);
      parser.gotoDefinitionTrigger = converter.convertPosition(position);
      parser.parseModule();
    } catch (e) {
      if (e instanceof MGotoDefinitionException) {
        return converter.convertMLocation(e.location);
      }
    }
    return null;
  },
}
