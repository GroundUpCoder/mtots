import * as vscode from 'vscode';
import { MError } from './lang/error';
import { MScanner } from './lang/scanner';

export async function tokenize() {
  const editor = vscode.window.activeTextEditor;
  if (!editor) {
    return;
  }
  const selection =
    editor.selection.isEmpty ?
      new vscode.Range(
        editor.document.lineAt(0).range.start,
        editor.document.lineAt(editor.document.lineCount - 1).range.end) :
      new vscode.Range(
        editor.selection.start,
        editor.selection.end);
  const text = editor.document.getText(selection);
  const document = await vscode.workspace.openTextDocument({
    content: '',
    language: 'plaintext',
  });

  const scanner = new MScanner(document.uri, text);

  let insertText = '';

  try {
    while (true) {
      const token = scanner.scanToken();
      insertText += (
        `${token.range.start.line + 1}:` +
        `${token.range.start.column + 1} - ` +
        `${token.range.end.line + 1}:` +
        `${token.range.end.column + 1} - ` +
        `${token.type} ` +
        `${token.value === null ? '' : JSON.stringify(token.value)}\n`);
      if (token.type === 'EOF') {
        break;
      }
    }
  } catch (e) {
    if (!(e instanceof MError)) {
      throw e;
    }
    insertText += `ERROR: ${e.message}`;
    insertText += `  ${e.location.range.start.line + 1}:`;
    insertText += `${e.location.range.start.column + 1}`;
  }

  const edit = new vscode.WorkspaceEdit();
  edit.insert(document.uri, new vscode.Position(0, 0), insertText);
  if (await vscode.workspace.applyEdit(edit)) {
    vscode.window.showTextDocument(document);
  }
}
