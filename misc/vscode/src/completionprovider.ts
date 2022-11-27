import * as vscode from "vscode";
import * as converter from "./converter";
import { ParseContext } from "./lang/parser";
import { MPosition } from "./lang/position";
import { DefaultSourceFinder } from "./sourcefinder";



export const completionProvider: vscode.CompletionItemProvider = {
  async provideCompletionItems(document, position, token, context) {
    const ctx = new ParseContext(DefaultSourceFinder);
    const module = await ctx.loadModule('__main__', [document.uri, document.getText()]);
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
