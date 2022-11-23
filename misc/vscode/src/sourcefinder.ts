import { join, sep } from "path";
import { Uri } from "vscode";
import { SourceFinder } from "./lang/parser";
import * as vscode from "vscode";


const roots = [
  process.env.MTOTS_ROOT || '',
  process.env.MTOTS_AUX_ROOT || '',
  process.env.MTOTS_LIB_ROOT || '',
  process.env.MTOTS_STDLIB_ROOT || '',
  join(process.env.HOME || process.env.USERPROFILE || '/', 'git', 'mtots', 'root'),
].filter(s => s.length > 0);


export const DefaultSourceFinder: SourceFinder = async path => {
  const relativeFilePath = join(...path.split('.'));
  for (const root of roots) {
    for (const extension of ['.types.mtots', '.mtots']) {
      const filePath = join(root, relativeFilePath + extension);
      const uri = Uri.file(filePath);
      const document = await vscode.workspace.openTextDocument(uri);
      const contents = document.getText();
      return [document.uri, contents];
    }
  }
  return null;
}
