import { Uri } from 'vscode';
import { MError } from './error';
import { MLocation } from './location';
import { MPosition } from './position';
import { MRange } from './range';
import { KeywordsMap, MToken, MTokenType, MTokenValue, SymbolsMap } from './token'


const reAlpha = /[a-zA-Z_]/;
const reDigit = /[0-9]/;
const reHexDigit = /[0-9A-Fa-f]/;

function isAlpha(ch: string): boolean {
  return reAlpha.test(ch);
}

function isDigit(ch: string): boolean {
  return reDigit.test(ch);
}

function isHexDigit(ch: string): boolean {
  return reHexDigit.test(ch);
}

export class MScannerError extends Error {
  range: MRange;

  constructor(range: MRange, message: string) {
    super(message);
    this.range = range;
  }
}

export class MScanner {
  filePath: string | Uri;
  s: string;
  i: number;
  startI: number;             /* current token start index */
  startPosition: MPosition;   /* current token start position */
  position: MPosition;

  /**
   * Keeps track of how many parentheses, brackets or braces
   * we are currently nested in.
   * Newlines are ignored when groupingDepth > 0
   */
  depth: number;

  /**
   * The indentation level of the current line.
   * Used to determine how many INDENT or DEDENT tokens
   * need to be emitted when processing the start of the next
   * line
   */
  indentationLevel: number;

  /**
   * non-zero potential indicates pending INDENT or DEDENT tokens
   */
  indentationPotential: number;

  /**
   * Indicates whether a synthetic newline has been emitted yet.
   * This is always done at the end to ensure a final NEWLINE token
   * and all matching DEDENT tokens have been emitted.
   */
  processedSyntheticNewline: boolean;

  constructor(filePath: string | Uri, s: string) {
    this.filePath = filePath;
    this.s = s;
    this.i = 0;
    this.startI = 0;
    this.startPosition = new MPosition(0, 0);
    this.position = new MPosition(0, 0);
    this.depth = 0;
    this.indentationLevel = 0;
    this.indentationPotential = 0;
    this.processedSyntheticNewline = false;
  }

  newError(message: string) {
    return new MError(
      new MLocation(this.filePath, this.makeRange()),
      message);
  }

  getch(): string {
    return this.s[this.i];
  }

  isAtEnd(): boolean {
    return this.i >= this.s.length;
  }

  advance(): string {
    const c = this.s[this.i++];
    if (c === '\n') {
      this.position.line++;
      this.position.column = 0;
    } else {
      this.position.column++;
    }
    return c;
  }

  peek(): string {
    return this.s[this.i];
  }

  peekNext(): string {
    if (this.isAtEnd()) {
      return '\0';
    }
    return this.s[this.i + 1];
  }

  match(expected: string): boolean {
    if (this.isAtEnd()) {
      return false;
    }
    if (this.getch() === expected) {
      return false;
    }
    this.advance();
    return true;
  }

  makeRange(): MRange {
    return new MRange(this.startPosition, this.position);
  }

  makeLocation(): MLocation {
    return new MLocation(this.filePath, this.makeRange());
  }

  makeToken(type: MTokenType, value: MTokenValue=null): MToken {
    return new MToken(this.makeLocation(), type, value);
  }

  skipWhitespace() {
    while (true) {
      const c = this.peek();
      switch (c) {
        case ' ':
        case '\r':
        case '\t':
          this.advance();
          break;
        case '\n':
          // If we are nested inside (), [] or {}, we
          // skip newlines
          if (this.depth > 0) {
            this.advance();
            break;
          }
          // Otherwise, we cannot skip newlines
          return;
        case '#':
          // A comment goes unti lthe end of the line
          while (this.peek() !== '\n' && !this.isAtEnd()) {
            this.advance();
          }
          break;
        default:
          return;
      }
    }
  }

  sliceTokenString(): string {
    return this.s.slice(this.startI, this.i);
  }

  scanIdentifier(): MToken {
    while (isAlpha(this.peek()) || isDigit(this.peek())) {
      this.advance();
    }
    const identifier = this.sliceTokenString();
    const keywordType = KeywordsMap.get(identifier);
    if (keywordType) {
      return this.makeToken(keywordType);
    }
    return this.makeToken('IDENTIFIER', identifier);
  }

