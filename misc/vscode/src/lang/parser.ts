import * as ast from './ast';
import { Ast } from "./ast";
import { MError } from "./error";
import { MLocation } from './location';
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

  private advance(): MToken {
    const token = this.peek;
    this.peek = this.scanner.scanToken();
    return token;
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

  private parseIdentifier(): ast.Identifier {
    const token = this.expect('IDENTIFIER');
    return new ast.Identifier(token.location, <string>token.value);
  }

  private parseQualifiedIdentifier(): ast.QualifiedIdentifier {
    let rawIdentifier = this.parseIdentifier();
    let qualifiedIdentifier = new ast.QualifiedIdentifier(
      rawIdentifier.location, null, rawIdentifier);
    while (this.consume('.')) {
      const memberIdentifier = this.parseIdentifier();
      qualifiedIdentifier = new ast.QualifiedIdentifier(
        qualifiedIdentifier.location.merge(memberIdentifier.location),
        qualifiedIdentifier,
        memberIdentifier);
    }
    return qualifiedIdentifier;
  }

  private parseTypeExpression(): ast.TypeExpression {
    const identifier = this.parseQualifiedIdentifier();
    const args: ast.TypeExpression[] = [];
    let endLocation = identifier.location;
    if (this.consume('[')) {
      const args = [];
      while (!this.at(']')) {
        args.push(this.parseTypeExpression());
        if (!this.consume(',')) {
          break;
        }
      }
      endLocation = this.expect(']').location;
    }
    let location = identifier.location.merge(endLocation);
    let te = new ast.TypeExpression(location, identifier, args);
    if (this.at('?')) {
      const qmarkLocation = this.expect('?').location;
      const identifier = new ast.QualifiedIdentifier(
        qmarkLocation, null, new ast.Identifier(qmarkLocation, 'optional'));
      location = location.merge(qmarkLocation);
      te = new ast.TypeExpression(location, identifier, [te]);
    }
    if (this.at('|')) {
      const pipeLocation = this.expect('|').location;
      const identifer = new ast.QualifiedIdentifier(
        pipeLocation, null, new ast.Identifier(pipeLocation, 'union'));
      const rhs = this.parseTypeExpression();
      location = location.merge(rhs.location);
      te = new ast.TypeExpression(location, identifer, [te, rhs]);
    }
    return te;
  }

  private parseArguments(): Ast[] {
    const args: Ast[] = [];
    while (!this.at(')')) {
      args.push(this.parseExpression());
      if (!this.consume(',')) {
        break;
      }
    }
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
        const identifier = this.parseIdentifier();
        if (this.consume('=')) {
          const value = this.parseExpression();
          return new ast.SetVariable(location, identifier, value);
        } else {
          return new ast.GetVariable(location, identifier);
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
        const methodIdentifier = new ast.Identifier(
          location,
          tokenType === '~' ? '__not__' :
          tokenType === '-' ? '__neg__' :
          tokenType === '+' ? '__pos__' : 'invalid');
        this.advance();
        return new ast.MethodCall(
          location, this.parsePrec(PREC_UNARY_MINUS), methodIdentifier, []);
    }
    throw this.newError(
      `Expected expression but got '${this.peek.type}'`);
  }

  private parseInfix(lhs: ast.Expression, startLocation: MLocation): ast.Expression {
    const tokenType = this.peek.type;
    const precedence = PrecMap.get(tokenType);
    const methodName = BinopMethodMap.get(tokenType);
    if (!precedence) {
      throw this.newError('assertionError');
    }

    switch (tokenType) {
      case '.': {
        this.advance();
        const identifier = this.parseIdentifier();
        if (this.at('(')) {
          this.advance();
          const args = this.parseArguments();
          const endLocation = this.expect(')').location;
          const location = startLocation.merge(endLocation);
          return new ast.MethodCall(location, lhs, identifier, args);
        } else if (this.consume('=')) {
          const value = this.parseExpression();
          const location = startLocation.merge(value.location);
          return new ast.SetField(location, lhs, identifier, value);
        }
        const location = startLocation.merge(identifier.location);
        return new ast.GetField(location, lhs, identifier);
      }
      case '[': {
        const openBracketLocation = this.advance().location;
        const index = this.parseExpression();
        const closeBracketLocation = this.expect(']').location;
        if (this.consume('=')) {
          const value = this.parseExpression();
          const location = startLocation.merge(value.location);
          const identifier = new ast.Identifier(
            openBracketLocation, '__setitem__');
          return new ast.MethodCall(
            location, lhs, identifier, [index, value]);
        }
        const location = startLocation.merge(closeBracketLocation);
        const identifier = new ast.Identifier(
          openBracketLocation, '__getitem__');
        return new ast.MethodCall(location, lhs, identifier, [index]);
      }
      case '(': {
        this.advance();
        const args = this.parseArguments();
        const closeParenLocation = this.expect(')').location;
        const location = startLocation.merge(closeParenLocation);
        return new ast.FunctionCall(location, lhs, args);
      }
    }

    if (methodName) {
      const operatorLocation = this.advance().location;
      const rhs = this.parsePrec(precedence + 1);
      const location = startLocation.merge(rhs.location);
      const methodIdentifier = new ast.Identifier(
        operatorLocation, methodName);
      return new ast.MethodCall(location, lhs, methodIdentifier, [rhs]);
    }
    if (tokenType === 'and' || tokenType === 'or') {
      this.advance();
      const rhs = this.parsePrec(precedence + 1);
      const location = startLocation.merge(rhs.location);
      return new ast.Logical(location, tokenType, [lhs, rhs]);
    }
    throw this.newError(`assertionError ${tokenType}`);
  }

  private parsePrec(precedence: number): ast.Expression {
    const startLocation = this.peek.location;
    let expr = this.parsePrefix();
    while (precedence <= (PrecMap.get(this.peek.type) || 0)) {
      expr = this.parseInfix(expr, startLocation);
    }
    return expr;
  }

  private parseExpression(): ast.Expression {
    return this.parsePrec(1);
  }

  private parseForStatement(): Ast {
    const startLocation = this.expect('for').location;
    const identifier = this.parseIdentifier();
    this.expect('in');
    const container = this.parseExpression();
    const body = this.parseBlock();
    const location = startLocation.merge(body.location);
    return new ast.For(location, identifier, container, body);
  }

  private parseIfStatement(): Ast {
    const startLocation = this.expect('if').location;
    const pairs: [ast.Expression, ast.Block][] = [];
    pairs.push([this.parseExpression(), this.parseBlock()]);
    while (this.consume('elif')) {
      pairs.push([this.parseExpression(), this.parseBlock()]);
    }
    const fallback = this.consume('else') ? this.parseBlock() : null;
    const location = startLocation.merge(
      fallback ? fallback.location : pairs[pairs.length - 1][1].location);
    return new ast.If(location, pairs, fallback);
  }

  private parseReturnStatement(): Ast {
    const startLocation = this.expect('return').location;
    const expression = this.parseExpression();
    const location = startLocation.merge(expression.location);
    this.expectStatementDelimiter();
    return new ast.Return(location, expression);
  }

  private parseWhileStatement(): Ast {
    const startLocation = this.expect('while').location;
    const condition = this.parseExpression();
    const body = this.parseBlock();
    const location = startLocation.merge(body.location);
    return new ast.While(location, condition, body);
  }

  private parseImportStatement(): Ast {
    const startLocation = this.peek.location;
    this.expect('import');
    const module = this.parseQualifiedIdentifier();
    const alias = this.consume('as') ? this.parseIdentifier() : null;
    const location = startLocation.merge(
      alias === null ? module.location : alias.location);
    return new ast.Import(location, module, alias);
  }

  private parseExpressionStatement(): Ast {
    const startLocation = this.peek.location;
    const expression = this.parseExpression();
    const location = startLocation.merge(expression.location);
    this.expectStatementDelimiter();
    return new ast.ExpressionStatement(location, expression);
  }

  private parseBlock(): ast.Block {
    const startLocation = this.expect(':').location;
    while (this.consume('NEWLINE') || this.consume(';'));
    this.expect('INDENT');
    const statements: ast.Statement[] = [];
    while (this.consume('NEWLINE') || this.consume(';'));
    while (!this.at('DEDENT')) {
      statements.push(this.parseDeclaration());
      while (this.consume('NEWLINE') || this.consume(';'));
    }
    const location = startLocation.merge(this.expect('DEDENT').location);
    while (this.consume('NEWLINE') || this.consume(';'));
    return new ast.Block(location, statements);
  }

  private parseStatement(): ast.Statement {
    switch (this.peek.type) {
      case 'for': return this.parseForStatement();
      case 'if': return this.parseIfStatement();
      case 'return': return this.parseReturnStatement();
      case 'while': return this.parseWhileStatement();
      case 'import': return this.parseImportStatement();
      case 'NEWLINE':
      case ';':
      case 'pass':
        const node = new ast.Nop(this.peek.location);
        while (this.consume('pass'));
        this.expectStatementDelimiter();
        while (this.consume('NEWLINE') || this.consume(';'));
        return node;
      default: return this.parseExpressionStatement();
    }
  }

  private parseFieldDeclaration(): ast.Field {
    const startLocation = this.peek.location;
    const final = this.consume('final');
    if (!final) {
      this.expect('var');
    }
    const identifier = this.parseIdentifier();
    const type = this.parseTypeExpression();
    const location = startLocation.merge(type.location);
    this.expectStatementDelimiter();
    return new ast.Field(location, final, identifier, type);
  }

  private parseClassDeclaration(): ast.Class {
    const startLocation = this.expect('class').location;
    const identifier = this.parseIdentifier();
    const bases = [];
    if (this.consume('(')) {
      bases.push(...this.parseArguments());
      this.expect(')');
    }
    this.expect(':');
    while (this.consume('NEWLINE') || this.consume(';'));
    this.expect('INDENT');
    while (this.consume('NEWLINE') || this.consume(';'));
    let documentation: ast.StringLiteral | null = null;
    if (this.at('STRING')) {
      const token = this.expect('STRING');
      documentation = new ast.StringLiteral(
        token.location, <string>token.value);
    }
    while (this.consume('NEWLINE') || this.consume(';'));
    while (this.consume('pass'));
    while (this.consume('NEWLINE') || this.consume(';'));
    const fields = [];
    while (this.at('var') || this.at('final')) {
      fields.push(this.parseFieldDeclaration());
      while (this.consume('NEWLINE') || this.consume(';'));
    }
    const methods = [];
    while (this.at('def')) {
      methods.push(this.parseFunctionDeclaration());
      while (this.consume('NEWLINE') || this.consume(';'));
    }
    const endLocation = this.expect('DEDENT').location;
    while (this.consume('NEWLINE') || this.consume(';'));
    const location = startLocation.merge(endLocation);

    return new ast.Class(
      location, identifier, bases, documentation, fields, methods);
  }

  private parseParameter(): ast.Parameter {
    const identifier = this.parseIdentifier();
    const type = this.at('IDENTIFIER') ?
      this.parseTypeExpression() :
      this.newAnyType(identifier.location);
    const defaultValue = this.consume('=') ? this.parseExpression() : null;
    const location = identifier.location.merge(
      defaultValue ? defaultValue.location : type.location);
    return new ast.Parameter(location, identifier, type, defaultValue);
  }

  private parseFunctionDeclaration(): ast.Function {
    const startLocation = this.expect('def').location;
    const identifier = this.parseIdentifier();
    const parameters = [];
    this.expect('(');
    while (!this.at(')')) {
      parameters.push(this.parseParameter());
      if (!this.consume(',')) {
        break;
      }
    }
    this.expect(')');
    const returnType = this.at(':') ?
      this.newAnyType(startLocation) :
      this.parseTypeExpression();
    const body = this.parseBlock();
    const location = startLocation.merge(body.location);
    return new ast.Function(location, identifier, parameters, returnType, body);
  }

  private newAnyType(location: MLocation): ast.TypeExpression {
    return new ast.TypeExpression(
      location,
      new ast.QualifiedIdentifier(
        location,
        null,
        new ast.Identifier(location, 'any')
      ),
      []);
  }

  private parseVarDeclaration(): ast.Variable {
    const startLocation = this.peek.location;
    const final = this.consume('final');
    if (!final) this.expect('var');
    const identifier = this.parseIdentifier();
    const type = this.at('=') ?
      this.newAnyType(startLocation) :
      this.parseTypeExpression();
    const value = this.parseExpression();
    const location = startLocation.merge(value.location);
    return new ast.Variable(location, final, identifier, type, value);
  }

  private parseDeclaration(): ast.Statement {
    switch (this.peek.type) {
      case 'class': return this.parseClassDeclaration();
      case 'def': return this.parseFunctionDeclaration();
      case 'final': case 'var': return this.parseVarDeclaration();
      default: return this.parseStatement();
    }
  }

  parseProgram() {
    while (!this.at('EOF')) {
      this.parseDeclaration();
    }
  }
}
