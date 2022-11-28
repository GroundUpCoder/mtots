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
    const help = new vscode.SignatureHelp();
    help.activeParameter = sh.parameterIndex;
    help.activeSignature = 0;
    const signatureInformation = new vscode.SignatureInformation(
      `(${sh.parameterTypes.map((pt, i) => `arg${i} ${pt}`).join(', ')})`);
    signatureInformation.activeParameter = sh.parameterIndex;
    signatureInformation.parameters.push(...sh.parameterTypes.map((pt, i) => {
      const pi = new vscode.ParameterInformation(`arg${i} ${pt}`);
      pi.documentation = pt.toString();
      return pi;
    }));
    help.signatures = [signatureInformation, new vscode.SignatureInformation('signature-2')];
    return help;
  },
}
