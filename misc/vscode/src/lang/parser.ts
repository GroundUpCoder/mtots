import * as ast from './ast';
import { MError } from "./error";
import { MLocation } from './location';
import { MRange } from './range';
import { MScanner } from "./scanner";
import { MToken, MTokenType } from "./token";

const PrecList: MTokenType[][] = [
  [],
  ['or'],
  ['and'],
  [],        // precedence for unary operator 'not'
  ['==', '!=', '<', '>', '<=', '>=', 'in', 'not', 'is', 'as'],
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
]);

export class MParser {
  private readonly scanner: MScanner;
  private peek: MToken;

  constructor(scanner: MScanner) {
    this.scanner = scanner;
    this.peek = scanner.scanToken();
  }

  private newError(message: string) {
    return this.newErrorAt(this.peek.location, message);
  }

  private newErrorAt(location: MLocation, message: string) {
    return new MError(location, message);
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

  private maybeAtTypeExpression(): boolean {
    return this.at('IDENTIFIER') || this.at('nil');
  }

  private parseTypeExpression(): ast.TypeExpression {
    if (this.at('nil')) {
      const location = this.advance().location;
      return new ast.TypeExpression(
        location,
        null,
        new ast.Identifier(location, 'nil'),
        []);
    }
    const firstIdentifier = this.parseIdentifier();
    let secondIdentifier: ast.Identifier | null = null;
    const maybeDotLocation = this.peek.location;
    if (this.consume('.')) {
      if (this.at('IDENTIFIER')) {
        secondIdentifier = this.parseIdentifier();
      } else {
        // while this should technically be a syntax error,
        // allowing this form to create the AST in some form allows
        // for better completions.
        // So we fake it here.
        secondIdentifier = new ast.Identifier(maybeDotLocation, ' ');
      }
    }
    const args: ast.TypeExpression[] = [];
    let endLocation = (secondIdentifier || firstIdentifier).location;
    if (this.consume('[')) {
      while (!this.at(']')) {
        args.push(this.parseTypeExpression());
        if (!this.consume(',')) {
          break;
        }
      }
      endLocation = this.expect(']').location;
    }
    let location = firstIdentifier.location.merge(endLocation);
    let te = secondIdentifier ?
      new ast.TypeExpression(location, firstIdentifier, secondIdentifier, args) :
      new ast.TypeExpression(location, null, firstIdentifier, args);
    if (this.at('?')) {
      const qmarkLocation = this.expect('?').location;
      location = location.merge(qmarkLocation);
      te = new ast.TypeExpression(
        location, null, new ast.Identifier(qmarkLocation, 'optional'), [te]);
    }
    if (this.at('|')) {
      const pipeLocation = this.expect('|').location;
      const rhs = this.parseTypeExpression();
      location = location.merge(rhs.location);
      te = new ast.TypeExpression(
        location, null, new ast.Identifier(pipeLocation, 'union'), [te, rhs]);
    }
    return te;
  }

  private parseArguments(): ast.Expression[] {
    return this.parseArgumentsAndLocations()[0];
  }

  private parseArgumentsAndLocations(): [ast.Expression[], MLocation[]] {
    const args: ast.Expression[] = [];
    const argLocations: MLocation[] = [];
    while (true) {
      const argStartPosition = this.peek.location.range.start;
      if (this.at(')')) {
        const argEndPosition = this.peek.location.range.start;
        argLocations.push(new MLocation(
          this.scanner.filePath,
          new MRange(argStartPosition, argEndPosition)));
        break;
      }
      args.push(this.parseExpression());
      const argEndPosition = this.peek.location.range.start;
      argLocations.push(new MLocation(
        this.scanner.filePath,
        new MRange(argStartPosition, argEndPosition)));
      if (!this.consume(',')) {
        break;
      }
    }
    while (!this.at(')')) {
      args.push(this.parseExpression());
      if (!this.consume(',')) {
        break;
      }
    }
    return [args, argLocations];
  }

  private parsePrefix(): ast.Expression {
    const startLocation = this.peek.location;
    switch (this.peek.type) {
      case '(': {
        this.advance();
        const expression = this.parseExpression();
        this.expect(')');
        return expression;
      }
      case '[': {
        this.advance();
        const items: ast.Expression[] = [];
        while (!this.at(']')) {
          items.push(this.parseExpression());
          if (!this.consume(',')) {
            break;
          }
        }
        const endLocation = this.expect(']').location;
        const location = startLocation.merge(endLocation);
        return new ast.ListDisplay(location, items);
      }
      case '{': {
        this.advance();
        const pairs: [ast.Expression, ast.Expression][] = [];
        while (!this.at('}')) {
          const key = this.parseExpression();
          const value = this.consume(':') ?
            this.parseExpression() :
            this.newNil(key.location);
          pairs.push([key, value]);
          if (!this.consume(',')) {
            break;
          }
        }
        const endLocation = this.expect('}').location;
        const location = startLocation.merge(endLocation);
        return new ast.DictDisplay(location, pairs);
      }
      case 'final': {
        this.advance();
        if (this.consume('[')) {
          const items: ast.Expression[] = [];
          while (!this.at(']')) {
            items.push(this.parseExpression());
            if (!this.consume(',')) {
              break;
            }
          }
          const endLocation = this.expect(']').location;
          const location = startLocation.merge(endLocation);
          return new ast.TupleDisplay(location, items);
        } else if (this.consume('{')) {
          const pairs: [ast.Expression, ast.Expression][] = [];
          while (!this.at('}')) {
            const key = this.parseExpression();
            const value = this.consume(':') ?
              this.parseExpression() :
              this.newNil(key.location);
            pairs.push([key, value]);
            if (!this.consume(',')) {
              break;
            }
          }
          const endLocation = this.expect('}').location;
          const location = startLocation.merge(endLocation);
          return new ast.FrozenDictDisplay(location, pairs);
        } else {
          throw this.newError(`Expected '[' or '{' following 'final' in expression`);
        }
      }
      case 'NUMBER': {
        const expression = new ast.NumberLiteral(
          startLocation, <number>this.peek.value);
        this.advance();
        return expression;
      }
      case 'STRING': {
        const expression = new ast.StringLiteral(
          startLocation, <string>this.peek.value);
        this.advance();
        return expression;
      }
      case 'true': {
        this.advance();
        return new ast.BoolLiteral(startLocation, true);
      }
      case 'false': {
        this.advance();
        return new ast.BoolLiteral(startLocation, false);
      }
      case 'nil': {
        this.advance();
        return new ast.NilLiteral(startLocation, null);
      }
      case 'this':
      case 'super': {
        const name = this.advance().type;
        return new ast.GetVariable(
          startLocation,
          new ast.Identifier(startLocation, name));
      }
      case 'IDENTIFIER': {
        const identifier = this.parseIdentifier();
        if (this.consume('=')) {
          const value = this.parseExpression();
          return new ast.SetVariable(startLocation, identifier, value);
        } else {
          return new ast.GetVariable(startLocation, identifier);
        }
      }
      case 'not': {
        this.advance();
        const arg = this.parsePrec(PREC_UNARY_NOT);
        const location = startLocation.merge(arg.location);
        return new ast.Logical(location, 'not', [arg]);
      }
      case 'raise': {
        this.advance();
        const exc = this.parseExpression();
        const location = startLocation.merge(exc.location);
        return new ast.Raise(location, exc);
      }
      case '~':
      case '-':
      case '+':
        const tokenType = this.peek.type;
        const methodIdentifier = new ast.Identifier(
          startLocation,
          tokenType === '~' ? '__not__' :
          tokenType === '-' ? '__neg__' :
          tokenType === '+' ? '__pos__' : 'invalid');
        this.advance();
        const arg = this.parsePrec(PREC_UNARY_MINUS);
        const location = startLocation.merge(arg.location);
        return new ast.MethodCall(location, arg, methodIdentifier, []);
    }
    throw this.newError(
      `Expected expression but got '${this.peek.type}'`);
  }

  private parseInfix(lhs: ast.Expression, startLocation: MLocation): ast.Expression {
    const tokenType = this.peek.type;
    const precedence = PrecMap.get(tokenType);
    const methodName = BinopMethodMap.get(tokenType);
    if (!precedence) {
      throw this.newError(`assertionError infix precedence=${precedence}`);
    }

    switch (tokenType) {
      case '.': {
        const dotLocation = this.advance().location;
        if (!this.at('IDENTIFIER')) {
          const followLocation = this.peek.location;
          const location = startLocation.merge(dotLocation);
          return new ast.Dot(location, lhs, dotLocation, followLocation);
        }
        const identifier = this.parseIdentifier();
        if (this.at('(')) {
          this.advance();
          const [args, argLocations] = this.parseArgumentsAndLocations();
          const endLocation = this.expect(')').location;
          const location = startLocation.merge(endLocation);
          return new ast.MethodCall(location, lhs, identifier, args, argLocations);
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
        const [args, argLocations] = this.parseArgumentsAndLocations();
        const closeParenLocation = this.expect(')').location;
        const location = startLocation.merge(closeParenLocation);
        return new ast.FunctionCall(location, lhs, args, argLocations);
      }
      case 'as': {
        this.advance();
        const assertType = this.parseTypeExpression();
        const location = startLocation.merge(assertType.location);
        return new ast.TypeAssertion(location, lhs, assertType);
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
    if (tokenType === 'in') {
      let operatorLocation = this.advance().location;
      const rhs = this.parsePrec(precedence + 1);
      const location = startLocation.merge(rhs.location);
      const methodIdentifier = new ast.Identifier(
        operatorLocation, '__contains__');
      return new ast.MethodCall(location, rhs, methodIdentifier, [lhs]);
    }
    if (tokenType === 'not') {
      let operatorLocation = this.advance().location;
      operatorLocation = operatorLocation.merge(this.expect('in').location);
      const rhs = this.parsePrec(precedence + 1);
      const location = startLocation.merge(rhs.location);
      const methodIdentifier = new ast.Identifier(
        operatorLocation, '__notcontains__');
      return new ast.MethodCall(location, rhs, methodIdentifier, [lhs]);
    }
    if (tokenType === 'is') {
      let operatorLocation = this.advance().location;
      let methodName = '__is__';
      if (this.at('not')) {
        operatorLocation = operatorLocation.merge(this.advance().location);
        methodName = '__isnot__';
      }
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
    throw this.newError(`assertionError tokenType=${tokenType}`);
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

  private parseForStatement(): ast.For {
    const startLocation = this.expect('for').location;
    const identifier = this.parseIdentifier();
    this.expect('in');
    const container = this.parseExpression();
    const body = this.parseBlock();
    const location = startLocation.merge(body.location);
    return new ast.For(location, identifier, container, body);
  }

  private parseIfStatement(): ast.If {
    const startLocation = this.expect('if').location;
    const pairs: [ast.Expression, ast.Block][] = [];
    const condition = this.parseExpression();
    pairs.push([condition, this.parseBlock()]);
    while (this.consume('elif')) {
      const condition = this.parseExpression();
      pairs.push([condition, this.parseBlock()]);
    }
    const fallback = this.consume('else') ? this.parseBlock() : null;
    const location = startLocation.merge(
      fallback ? fallback.location : pairs[pairs.length - 1][1].location);
    return new ast.If(location, pairs, fallback);
  }

  private parseReturnStatement(): ast.Return {
    const startLocation = this.expect('return').location;
    const expression = this.parseExpression();
    const location = startLocation.merge(expression.location);
    this.expectStatementDelimiter();
    return new ast.Return(location, expression);
  }

  private parseWhileStatement(): ast.While {
    const startLocation = this.expect('while').location;
    const condition = this.parseExpression();
    const body = this.parseBlock();
    const location = startLocation.merge(body.location);
    return new ast.While(location, condition, body);
  }

  private parseImportStatement(): ast.Import {
    const startLocation = this.peek.location;
    this.expect('import');
    const moduleID = this.parseQualifiedIdentifier();
    const alias = this.consume('as') ? this.parseIdentifier() : null;
    const location = startLocation.merge(
      alias === null ? moduleID.location : alias.location);
    const importModule = new ast.Import(location, moduleID, alias);
    return importModule;
  }

  private parseExpressionStatement(): ast.ExpressionStatement {
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
      const itemDoc = getLastDocumentation(statements);
      statements.push(this.parseDeclaration(itemDoc));
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
    const typeExpression = this.parseTypeExpression();
    const location = startLocation.merge(typeExpression.location);
    this.expectStatementDelimiter();
    return new ast.Field(location, final, identifier, typeExpression);
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
    const staticMethods = [];
    while (this.at('static')) {
      this.expect('static');
      staticMethods.push(this.parseFunctionDeclaration());
      while (this.consume('NEWLINE') || this.consume(';'));
    }
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
      location, identifier, bases, documentation, staticMethods, fields, methods);
  }

  private parseParameter(): ast.Parameter {
    const identifier = this.parseIdentifier();
    const type = this.maybeAtTypeExpression() ?
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
    let documentation: ast.StringLiteral | null = null;
    const bodyStatements = body.statements;
    if (bodyStatements.length > 0) {
      const firstStatement = body.statements[0];
      if (firstStatement instanceof ast.ExpressionStatement) {
        const innerExpression = firstStatement.expression;
        if (innerExpression instanceof ast.StringLiteral) {
          documentation = innerExpression;
        }
      }
    }
    const location = startLocation.merge(body.location);
    return new ast.Function(
      location,
      identifier,
      parameters,
      returnType,
      documentation,
      body);
  }

  private newAnyType(location: MLocation): ast.TypeExpression {
    return new ast.TypeExpression(
      location,
      null,
      new ast.Identifier(location, 'Any'),
      []);
  }

  private newNil(location: MLocation): ast.NilLiteral {
    return new ast.NilLiteral(location, null);
  }

  private parseVariableDeclaration(itemDoc: string | null): ast.Variable {
    const startLocation = this.peek.location;
    const final = this.consume('final');
    if (!final) this.expect('var');
    const identifier = this.parseIdentifier();
    const typeExpression = this.at('=') ?
      null :
      this.parseTypeExpression();
    this.expect('=');
    const value = this.parseExpression();
    const location = startLocation.merge(value.location);
    return new ast.Variable(location, itemDoc, final, identifier, typeExpression, value);
  }

  private parseDeclaration(itemDoc: string | null): ast.Statement {
    switch (this.peek.type) {
      case 'def': return this.parseFunctionDeclaration();
      case 'final': case 'var': return this.parseVariableDeclaration(itemDoc);
      default: return this.parseStatement();
    }
  }

  private parseModuleLevelDeclaration(itemDoc: string | null): ast.Statement {
    switch (this.peek.type) {
      case 'class': return this.parseClassDeclaration();
      default: return this.parseDeclaration(itemDoc);
    }
  }

  parseFile(): ast.File {
    const startLocation = this.peek.location;
    let documentation: string | null = null;
    const imports: ast.Import[] = [];
    const statements: ast.Statement[] = [];
    try {
      while (this.consume('NEWLINE') || this.consume(';'));
      if (this.at('STRING')) {
        documentation = <string>this.advance().value;
      }
      while (this.consume('NEWLINE') || this.consume(';'));
      while (this.at('import')) {
        imports.push(this.parseImportStatement());
        while (this.consume('NEWLINE') || this.consume(';'));
      }
      while (!this.at('EOF')) {
        const itemDoc: string | null = getLastDocumentation(statements);
        statements.push(this.parseModuleLevelDeclaration(itemDoc));
      }
      const location = startLocation.merge(this.peek.location);
      return new ast.File(location, documentation, imports, statements, []);
    } catch (e) {
      if (e instanceof MError) {
        const location = startLocation.merge(this.peek.location);
        return new ast.File(location, documentation, imports, statements, [e]);
      } else {
        throw e;
      }
    }
  }
}

function getLastDocumentation(statements: ast.Statement[]): string | null {
  if (statements.length > 1) {
    const last = statements[statements.length - 1];
    if (last instanceof ast.ExpressionStatement) {
      const expr = last.expression;
      if (expr instanceof ast.StringLiteral) {
        return expr.value;
      }
    }
  }
  return null;
}
