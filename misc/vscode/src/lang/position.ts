
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

  lt(other: MPosition): boolean {
    return this.line === other.line ?
        this.column < other.column :
        this.line < other.line;
  }

  le(other: MPosition): boolean {
    return this.line === other.line ?
        this.column <= other.column :
        this.line <= other.line;
  }

  equals(other: MPosition): boolean {
    return this.line === other.line && this.column === other.column;
  }

  toString() {
    return `MPosition(${this.line}, ${this.column})`;
  }
}
