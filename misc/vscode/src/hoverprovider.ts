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
        const documentation = e.symbol.definition.documentation;
        if (documentation) {
          return new vscode.Hover(documentation.value);
        }
      }
    }
    return null;
  },
};
