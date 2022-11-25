import { Uri } from 'vscode';
import * as ast from './ast';
import { Ast } from "./ast";
import { MError } from "./error";
import { MLocation } from './location';
import { MPosition } from './position';
import { MScanner } from "./scanner";
import { MScope } from "./scope";
import { MSymbol, MSymbolUsage } from "./symbol";
import { MToken, MTokenType } from "./token";
import { MType } from './type';
import * as types from './type';
import { TypeSolver } from './typesolver';

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
]);

export type SourceFinder = (path: string) => Promise<[string | Uri, string] | null>;

export class ParseContext {
  readonly sourceFinder: SourceFinder;
  private readonly moduleCache: Map<string, ast.Module> = new Map();
  readonly builtinScope: MScope = new MScope();
  constructor(sourceFinder: SourceFinder) {
    this.sourceFinder = sourceFinder;
    this.loadBuiltin();
  }

  async loadBuiltin() {
    const preludeModule = await this.loadModule('__builtin__');
    if (!preludeModule) {
      return; // TODO: indicate error
    }
    for (const symbol of preludeModule.scope.map.values()) {
      this.builtinScope.set(symbol);
    }
  }

  async loadModule(
      moduleName: string,
      uriAndContents: [Uri, string] | null = null): Promise<ast.Module | null> {
    const cached = this.moduleCache.get(moduleName);
    if (cached) {
      return cached;
    }
    let filePath: string | Uri = '';
    let contents: string = '';
    if (uriAndContents) {
      [filePath, contents] = uriAndContents;
    } else {
      const finderResult = await this.sourceFinder(moduleName);
      if (!finderResult) {
        return null;
      }
      const [foundFilePath, foundContents] = finderResult;
      filePath = foundFilePath;
      contents = foundContents;
    }
    const scanner = new MScanner(filePath, contents);
    const parser = new MParser(scanner, this);
    const module = await parser.parseModule();
    this.moduleCache.set(moduleName, module);
    return module;
  }
}

export class MParser {
  private symbolUsages: MSymbolUsage[];

  /**
   * Semantic errors are useful to alert about, but they should not
   * cause the parse to fail.
   * On the other hand, we throw on the first parse error since
   * such an error means we cannot be sure if anything we parse going forward
   * is at all meaningful.
   */
  private semanticErrors: MError[];

  private typeSolver: TypeSolver;
  private scanner: MScanner;
  private scope: MScope;
  private readonly moduleScope: MScope;
  private peek: MToken;

  private context: ParseContext;

  constructor(scanner: MScanner, context: ParseContext) {
    this.symbolUsages = [];
    this.semanticErrors = [];
    this.typeSolver = new TypeSolver(this.semanticErrors, this.symbolUsages);
    this.scanner = scanner;
    this.scope = new MScope(context.builtinScope);
    this.moduleScope = this.scope;
    this.peek = scanner.scanToken();
    this.context = context;
  }

