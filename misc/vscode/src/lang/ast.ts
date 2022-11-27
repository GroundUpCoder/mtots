import { CompletionPoint } from "./completion";
import { MError } from "./error";
import { MLocation } from "./location";
import { MPosition } from "./position";
import { MScope } from "./scope";
import { MSymbol, MSymbolUsage } from "./symbol";

type LogicalOperator = (
  'not'     |
  'and'     |
  'or'
)

export abstract class Ast {
  readonly className: string;    // makes JSON.stringify a bit more readable
  readonly location: MLocation;
  constructor(location: MLocation) {
    this.className = this.constructor.name;
    this.location = location;
  }
}

export class Identifier extends Ast {
  readonly name: string;
  constructor(location: MLocation, name: string) {
    super(location);
    this.name = name;
  }
}

export class QualifiedIdentifier extends Ast {
  readonly parent: QualifiedIdentifier | null;
  readonly identifier: Identifier;
  constructor(
      location: MLocation,
      parent: QualifiedIdentifier | null,
      identifier: Identifier) {
    super(location);
    this.parent = parent;
    this.identifier = identifier;
  }
  toString() {
    const parent = this.parent;
    return parent ? parent + this.identifier.name : this.identifier.name;
  }
}

export abstract class Statement extends Ast {
  abstract accept<R>(visitor: StatementVisitor<R>): R;
}

export abstract class Expression extends Ast {
  abstract accept<R>(visitor: ExpressionVisitor<R>): R;
}

/**
 * All type expressions boil down to operator[Arg1, Arg2, ...]
 * in some form.
 *
 * Some syntactic sugar:
 *   SomeType?  => optional[SomeType]
 *   A.B        => member[A, B]
 *   A | B | C  => union[A, union[B, C]]
 *
 */
export class TypeExpression extends Ast {
  readonly identifier: QualifiedIdentifier;
  readonly args: TypeExpression[];
  constructor(
      location: MLocation,
      identifier: QualifiedIdentifier,
      args: TypeExpression[]) {
    super(location);
    this.identifier = identifier;
    this.args = args;
  }
}

export class Module extends Ast {
  readonly statements: Statement[];
  readonly scope: MScope;
  private readonly symbolUsages: MSymbolUsage[];
  private readonly completionPoints: CompletionPoint[];
  readonly errors: MError[];
  constructor(
      location: MLocation,
      statements: Statement[],
      scope: MScope,
      symbolUsages: MSymbolUsage[],
      completionPoints: CompletionPoint[],
      errors: MError[]) {
    super(location);
    this.statements = statements;
    this.scope = scope;
    this.symbolUsages = symbolUsages;
    this.completionPoints = completionPoints;
    this.errors = errors;
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
}

export class Nop extends Statement {
  constructor(location: MLocation) {
    super(location);
  }

  accept<R>(visitor: StatementVisitor<R>): R {
    return visitor.visitNop(this);
  }
}

export class Parameter extends Ast {
  readonly identifier: Identifier;
  readonly typeExpression: TypeExpression;
  readonly defaultValue: Expression | null;
  constructor(
      location: MLocation,
      identifier: Identifier,
      typeExpression: TypeExpression,
      defaultValue: Expression | null) {
    super(location);
    this.identifier = identifier;
    this.typeExpression = typeExpression;
    this.defaultValue = defaultValue;
  }
}

export class Function extends Statement {
  readonly identifier: Identifier;
  readonly parameters: Parameter[];
  readonly returnType: TypeExpression;
  readonly documentation: StringLiteral | null;
  readonly body: Block;
  constructor(
      location: MLocation,
      identifier: Identifier,
      parameters: Parameter[],
      returnType: TypeExpression,
      documentation: StringLiteral | null,
      body: Block) {
    super(location);
    this.identifier = identifier;
    this.parameters = parameters;
    this.returnType = returnType;
    this.documentation = documentation;
    this.body = body;
  }

  accept<R>(visitor: StatementVisitor<R>): R {
    return visitor.visitFunction(this);
  }
}

export class Field extends Ast {
  readonly final: boolean;
  readonly identifier: Identifier;
  readonly typeExpression: TypeExpression;
  constructor(
      location: MLocation,
      final: boolean,
      identifier: Identifier,
      typeExpression: TypeExpression) {
    super(location);
    this.final = final;
    this.identifier = identifier;
    this.typeExpression = typeExpression;
  }
}

export class Class extends Statement {
  readonly identifier: Identifier;
  readonly bases: Expression[];
  readonly documentation: StringLiteral | null;
  readonly fields: Field[];
  readonly methods: Function[];
  constructor(
      location: MLocation,
      identifier: Identifier,
      bases: Expression[],
      documentation: StringLiteral | null,
      fields: Field[],
      methods: Function[]) {
    super(location);
    this.identifier = identifier;
    this.bases = bases;
    this.documentation = documentation;
    this.fields = fields;
    this.methods = methods;
  }

