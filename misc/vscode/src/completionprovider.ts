import * as vscode from "vscode";
import * as converter from "./converter";
import { MContext } from "./state";



export const completionProvider: vscode.CompletionItemProvider = {
  async provideCompletionItems(document, position, token, context) {
    const module = await MContext.loadModuleWithContents(document.uri, document.getText());
    if (!module) {
      return null;
    }
    const completionPoint = module.findCompletionPoint(converter.convertPosition(position));
    if (!completionPoint) {
      return null;
    }
    const completions = completionPoint.getCompletions();
    return completions.map(completion => new vscode.CompletionItem(completion));
  },
};
