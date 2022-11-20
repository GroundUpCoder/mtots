import { Uri } from "vscode";
import { MRange } from "./range";


export class MLocation {
  filePath: string | Uri;
  range: MRange;

  constructor(filePath: string | Uri, range: MRange) {
    this.filePath = filePath;
    this.range = range;
  }

  merge(other: MLocation): MLocation {
    if (this.filePath !== other.filePath) {
      throw new Error(`assertionError ${this.filePath}, ${other.filePath}`);
    }
    return new MLocation(
      this.filePath,
      this.range.merge(other.range));
  }
}
