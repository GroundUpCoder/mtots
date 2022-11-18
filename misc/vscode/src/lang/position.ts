
export class MPosition {

  /** Line number, zero indexed */
  line: number;

  /** Column number, zero indexed */
  column: number;

  constructor(line: number, column: number) {
    this.line = line;
    this.column = column;
  }

  clone(): MPosition {
    return new MPosition(this.line, this.column);
  }
}
