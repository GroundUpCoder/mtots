import * as vscode from 'vscode';
import * as converter from './converter';
import { MLocation } from './lang/location';
import { MParser, ParseContext } from './lang/parser';
import { MScanner } from './lang/scanner';
import { MSymbol } from './lang/symbol';
import { DefaultSourceFinder } from './sourcefinder';

export const hoverProvider: vscode.HoverProvider = {
  async provideHover(document, position, token) {
    const moduleSymbol = new MSymbol('__main__', MLocation.of(document.uri));
    const scanner = new MScanner(document.uri, document.getText());
    const parser = new MParser(scanner, moduleSymbol, new ParseContext(DefaultSourceFinder));
    const module = await parser.parseModule();
    const usage = module.findUsage(converter.convertPosition(position));
    if (usage) {
      const markedStrings: vscode.MarkdownString[] = [];
      const type = usage.symbol.type
      markedStrings.push(new vscode.MarkdownString(type.toString()));
      const documentation = usage.symbol.documentation;
      if (documentation) {
        markedStrings.push(new vscode.MarkdownString(documentation.value));
      }
      if (markedStrings.length > 0) {
        return new vscode.Hover(markedStrings);
      }
    }
    return null;
  },
};
