import * as vscode from "vscode";
import * as converter from "./converter";
import { MError } from "./lang/error";
import { MParser } from "./lang/parser";
import { MScanner } from "./lang/scanner";


export function initDiagnostic(context: vscode.ExtensionContext) {
  const collection = vscode.languages.createDiagnosticCollection('mtots');
  if (vscode.window.activeTextEditor) {
    if (vscode.window.activeTextEditor.document.languageId === 'mtots') {
      updateDiagnostics(vscode.window.activeTextEditor.document, collection);
    }
  }
  context.subscriptions.push(vscode.workspace.onDidSaveTextDocument(document => {
    if (document.languageId === 'mtots') {
      updateDiagnostics(document, collection);
    }
  }));
  context.subscriptions.push(vscode.window.onDidChangeActiveTextEditor(editor => {
    if (editor?.document.languageId === 'mtots') {
      updateDiagnostics(editor.document, collection);
    }
  }));
}

function updateDiagnostics(document: vscode.TextDocument, dc: vscode.DiagnosticCollection) {
  const scanner = new MScanner(document.uri, document.getText());
  const parser = new MParser(scanner);
  try {
    const module = parser.parseModule();
    dc.set(document.uri, module.semanticErrors.map(e => { return {
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
