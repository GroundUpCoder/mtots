import { join } from "path";
import { Uri } from "vscode";
import { SourceFinder } from "./lang/context";
import * as vscode from "vscode";


const roots = [
  process.env.MTOTS_ROOT || '',
  process.env.MTOTS_AUX_ROOT || '',
  process.env.MTOTS_LIB_ROOT || '',
  process.env.MTOTS_STDLIB_ROOT || '',
  join(process.env.HOME || process.env.USERPROFILE || '/', 'git', 'mtots', 'root'),
].filter(s => s.length > 0);


async function openDocument(uri: Uri): Promise<vscode.TextDocument | null> {
  try {
    return await vscode.workspace.openTextDocument(uri);
  } catch (e) {
    // Assume that the document does not exist
    return null;
  }
}


export const DefaultSourceFinder: SourceFinder = async (path, oldUriAndVersion) => {
  const relativeFilePath = join(...path.split('.'));
  for (const root of roots) {
    for (const extension of ['.types.mtots', '.mtots']) {
      const filePath = join(root, relativeFilePath + extension);
      const uri = Uri.file(filePath);
      const document = await openDocument(uri);
      if (document === null) {
        continue;
      }
      if (oldUriAndVersion) {
        const [oldUri, oldVersion] = oldUriAndVersion;
        if (oldUri.toString() === document.uri.toString() && oldVersion === document.version) {
          return 'useCached';
        }
      }
      const contents = document.getText();
      return { uri: document.uri, contents, version: document.version};
    }
  }
  return null;
}
