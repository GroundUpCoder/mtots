import * as vscode from 'vscode';
import * as commands from './commands';
import { ExtensionContext } from 'vscode';
import { definitionProvider } from './definitionprovider';
import { hoverProvider } from './hoverprovider';
import { initDiagnostic } from './diagnostics';

export function activate(context: ExtensionContext) {
  context.subscriptions.push(vscode.commands.registerCommand(
    'mtots.tokenize',
    commands.tokenize));
  context.subscriptions.push(vscode.commands.registerCommand(
    'mtots.parse',
    commands.parse));
  context.subscriptions.push(vscode.languages.registerDefinitionProvider(
    { language: 'mtots' },
    definitionProvider));
  context.subscriptions.push(vscode.languages.registerHoverProvider(
    { language: 'mtots' },
    hoverProvider));
  initDiagnostic(context);
}
