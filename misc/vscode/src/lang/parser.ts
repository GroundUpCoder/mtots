import * as ast from './ast';
import { Ast } from "./ast";
import { MError } from "./error";
import { MScanner } from "./scanner";
import { MScope } from "./scope";
import { MSymbolTable } from "./symbol";
import { MToken, MTokenType } from "./token";

const PrecList: MTokenType[][] = [
  [],
  ['or'],
  ['and'],
  [],        // precedence for unary operator 'not'
  ['==', '!=', '<', '>', '<=', '>=', 'in', 'not', 'is'],
  ['<<', '>>'],
  ['&'],
  ['^'],
  ['|'],
  ['+', '-'],
  ['*', '/', '//', '%'],
  [],        // precedence for unary operators '-', '+' and '~'
  ['.', '(', '['],
];
const PrecMap: Map<MTokenType, number> = new Map();
for (let i = 0; i < PrecList.length; i++) {
  for (const tokenType of PrecList[i]) {
    PrecMap.set(tokenType, i);
  }
}
const PREC_UNARY_NOT = PrecMap.get('and')! + 1;
const PREC_UNARY_MINUS = PrecMap.get('*')! + 1;
const BinopMethodMap: Map<MTokenType, string> = new Map([
  ['==', '__eq__'],
  ['!=', '__ne__'],
  ['<', '__lt__'],
  ['<=', '__le__'],
  ['>', '__gt__'],
  ['>=', '__ge__'],
  ['in', '__contains__'],
  ['<<', '__lshift__'],
  ['>>', '__rshift__'],
  ['&', '__and__'],
  ['^', '__xor__'],
  ['|', '__or__'],
  ['+', '__add__'],
  ['-', '__sub__'],
  ['*', '__mul__'],
  ['/', '__div__'],
  ['//', '__floordiv__'],
  ['%', '__mod__'],
])

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

  private newError(message: string) {
    throw new MError(this.peek.location, message);
  }

  private advance() {
    this.peek = this.scanner.scanToken();
  }

  private at(tokenType: MTokenType): boolean {
    return this.peek.type === tokenType;
  }

  private consume(tokenType: MTokenType): boolean {
    if (this.at(tokenType)) {
      this.advance();
      return true;
    }
    return false;
  }

  private expect(tokenType: MTokenType, message: string | null = null): MToken {
    const token = this.peek;
    if (!this.consume(tokenType)) {
      if (!message) {
        message = `Expected '${tokenType}'`;
      }
      throw this.newError(`${message} but got '${this.peek.type}'`);
    }
    return token;
  }

  private expectStatementDelimiter(message: string | null = null) {
    if (!this.consume('NEWLINE')) {
      this.expect(';', message);
    }
  }

  private parsePrimary(): Ast {
    const location = this.peek.location;
    if (this.consume('(')) {
      const expression = this.parseExpression();
      this.expect(')');
      return expression;
    } else if (this.at('NUMBER')) {
      const expression = new ast.NumberLiteral(
        location, <number>this.peek.value);
      this.advance();
      return expression;
    } else if (this.at('STRING')) {
      const expression = new ast.StringLiteral(
        location, <string>this.peek.value);
      this.advance();
      return expression;
    } else if (this.consume('true')) {
      return new ast.BoolLiteral(location, true);
    } else if (this.consume('false')) {
      return new ast.BoolLiteral(location, false);
    } else if (this.consume('nil')) {
      return new ast.NilLiteral(location, null);
    } else if (this.at('IDENTIFIER')) {
      const name = <string>this.peek.value;
      this.advance();
      if (this.consume('=')) {
        const value = this.parseExpression();
        return new ast.SetVariable(location, name, value);
      } else {
        return new ast.GetVariable(location, name);
      }
    }
    throw this.newError(
      `Expected expression but got '${this.peek.type}'`);
  }

  private parseArguments(): Ast[] {
    const args: Ast[] = [];
    this.expect('(');
    while (!this.at(')')) {
      args.push(this.parseExpression());
      if (!this.consume(',')) {
        break;
      }
    }
    this.expect(')');
    return args;
  }

  private parsePrefix(): ast.Expression {
    const location = this.peek.location;
    switch (this.peek.type) {
      case '(': {
        this.advance();
        const expression = this.parseExpression();
        this.expect(')');
        return expression;
      }
      case 'NUMBER': {
        const expression = new ast.NumberLiteral(
          location, <number>this.peek.value);
        this.advance();
        return expression;
      }
      case 'STRING': {
        const expression = new ast.StringLiteral(
          location, <string>this.peek.value);
        this.advance();
        return expression;
      }
      case 'true': {
        this.advance();
        return new ast.BoolLiteral(location, true);
      }
      case 'false': {
        this.advance();
        return new ast.BoolLiteral(location, false);
      }
      case 'nil': {
        this.advance();
        return new ast.NilLiteral(location, null);
      }
      case 'IDENTIFIER': {
        const name = <string>this.peek.value;
        this.advance();
        if (this.consume('=')) {
          const value = this.parseExpression();
          return new ast.SetVariable(location, name, value);
        } else {
          return new ast.GetVariable(location, name);
        }
      }
      case 'not': {
        this.advance();
        return new ast.Logical(location, 'not', [
          this.parsePrec(PREC_UNARY_NOT)]);
      }
      case '~':
      case '-':
      case '+':
        const tokenType = this.peek.type;
        let methodName = 'invalid';
        switch (tokenType) {
          case '~':
            methodName = '__not__';
            break;
          case '-':
            methodName = '__neg__';
            break;
          case '+':
            methodName = '__pos__';
            break;
          default:
            throw this.newError(`assertionError ${tokenType}`);
        }
        this.advance();
        return new ast.MethodCall(
          location, this.parsePrec(PREC_UNARY_MINUS), methodName, []);
    }
    throw this.newError(
      `Expected expression but got '${this.peek.type}'`);
  }

  parseInfix(lhs: ast.Expression): ast.Expression {
    const location = this.peek.location;
    const tokenType = this.peek.type;
    const precedence = PrecMap.get(tokenType);
    const methodName = BinopMethodMap.get(tokenType);
    if (!precedence) {
      throw this.newError('assertionError');
    }

    switch (tokenType) {
      case '.': {
        this.advance();
        const token = this.expect(
          'IDENTIFIER', 'Expected field or method identifier');
        const nameLocation = token.location;
        const name = <string>token.value;
        if (this.at('(')) {
          const args = this.parseArguments();
          return new ast.MethodCall(nameLocation, lhs, name, args);
        } else if (this.consume('=')) {
          const value = this.parseExpression();
          return new ast.SetField(nameLocation, lhs, name, value);
        }
        return new ast.GetField(nameLocation, lhs, name);
      }
      case '[': {
        this.advance();
        const index = this.parseExpression();
        this.expect(']');
        if (this.consume('=')) {
          const value = this.parseExpression();
          return new ast.MethodCall(
            location, lhs, '__setitem__', [index, value]);
        }
        return new ast.MethodCall(location, lhs, '__getitem__', [index]);
      }
      case '(': {
        this.advance();
        const args = this.parseArguments();
        return new ast.FunctionCall(location, lhs, args);
      }
    }

    if (methodName) {
      this.advance();
      const rhs = this.parsePrec(precedence + 1);
      return new ast.MethodCall(location, lhs, methodName, [rhs]);
    }
    if (tokenType === 'and' || tokenType === 'or') {
      this.advance();
      const rhs = this.parsePrec(precedence + 1);
      return new ast.Logical(location, tokenType, [lhs, rhs]);
    }
    throw this.newError(`assertionError ${tokenType}`);
  }

  parsePrec(precedence: number): ast.Expression {
    let expr = this.parsePrefix();
    while (precedence <= (PrecMap.get(this.peek.type) || 0)) {
      expr = this.parseInfix(expr);
    }
    return expr;
  }

  parseExpression(): Ast {
    return this.parsePrec(1);
  }

  parseForStatement(): Ast {
    throw this.newError('TODO: parseForStatement');
  }

  parseIfStatement(): Ast {
    throw this.newError('TODO: parseIfStatement');
  }

  parseReturnStatement(): Ast {
    throw this.newError('TODO: parseReturnStatement');
  }

  parseWhileStatement(): Ast {
    throw this.newError('TODO: parseWhileStatement');
  }

  parseImportStatement(): Ast {
    throw this.newError('TODO: parseImportStatement');
  }

  parseExpressionStatement(): Ast {
    throw this.newError('TODO: parseExpressionStatement');
  }

  parseStatement(): Ast {
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
      return this.parseExpressionStatement();
    }
  }

  parseClassDeclaration(): Ast {
    throw this.newError('TODO: parseClassDeclaration');
  }

  parseFunctionDeclaration(): Ast {
    throw this.newError('TODO: parseFunctionDeclaration');
  }

  parseVarDeclaration(): Ast {
    throw this.newError('TODO: parseVarDeclaration');
  }

  parseDeclaration(): Ast {
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
