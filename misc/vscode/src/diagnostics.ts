import * as vscode from "vscode";
import * as converter from "./converter";
import { MError } from "./lang/error";
import { MLocation } from "./lang/location";
import { MParser, ParseContext } from "./lang/parser";
import { MRange } from "./lang/range";
import { MScanner } from "./lang/scanner";
import { MSymbol } from "./lang/symbol";
import { DefaultSourceFinder } from "./sourcefinder";


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
  const moduleSymbol = new MSymbol('__main__', MLocation.of(document.uri));
  const scanner = new MScanner(document.uri, document.getText());
  const parser = new MParser(scanner, moduleSymbol, new ParseContext(DefaultSourceFinder));
  try {
    const module = await parser.parseModule();
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
