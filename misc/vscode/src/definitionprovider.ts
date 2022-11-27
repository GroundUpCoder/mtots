import * as vscode from "vscode";
import * as converter from "./converter";
import { MContext } from "./state";


export const definitionProvider: vscode.DefinitionProvider = {
  async provideDefinition(document, position, token) {
    const module = await MContext.loadModuleWithContents(document.uri, document.getText());
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
