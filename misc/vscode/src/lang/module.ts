import { File } from "./ast";
import { CompletionPoint } from "./completion";
import { MError } from "./error";
import { MPosition } from "./position";
import { MScope } from "./scope";
import { MSignatureHelper } from "./sighelp";
import { MSymbolUsage } from "./symbol";

export class MModule {
  readonly version: number;
  readonly file: File;
  readonly scope: MScope;
  readonly symbolUsages: MSymbolUsage[];
  readonly completionPoints: CompletionPoint[];
  readonly signatureHelpers: MSignatureHelper[];
  readonly errors: MError[];
  constructor(version: number, file: File, scope: MScope) {
    this.version = version;
    this.file = file;
    this.scope = scope;
    this.symbolUsages = [];
    this.completionPoints = [];
    this.signatureHelpers = [];
    this.errors = [...file.syntaxErrors];
  }

  findUsage(position: MPosition): MSymbolUsage | null {
    for (const usage of this.symbolUsages) {
      if (usage.location.range.contains(position)) {
        return usage;
      }
    }
    return null;
  }

  findCompletionPoint(position: MPosition): CompletionPoint | null {
    for (const cp of this.completionPoints) {
      const cpRange = cp.location.range;
      if (cpRange.start.le(position) && position.le(cpRange.end)) {
        return cp;
      }
    }
    return null;
  }

  findSignatureHelper(position: MPosition): MSignatureHelper | null {
    let bestSoFar: MSignatureHelper | null = null;
    for (const sh of this.signatureHelpers) {
      const shRange = sh.location.range;
      if (shRange.start.le(position) && position.le(shRange.end) &&
          (bestSoFar == null ||
            bestSoFar.location.range.start.lt(shRange.start))) {
        bestSoFar = sh;
      }
    }
    return bestSoFar;
  }
}
