import * as vscode from 'vscode';
import * as commands from './commands';
import { ExtensionContext } from 'vscode';

export function activate(context: ExtensionContext) {
  context.subscriptions.push(vscode.commands.registerCommand(
    'mtots.tokenize',
    commands.tokenize));
}
