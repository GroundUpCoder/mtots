import * as vscode from 'vscode';
import * as converter from './converter';
import { MContext } from './state';

function formatDocString(docString: string): string {
  if (docString.startsWith('\n    ')) {
    return docString.replace(/\n    /g, '\n');
  }
  if (docString.startsWith('\n  ')) {
    return docString.replace(/\n  /g, '\n');
  }
  return docString;
}

export const hoverProvider: vscode.HoverProvider = {
  async provideHover(document, position, token) {
    const module = await MContext.loadModuleWithContents(document.uri, document.getText());
    if (!module) {
      return;
    }
    const usage = module.findUsage(converter.convertPosition(position));
    if (usage) {
      const markedStrings: vscode.MarkdownString[] = [];
      const type = usage.symbol.valueType
      if (type) {
        const typeMarkdownString = new vscode.MarkdownString();
        typeMarkdownString.appendCodeblock(type.toString());
        markedStrings.push(typeMarkdownString);
      }
      const documentation = usage.symbol.documentation;
      if (documentation) {
        const docMarkdownString = new vscode.MarkdownString();
        const docString = formatDocString(documentation);
        docMarkdownString.appendMarkdown(docString);
        markedStrings.push(docMarkdownString);
      }
      if (markedStrings.length > 0) {
        return new vscode.Hover(markedStrings);
      }
    }
    return null;
  },
};
