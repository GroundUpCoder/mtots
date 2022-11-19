import * as ast from './ast';
import { MAst } from "./ast";
import { MError } from "./error";
import { MScanner } from "./scanner";
import { MScope } from "./scope";
import { MSymbolTable } from "./symbol";
import { MToken, MTokenType } from "./token";


export class MParser {
  symbolTable: MSymbolTable;
  filePath: string;
  scanner: MScanner;
  scope: MScope;
  peek: MToken;

  constructor(symbolTable: MSymbolTable, filePath: string, scanner: MScanner) {
    this.symbolTable = symbolTable;
    this.filePath = filePath;
    this.scanner = scanner;
    this.scope = new MScope();
    this.peek = scanner.scanToken();
  }

  newError(message: string) {
    throw new MError(this.peek.location, message);
  }

  advance() {
    this.peek = this.scanner.scanToken();
  }

  at(tokenType: MTokenType): boolean {
    return this.peek.type === tokenType;
  }

  consume(tokenType: MTokenType): boolean {
    if (this.at(tokenType)) {
      this.advance();
      return true;
    }
    return false;
  }

  expect(tokenType: MTokenType, message: string | null = null) {
    if (!this.consume(tokenType)) {
      if (!message) {
        message = `Expected '${tokenType}'`;
      }
      throw this.newError(`${message} but got '${this.peek.type}'`);
    }
  }

  expectStatementDelimiter(message: string | null = null) {
    if (!this.consume('NEWLINE')) {
      this.expect(';', message);
    }
  }

  parseForStatement(): MAst {
    throw this.newError('TODO: parseForStatement');
  }

  parseIfStatement(): MAst {
    throw this.newError('TODO: parseIfStatement');
  }

  parseReturnStatement(): MAst {
    throw this.newError('TODO: parseReturnStatement');
  }

  parseWhileStatement(): MAst {
    throw this.newError('TODO: parseWhileStatement');
  }

  parseImportStatement(): MAst {
    throw this.newError('TODO: parseImportStatement');
  }

  parseExpressionStatement(): MAst {
    throw this.newError('TODO: parseExpressionStatement');
  }

  parseStatement(): MAst {
    if (this.at('for')) {
      return this.parseForStatement();
    } else if (this.at('if')) {
      return this.parseIfStatement();
    } else if (this.at('return')) {
      return this.parseReturnStatement();
    } else if (this.at('while')) {
      return this.parseWhileStatement();
    } else if (this.at('import')) {
      return this.parseImportStatement();
    } else if (this.consume('NEWLINE') || this.consume(';')) {
      return new ast.Nop(this.peek.location);
    } else if (this.at('pass')) {
      const node = new ast.Nop(this.peek.location);
      this.advance();
      this.expectStatementDelimiter(
        'Expected statement delimiter at end of pass statement');
      return node;
    } else {
      return this.parseStatement();
    }
  }

  parseClassDeclaration(): MAst {
    throw this.newError('TODO: parseClassDeclaration');
  }

  parseFunctionDeclaration(): MAst {
    throw this.newError('TODO: parseFunctionDeclaration');
  }

  parseVarDeclaration(): MAst {
    throw this.newError('TODO: parseVarDeclaration');
  }

  parseDeclaration(): MAst {
    if (this.at('class')) {
      return this.parseClassDeclaration();
    } else if (this.at('def')) {
      return this.parseFunctionDeclaration();
    } else if (this.at('var')) {
      return this.parseVarDeclaration();
    } else {
      return this.parseStatement();
    }
  }

  parseProgram() {
    while (!this.at('EOF')) {
      this.parseDeclaration();
    }
  }
}
