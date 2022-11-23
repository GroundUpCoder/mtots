import * as vscode from 'vscode';
import * as converter from './converter';
import { MProvideHoverException } from './lang/error';
import { MParser } from './lang/parser';
import { MScanner } from './lang/scanner';

export const hoverProvider: vscode.HoverProvider = {
  provideHover(document, position, token) {
    const scanner = new MScanner(document.uri, document.getText());
    try {
      const parser = new MParser(scanner);
      parser.provideHoverTrigger = converter.convertPosition(position);
      parser.parseModule();
    } catch (e) {
      if (e instanceof MProvideHoverException) {
        const markedStrings: vscode.MarkdownString[] = [];
        const type = e.symbol.definition.type
        markedStrings.push(new vscode.MarkdownString(type.toString()));
        const documentation = e.symbol.definition.documentation;
        if (documentation) {
          markedStrings.push(new vscode.MarkdownString(documentation.value));
        }
        if (markedStrings.length > 0) {
          return new vscode.Hover(markedStrings);
        }
      }
    }
    return null;
  },
};
