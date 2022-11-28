import * as vscode from "vscode"
import * as converter from "./converter"
import { MContext } from "./state"


export const signatureHelpProvider: vscode.SignatureHelpProvider = {
  async provideSignatureHelp(document, position, token, context) {
    const module = await MContext.loadModuleWithContents(document.uri, document.getText());
    const sh = module.findSignatureHelper(converter.convertPosition(position));
    if (!sh) {
      return null;
    }
    const parameterLabels = sh.parameterNames ?
      sh.parameterNames.map((n, i) => `${n} ${sh.parameterTypes[i]}`) :
      sh.parameterTypes.map((t, i) => `arg${i} ${t}`);

    const help = new vscode.SignatureHelp();
    help.activeParameter = sh.parameterIndex;
    help.activeSignature = 0;
    const signatureInformation = new vscode.SignatureInformation(
      (sh.functionName || '') +
      `(${parameterLabels.join(', ')})` +
      (sh.returnType ? ` ${sh.returnType}` : ''));
    if (sh.functionDocumentation) {
      signatureInformation.documentation = sh.functionDocumentation;
    }
    signatureInformation.activeParameter = sh.parameterIndex;
    signatureInformation.parameters.push(...sh.parameterTypes.map((pt, i) => {
      return new vscode.ParameterInformation(parameterLabels[i]);
    }));
    help.signatures = [signatureInformation];
    return help;
  },
}
