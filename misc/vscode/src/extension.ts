import * as vscode from 'vscode';
import * as commands from './commands';
import { ExtensionContext, Location } from 'vscode';
import { Parser } from './lang/parser';
import { MScanner, MScannerError } from './lang/scanner';

export function activate(context: ExtensionContext) {
  console.log(`mtots extension activated! Parser = ${Parser}`);

  context.subscriptions.push(vscode.commands.registerCommand(
    'mtots.tokenize',
    commands.tokenize));
}