  accept<R>(visitor: StatementVisitor<R>): R {
    return visitor.visitClass(this);
  }
}

export class Import extends Statement {
  readonly module: QualifiedIdentifier;
  readonly alias: Identifier;
  constructor(
      location: MLocation,
      module: QualifiedIdentifier,
      alias: Identifier | null) {
    super(location);
    this.module = module;
    this.alias = alias || module.identifier;
  }

  accept<R>(visitor: StatementVisitor<R>): R {
    return visitor.visitImport(this);
  }
}

/** Variable Declaration */
export class Variable extends Statement {
  readonly final: boolean;
  readonly identifier: Identifier;
  readonly typeExpression: TypeExpression | null;
  readonly valueExpression: Expression;
  constructor(
      location: MLocation,
      final: boolean,
      identifier: Identifier,
      typeExpression: TypeExpression | null,
      valueExpression: Expression) {
    super(location);
    this.final = final;
    this.identifier = identifier;
    this.typeExpression = typeExpression;
    this.valueExpression = valueExpression;
  }

  accept<R>(visitor: StatementVisitor<R>): R {
    return visitor.visitVariable(this);
  }
}

export class While extends Statement {
  readonly condition: Expression;
  readonly body: Block;
  constructor(location: MLocation, condition: Expression, body: Block) {
    super(location);
    this.condition = condition;
    this.body = body;
  }

  accept<R>(visitor: StatementVisitor<R>): R {
    return visitor.visitWhile(this);
  }
}

export class For extends Statement {
  readonly variable: Identifier;
  readonly container: Expression;
  readonly body: Block;
  constructor(
      location: MLocation,
      variable: Identifier,
      container: Expression,
      body: Block) {
    super(location);
    this.variable = variable;
    this.container = container;
    this.body = body;
  }

  accept<R>(visitor: StatementVisitor<R>): R {
    return visitor.visitFor(this);
  }
}

export class If extends Statement {
  readonly pairs: [Expression, Block][];
  readonly fallback: Block | null;
  constructor(
      location: MLocation,
      pairs: [Expression, Block][],
      fallback: Block | null) {
    super(location);
    this.pairs = pairs;
    this.fallback = fallback;
  }

  accept<R>(visitor: StatementVisitor<R>): R {
    return visitor.visitIf(this);
  }
}

export class Block extends Statement {
  readonly statements: Statement[];
  constructor(location: MLocation, statements: Statement[]) {
    super(location);
    this.statements = statements;
  }

  accept<R>(visitor: StatementVisitor<R>): R {
    return visitor.visitBlock(this);
  }
}

export class Return extends Statement {
  readonly expression: Expression;
  constructor(location: MLocation, expression: Expression) {
    super(location);
    this.expression = expression;
  }

  accept<R>(visitor: StatementVisitor<R>): R {
    return visitor.visitReturn(this);
  }
}

export class ExpressionStatement extends Statement {
  readonly expression: Expression;
  constructor(location: MLocation, expression: Expression) {
    super(location);
    this.expression = expression;
  }

