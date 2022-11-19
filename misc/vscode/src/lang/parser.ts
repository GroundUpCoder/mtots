import { MScanner } from "./scanner";
import { MSymbolTable } from "./symbol";


export class MParser {
  symbolTable: MSymbolTable;
  filePath: string;
  scanner: MScanner;

  constructor(symbolTable: MSymbolTable, filePath: string, scanner: MScanner) {
    this.symbolTable = symbolTable;
    this.filePath = filePath;
    this.scanner = scanner;
  }
}
