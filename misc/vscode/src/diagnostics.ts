import * as vscode from "vscode";
import * as converter from "./converter";
import { MError } from "./lang/error";
import { MContext } from "./state";


export async function initDiagnostic(context: vscode.ExtensionContext) {
  const collection = vscode.languages.createDiagnosticCollection('mtots');
  if (vscode.window.activeTextEditor) {
    if (vscode.window.activeTextEditor.document.languageId === 'mtots') {
      await updateDiagnostics(vscode.window.activeTextEditor.document, collection);
    }
  }
  context.subscriptions.push(vscode.workspace.onDidSaveTextDocument(async document => {
    if (document.languageId === 'mtots') {
      await updateDiagnostics(document, collection);
    }
  }));
  context.subscriptions.push(vscode.window.onDidChangeActiveTextEditor(async editor => {
    if (editor?.document.languageId === 'mtots') {
      await updateDiagnostics(editor.document, collection);
    }
  }));
}

async function updateDiagnostics(document: vscode.TextDocument, dc: vscode.DiagnosticCollection) {
  try {
    const module = await MContext.loadModuleWithContents(document.uri, document.getText());
    if (!module) {
      return;
    }
    dc.set(document.uri, module.errors.map(e => { return {
      message: e.message,
      range: converter.convertMRange(e.location.range),
      severity: vscode.DiagnosticSeverity.Warning,
    }}));
  } catch (e) {
    if (e instanceof MError) {
      dc.set(document.uri, [{
        message: e.message,
        range: converter.convertMRange(e.location.range),
        severity: vscode.DiagnosticSeverity.Error,
      }]);
      return;
    } else {
      throw e;
    }
  }
}