  accept<R>(visitor: StatementVisitor<R>): R {
    return visitor.visitExpressionStatement(this);
  }
}

export class GetVariable extends Expression {
  readonly identifier: Identifier;
  constructor(location: MLocation, identifier: Identifier) {
    super(location);
    this.identifier = identifier;
  }
  accept<R>(visitor: ExpressionVisitor<R>): R {
    return visitor.visitGetVariable(this);
  }
}

export class SetVariable extends Expression {
  readonly identifier: Identifier;
  readonly value: Expression;
  constructor(location: MLocation, identifier: Identifier, value: Expression) {
    super(location);
    this.identifier = identifier;
    this.value = value;
  }
  accept<R>(visitor: ExpressionVisitor<R>): R {
    return visitor.visitSetVariable(this);
  }
}

export abstract class Literal<T> extends Expression {
  readonly value: T;
  constructor(location: MLocation, value: T) {
    super(location);
    this.value = value;
  }
}

export class NilLiteral extends Literal<null> {
  accept<R>(visitor: ExpressionVisitor<R>): R {
    return visitor.visitNilLiteral(this);
  }
}
export class BoolLiteral extends Literal<boolean> {
  accept<R>(visitor: ExpressionVisitor<R>): R {
    return visitor.visitBoolLiteral(this);
  }
}
export class NumberLiteral extends Literal<number> {
  accept<R>(visitor: ExpressionVisitor<R>): R {
    return visitor.visitNumberLiteral(this);
  }
}
export class StringLiteral extends Literal<string> {
  accept<R>(visitor: ExpressionVisitor<R>): R {
    return visitor.visitStringLiteral(this);
  }
}

export class TypeAssertion extends Expression {
  readonly expression: Expression;
  readonly typeExpression: TypeExpression;
  constructor(location: MLocation, expression: Expression, typeExpression: TypeExpression) {
    super(location);
    this.expression = expression;
    this.typeExpression = typeExpression;
  }
  accept<R>(visitor: ExpressionVisitor<R>): R {
    return visitor.visitTypeAssertion(this);
  }
}

export class ListDisplay extends Expression {
  readonly items: Expression[];
  constructor(location: MLocation, items: Expression[]) {
    super(location);
    this.items = items;
  }
  accept<R>(visitor: ExpressionVisitor<R>): R {
    return visitor.visitListDisplay(this);
  }
}

export class DictDisplay extends Expression {
  readonly pairs: [Expression, Expression][];
  constructor(location: MLocation, pairs: [Expression, Expression][]) {
    super(location);
    this.pairs = pairs;
  }
  accept<R>(visitor: ExpressionVisitor<R>): R {
    return visitor.visitDictDisplay(this);
  }
}

export class FunctionCall extends Expression {
  readonly func: Expression;
  readonly args: Expression[];
  constructor(location: MLocation, func: Expression, args: Expression[]) {
    super(location);
    this.func = func;
    this.args = args;
  }
  accept<R>(visitor: ExpressionVisitor<R>): R {
    return visitor.visitFunctionCall(this);
  }
}

export class MethodCall extends Expression {
  readonly owner: Expression;
  readonly identifier: Identifier;
  readonly args: Expression[];
  constructor(
      location: MLocation,
      owner: Expression,
      identifier: Identifier,
      args: Expression[]) {
    super(location);
    this.owner = owner;
    this.identifier = identifier;
    this.args = args;
  }
  accept<R>(visitor: ExpressionVisitor<R>): R {
    return visitor.visitMethodCall(this);
  }
}

export class GetField extends Expression {
  readonly owner: Expression;
  readonly identifier: Identifier;
  constructor(location: MLocation, owner: Expression, identifier: Identifier) {
    super(location);
    this.owner = owner;
    this.identifier = identifier;
  }
  accept<R>(visitor: ExpressionVisitor<R>): R {
    return visitor.visitGetField(this);
  }
}

export class SetField extends Expression {
  readonly owner: Expression;
  readonly identifier: Identifier;
  readonly value: Expression;
  constructor(
      location: MLocation,
      owner: Expression,
      identifier: Identifier,
      value: Expression) {
    super(location);
    this.owner = owner;
    this.identifier = identifier;
    this.value = value;
  }
  accept<R>(visitor: ExpressionVisitor<R>): R {
    return visitor.visitSetField(this);
  }
}

export class Logical extends Expression {
  readonly op: LogicalOperator;
  readonly args: Expression[];
  constructor(
      location: MLocation,
      op: LogicalOperator,
      args: Expression[]) {
    super(location);
    this.op = op;
    this.args = args;
  }
  accept<R>(visitor: ExpressionVisitor<R>): R {
    return visitor.visitLogical(this);
  }
}

export class Raise extends Expression {
  readonly exception: Expression;
  constructor(location: MLocation, exception: Expression) {
    super(location);
    this.exception = exception;
  }

  accept<R>(visitor: ExpressionVisitor<R>): R {
    return visitor.visitRaise(this);
  }
}

export abstract class ExpressionVisitor<R> {
  abstract visitGetVariable(e: GetVariable): R;
  abstract visitSetVariable(e: SetVariable): R;
  abstract visitNilLiteral(e: NilLiteral): R;
  abstract visitBoolLiteral(e: BoolLiteral): R;
  abstract visitNumberLiteral(e: NumberLiteral): R;
  abstract visitStringLiteral(e: StringLiteral): R;
  abstract visitTypeAssertion(e: TypeAssertion): R;
  abstract visitListDisplay(e: ListDisplay): R;
  abstract visitDictDisplay(e: DictDisplay): R;
  abstract visitFunctionCall(e: FunctionCall): R;
  abstract visitMethodCall(e: MethodCall): R;
  abstract visitGetField(e: GetField): R;
  abstract visitSetField(e: SetField): R;
  abstract visitLogical(e: Logical): R;
  abstract visitRaise(e: Raise): R;
}

export abstract class StatementVisitor<R> {
  abstract visitNop(s: Nop): R;
  abstract visitFunction(s: Function): R;
  abstract visitClass(s: Class): R;
  abstract visitImport(s: Import): R;
  abstract visitVariable(s: Variable): R;
  abstract visitWhile(s: While): R;
  abstract visitFor(s: For): R;
  abstract visitIf(s: If): R;
  abstract visitBlock(s: Block): R;
  abstract visitReturn(s: Return): R;
  abstract visitExpressionStatement(s: ExpressionStatement): R;
}