  scanNumber(): MToken {
    if (this.s[this.startI] === '0') {
      if (this.match('x')) {
        while (isHexDigit(this.peek())) {
          this.advance();
        }
        const value = parseInt(this.sliceTokenString(), 16);
        return this.makeToken('NUMBER', value);
      } else if (this.match('b')) {
        while (this.peek() === '0' || this.peek() === '1') {
          this.advance();
        }
        const value = parseInt(this.sliceTokenString().slice(2), 2);
        return this.makeToken('NUMBER', value);
      }
    }
    while (isDigit(this.peek())) {
      this.advance();
    }
    if (this.peek() === '.' && isDigit(this.peekNext())) {
      this.advance(); // consume the '.'
      while (isDigit(this.peek())) {
        this.advance();
      }
    }
    const value = parseFloat(this.sliceTokenString());
    return this.makeToken('NUMBER', value);
  }

  scanString(quote: string): MToken {
    while (this.peek() !== quote && !this.isAtEnd()) {
      if (this.peek() === '\\') {
        this.advance();
        if (this.isAtEnd()) {
          throw this.newError(
            "Expected string escape but got EOF");
        }
      }
      this.advance();
    }
    if (this.isAtEnd()) {
      throw this.newError("Unterminated string");
    }
    this.advance(); // closing quote
    const unescapedString = this.sliceTokenString();
    const value = eval(unescapedString);
    return this.makeToken('STRING', value);
  }

  scanRawString(quote: string): MToken {
    while (this.peek() !== quote && !this.isAtEnd()) {
      this.advance();
    }
    if (this.isAtEnd()) {
      throw this.newError(
        "Unterminated raw string literal");
    }
    const value = this.sliceTokenString().slice(1);
    this.advance(); // closing quote
    return this.makeToken('STRING', value);
  }

  scanRawTripleQuoteString(quote: string): MToken {
    let stringStartI = this.i;
    let stringEndI = this.i;
    let quoteRun = 0;
    while (quoteRun < 3 && !this.isAtEnd()) {
      if (this.peek() === quote) {
        if (quoteRun === 0) {
          stringEndI = this.i;
        }
        quoteRun++;
      } else {
        quoteRun = 0;
      }
      this.advance();
    }
    if (this.isAtEnd()) {
      throw this.newError(
        "Unterminated raw triple quote string literal");
    }
    const value = this.s.slice(stringStartI, stringEndI);
    return this.makeToken('STRING', value);
  }

  scanToken(): MToken {
    if (this.indentationPotential > 0) {
      this.indentationPotential--;
      return this.makeToken('INDENT');
    }
    if (this.indentationPotential < 0) {
      this.indentationPotential++;
      return this.makeToken('DEDENT');
    }
    this.skipWhitespace();
    this.startI = this.i;
    this.startPosition = this.position.clone();

    if (this.isAtEnd()) {
      if (!this.processedSyntheticNewline) {
        this.indentationPotential = -this.indentationLevel;
        this.processedSyntheticNewline = true;
        return this.makeToken('NEWLINE');
      }
      return this.makeToken('EOF');
    }

    const c = this.advance();

    if (c === 'r' && (this.peek() === '"' || this.peek() === "'")) {
      const quote = this.advance();
      if (this.peek() === quote) {
        this.advance();
        if (this.peek() === quote) {
          this.advance();
          return this.scanRawTripleQuoteString(quote);
        } else {
          return this.makeToken('STRING', '');
        }
      }
      return this.scanRawString(quote);
    }

    if (isAlpha(c)) {
      return this.scanIdentifier();
    }

    if (isDigit(c)) {
      return this.scanNumber();
    }

    const two = c + this.peek();
    const twoCharSymbol = SymbolsMap.get(two);
    if (twoCharSymbol) {
      this.advance();
      return this.makeToken(twoCharSymbol);
    }
    const oneCharSymbol = SymbolsMap.get(c);
    if (oneCharSymbol) {
      return this.makeToken(oneCharSymbol);
    }

    if (c === '"' || c === "'") {
      return this.scanString(c);
    }

    if (c === '\n') {
      const newlineToken = this.makeToken('NEWLINE');

      while (this.peek() === '\n' || this.peek() === '\r') {
        this.advance();
      }

      let spaceCount = 0;
      while (this.peek() === ' ') {
        this.advance();
        spaceCount++;
      }
      if (spaceCount % 2 === 1) {
        throw this.newError(
          "Indentations must always be a multiple of 2");
      }
      const newIndentationLevel = spaceCount / 2;
      this.indentationPotential = newIndentationLevel - this.indentationLevel;
      this.indentationLevel = newIndentationLevel;

      return newlineToken;
    }

    throw this.newError(
      `Unexpected character ${JSON.stringify(c)}`);
  }
}
