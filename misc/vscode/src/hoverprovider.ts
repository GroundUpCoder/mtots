import * as vscode from 'vscode';
import * as type from './lang/type';
import * as converter from './converter';
import { MContext } from './state';
import { TypeBinder } from './lang/solver';

function formatDocString(docString: string): string {
  if (docString.startsWith('\n    ')) {
    return docString.replace(/\n    /g, '\n').replace('\n', '');
  }
  if (docString.startsWith('\n  ')) {
    return docString.replace(/\n  /g, '\n').replace('\n', '');
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
      const symbol = usage.symbol;
      const markedStrings: vscode.MarkdownString[] = [];
      const signature = symbol.functionSignature;
      if (signature) {
        const signatureMarkdownString = new vscode.MarkdownString();
        signatureMarkdownString.appendCodeblock(signature.format(symbol.name));
        markedStrings.push(signatureMarkdownString);
        if (usage.bindings) {
          const binder = new TypeBinder(usage.bindings, false);
          const boundSignature = new type.FunctionSignature(
            [],
            signature.parameters.map(p => [p[0], binder.bind(p[1], type.Any)]),
            signature.optionalParameters.map(p => [p[0], binder.bind(p[1], type.Any)]),
            binder.bind(signature.returnType, type.Any));
            const signatureMarkdownString = new vscode.MarkdownString();
            signatureMarkdownString.appendCodeblock(boundSignature.format(symbol.name));
            markedStrings.push(signatureMarkdownString);
        }
      } else {
        const valueType = symbol.valueType
        if (valueType) {
          const typeMarkdownString = new vscode.MarkdownString();
          typeMarkdownString.appendCodeblock(valueType.format(symbol.final, symbol.name));
          markedStrings.push(typeMarkdownString);
        } else {
          const typeType = symbol.typeType;
          if (typeType) {
            if (typeType instanceof type.BuiltinPrimitive) {
              const builtinMarkdownString = new vscode.MarkdownString();
              builtinMarkdownString.appendCodeblock(`class ${typeType.name}(Builtin)`);
              markedStrings.push(builtinMarkdownString);
            } else if (typeType instanceof type.TypeParameter) {
              const builtinMarkdownString = new vscode.MarkdownString();
              builtinMarkdownString.appendCodeblock(`${typeType.symbol.name}`);
              markedStrings.push(builtinMarkdownString);
            }
          }
        }
      }
      const documentation = symbol.documentation;
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