  private newSemanticErrorAt(location: MLocation, message: string) {
    this.semanticErrors.push(this.newErrorAt(location, message));
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

  private solveType(e: ast.TypeExpression | ast.Expression): MType {
    return this.typeSolver.solve(e, this.scope);
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
        new ast.QualifiedIdentifier(location, null, new ast.Identifier(location, 'nil')),
        []);
    }
    const identifier = this.parseQualifiedIdentifier();
    const args: ast.TypeExpression[] = [];
    let endLocation = identifier.location;
    if (this.consume('[')) {
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

  private parseArguments(): ast.Expression[] {
    const args: ast.Expression[] = [];
    while (!this.at(')')) {
      args.push(this.parseExpression());
      if (!this.consume(',')) {
        break;
      }
    }
    return args;
  }

  private recordSymbolDefinition(
      identifier: ast.Identifier, addToScope: boolean, final: boolean = true): MSymbol {
    const previous = this.scope.map.get(identifier.name);
    if (previous) {
      this.newSemanticErrorAt(
        identifier.location,
        `'${previous.name}' is already defined in this scope`);
    }
    const symbol = new MSymbol(identifier.name, identifier.location, final);
    if (addToScope) {
      this.scope.set(symbol);
    }
    this.symbolUsages.push(symbol.definition);
    return symbol;
  }

  private recordSymbolUsage(identifier: ast.Identifier) {
    const symbol = this.scope.get(identifier.name);
    if (symbol === null) {
      return;
    }
    const usage = new MSymbolUsage(identifier.location, symbol);
    symbol.usages.push(usage);
    this.symbolUsages.push(usage);
  }

  private recordMemberUsage(owner: ast.Expression, identifier: ast.Identifier) {
    const ownerType = this.solveType(owner);
    if (ownerType instanceof types.Module) {
      const ownerSymbol = ownerType.symbol;
      const memberSymbol = ownerSymbol.members.get(identifier.name);
      if (!memberSymbol) {
        return;
      }
      const usage = new MSymbolUsage(identifier.location, memberSymbol);
      memberSymbol.usages.push(usage);
      this.symbolUsages.push(usage);
    }
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
      case 'IDENTIFIER': {
        const identifier = this.parseIdentifier();
        this.recordSymbolUsage(identifier);
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
        this.advance();
        const identifier = this.parseIdentifier();
        this.recordMemberUsage(lhs, identifier);
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
    if (tokenType === 'not') {
      let operatorLocation = this.advance().location;
      operatorLocation = operatorLocation.merge(this.expect('in').location);
      const rhs = this.parsePrec(precedence + 1);
      const location = startLocation.merge(rhs.location);
      const methodIdentifier = new ast.Identifier(
        operatorLocation, '__notcontains__');
      return new ast.MethodCall(location, lhs, methodIdentifier, [rhs]);
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

  private parseForStatement(): Ast {
    const startLocation = this.expect('for').location;
    const identifier = this.parseIdentifier();
    const symbol = this.recordSymbolDefinition(identifier, true);
    this.expect('in');
    const container = this.parseExpression();
    const containerType = this.solveType(container);
    const itemType = containerType.getForInItemType();
    if (itemType) {
      symbol.type = itemType;
    } else {
      this.newSemanticErrorAt(
        container.location,
        `${containerType} is not iterable`);
    }
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

  private async parseImportStatement(): Promise<Ast> {
    const startLocation = this.peek.location;
    this.expect('import');
    const moduleID = this.parseQualifiedIdentifier();
    const alias = this.consume('as') ? this.parseIdentifier() : null;
    const location = startLocation.merge(
      alias === null ? moduleID.location : alias.location);
    const importModule = new ast.Import(location, moduleID, alias);
    const importSymbol = this.recordSymbolDefinition(importModule.alias, true);
    importSymbol.type = types.Module.of(importSymbol, moduleID);
    const module = await this.context.loadModule(moduleID.toString());
    if (module) {
      importSymbol.members = module.scope.map;
      if (module.errors.length > 0) {
        const e = module.errors[0];
        this.newSemanticErrorAt(
          location, `module ${moduleID} has errors: ${e.message}`);
      }
    } else {
      this.newSemanticErrorAt(moduleID.location, `Module ${moduleID} not found`);
    }
    return importModule;
  }

  private parseExpressionStatement(): Ast {
    const startLocation = this.peek.location;
    const expression = this.parseExpression();
    const location = startLocation.merge(expression.location);
    this.expectStatementDelimiter();
    return new ast.ExpressionStatement(location, expression);
  }

  private parseBlock(): ast.Block {
    const oldScope = this.scope;
    this.scope = new MScope(oldScope);
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
    this.scope = oldScope;
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

  private parseFieldDeclaration(parentSymbol: MSymbol): ast.Field {
    const startLocation = this.peek.location;
    const final = this.consume('final');
    if (!final) {
      this.expect('var');
    }
    const identifier = this.parseIdentifier();
    const symbol = this.recordSymbolDefinition(identifier, false, final);
    parentSymbol.members.set(symbol.name, symbol);
    const type = this.parseTypeExpression();
    const location = startLocation.merge(type.location);
    this.expectStatementDelimiter();
    return new ast.Field(location, final, identifier, type);
  }

  private parseClassDeclaration(parentSymbol: MSymbol | null): ast.Class {
    const startLocation = this.expect('class').location;
    const identifier = this.parseIdentifier();
    const classSymbol = this.recordSymbolDefinition(identifier, true);
    if (parentSymbol) {
      parentSymbol.members.set(classSymbol.name, classSymbol);
    }
    classSymbol.type = types.Class.of(classSymbol);
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
    classSymbol.documentation = documentation;
    while (this.consume('NEWLINE') || this.consume(';'));
    while (this.consume('pass'));
    while (this.consume('NEWLINE') || this.consume(';'));
    const fields = [];
    while (this.at('var') || this.at('final')) {
      fields.push(this.parseFieldDeclaration(classSymbol));
      while (this.consume('NEWLINE') || this.consume(';'));
    }
    const methods = [];
    while (this.at('def')) {
      methods.push(this.parseMethodDeclaration(classSymbol));
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
    const type = this.maybeAtTypeExpression() ?
      this.parseTypeExpression() :
      this.newAnyType(identifier.location);
    const defaultValue = this.consume('=') ? this.parseExpression() : null;
    const location = identifier.location.merge(
      defaultValue ? defaultValue.location : type.location);
    const definition = this.recordSymbolDefinition(identifier, true, false);
    definition.type = this.solveType(type);
    return new ast.Parameter(location, identifier, type, defaultValue);
  }

  private parseFunctionDeclaration(parentSymbol: MSymbol | null): ast.Function {
    return this.parseFunctionOrMethodDeclaration(parentSymbol, false);
  }

  private parseMethodDeclaration(parentSymbol: MSymbol): ast.Function {
    return this.parseFunctionOrMethodDeclaration(parentSymbol, true);
  }

  private checkParameters(parameters: ast.Parameter[]) {
    let i = 0;
    for (; i < parameters.length; i++) {
      if (parameters[i].defaultValue !== null) {
        break;
      }
    }
    for (; i < parameters.length; i++) {
      if (parameters[i].defaultValue === null) {
        this.newSemanticErrorAt(
          parameters[i].location,
          `non-optional parameter may not follow an optional parameter`);
      }
    }
  }

  private parseFunctionOrMethodDeclaration(
      parentSymbol: MSymbol | null, isMethod:boolean): ast.Function {
    const startLocation = this.expect('def').location;
    const identifier = this.parseIdentifier();
    const functionSymbol = this.recordSymbolDefinition(identifier, !isMethod);
    if (parentSymbol) {
      parentSymbol.members.set(functionSymbol.name, functionSymbol);
    }
    const outerScope = this.scope;
    this.scope = new MScope(outerScope);
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
    this.checkParameters(parameters);
    const functionType = types.Function.of(
      parameters.map(p => this.solveType(p.type)),
      parameters.filter(p => p.defaultValue !== null).length,
      this.solveType(returnType));
    functionSymbol.type = functionType;
    const body = this.parseBlock();
    if (body.statements.length &&
        body.statements[0] instanceof ast.ExpressionStatement &&
        body.statements[0].expression instanceof ast.StringLiteral) {
      functionSymbol.documentation = body.statements[0].expression;
    }
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
    this.scope = outerScope;
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
      new ast.QualifiedIdentifier(
        location,
        null,
        new ast.Identifier(location, 'any')
      ),
      []);
  }

  private newNil(location: MLocation): ast.NilLiteral {
    return new ast.NilLiteral(location, null);
  }

  private parseVariableDeclaration(parentSymbol: MSymbol | null): ast.Variable {
    const startLocation = this.peek.location;
    const final = this.consume('final');
    if (!final) this.expect('var');
    const identifier = this.parseIdentifier();
    const explicitType = !this.at('=');
    const type = explicitType ?
      this.parseTypeExpression() :
      this.newAnyType(startLocation);
    const solvedVariableType = this.solveType(type);
    this.expect('=');
    const value = this.parseExpression();
    const valueType = this.typeSolver.solveExpression(
      value, this.scope, solvedVariableType);
    if (!valueType.isAssignableTo(solvedVariableType)) {
      this.newSemanticErrorAt(
        value.location,
        `Cannot assign ${valueType} to ${solvedVariableType}`);
    }
    const location = startLocation.merge(value.location);
    const varSymbol = this.recordSymbolDefinition(identifier, true, final);
    varSymbol.type = explicitType ? solvedVariableType : valueType;
    if (parentSymbol) {
      parentSymbol.members.set(varSymbol.name, varSymbol);
    }
    return new ast.Variable(location, final, identifier, type, value);
  }

  private parseDeclaration(): ast.Statement {
    switch (this.peek.type) {
      case 'class': return this.parseClassDeclaration(null);
      case 'def': return this.parseFunctionDeclaration(null);
      case 'final': case 'var': return this.parseVariableDeclaration(null);
      default: return this.parseStatement();
    }
  }

  private async parseModuleLevelDeclaration(): Promise<ast.Statement> {
    switch (this.peek.type) {
      case 'import': return await this.parseImportStatement();
      default: return this.parseDeclaration();
    }
  }

  async parseModule(): Promise<ast.Module> {
    const startLocation = this.peek.location;
    const statements: ast.Statement[] = [];
    try {
      while (!this.at('EOF')) {
        statements.push(await this.parseModuleLevelDeclaration());
      }
      const location = startLocation.merge(this.peek.location);
      return new ast.Module(
        location, statements, this.scope, this.symbolUsages, this.semanticErrors);
    } catch (e) {
      if (e instanceof MError) {
        const location = startLocation.merge(this.peek.location);
        const errors = [e, ...this.semanticErrors];
        return new ast.Module(
          location, statements, this.scope, this.symbolUsages, errors);
      } else {
        throw e;
      }
    }
  }
}
