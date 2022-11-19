import { Uri } from "vscode";
import { MRange } from "./range";


export class MLocation {
  filePath: string | Uri;
  range: MRange;

  constructor(filePath: string | Uri, range: MRange) {
    this.filePath = filePath;
    this.range = range;
  }
}
