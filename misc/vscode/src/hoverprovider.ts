import * as vscode from 'vscode';
import * as converter from './converter';
import { MProvideHoverException } from './lang/error';
import { MLocation } from './lang/location';
import { MParser, ParseContext } from './lang/parser';
import { MScanner } from './lang/scanner';
import { MSymbol } from './lang/symbol';
import { DefaultSourceFinder } from './sourcefinder';

export const hoverProvider: vscode.HoverProvider = {
  async provideHover(document, position, token) {
    const moduleSymbol = new MSymbol('__main__', MLocation.of(document.uri));
    const scanner = new MScanner(document.uri, document.getText());
    try {
      const parser = new MParser(scanner, moduleSymbol, new ParseContext(DefaultSourceFinder));
      parser.provideHoverTrigger = converter.convertPosition(position);
      await parser.parseModule();
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
