import * as vscode from 'vscode';
import { MError } from './lang/error';
import { MParser } from './lang/parser';
import { MScanner } from './lang/scanner';

async function writeToNewEditor(
    f: (emit: (m: string) => void) => void,
    language: string = 'plaintext') {
  const document = await vscode.workspace.openTextDocument({
    content: '',
    language: language,
  });
  let insertText = '';
  function emit(m: string) {
    insertText += m;
  }
  try {
    f(emit);
  } catch (e) {
    if (e instanceof MError) {
      insertText += `ERROR: ${e.message}`;
      insertText += `  ${e.location.range.start.line + 1}:`;
      insertText += `${e.location.range.start.column + 1}`;
    } else {
      console.log(e);
      throw e;
    }
  }
  const edit = new vscode.WorkspaceEdit();
  edit.insert(document.uri, new vscode.Position(0, 0), insertText);
  if (await vscode.workspace.applyEdit(edit)) {
    vscode.window.showTextDocument(document);
  }
}

function getSelectionOrAllText(editor: vscode.TextEditor) {
  const selection =
    editor.selection.isEmpty ?
      new vscode.Range(
        editor.document.lineAt(0).range.start,
        editor.document.lineAt(editor.document.lineCount - 1).range.end) :
      new vscode.Range(
        editor.selection.start,
        editor.selection.end);
  return editor.document.getText(selection);
}

export async function tokenize() {
  const editor = vscode.window.activeTextEditor;
  if (!editor) {
    return;
  }
  const text = getSelectionOrAllText(editor);
  const scanner = new MScanner('<input>', text);

  await writeToNewEditor(emit => {
    while (true) {
      const token = scanner.scanToken();
      emit(
        `${token.location.range.start.line + 1}:` +
        `${token.location.range.start.column + 1} - ` +
        `${token.location.range.end.line + 1}:` +
        `${token.location.range.end.column + 1} - ` +
        `${token.type} ` +
        `${token.value === null ? '' : JSON.stringify(token.value)}\n`);
      if (token.type === 'EOF') {
        break;
      }
    }
  });
}

export async function parse() {
  const editor = vscode.window.activeTextEditor;
  if (!editor) {
    return;
  }
  const text = getSelectionOrAllText(editor);
  await writeToNewEditor(emit => {
    const scanner = new MScanner('<input>', text);
    const parser = new MParser(scanner);
    const moduleAst = parser.parseModule();
    emit(JSON.stringify(moduleAst));
  }, 'json');
}
