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

export abstract class Statement extends Ast {}

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
  readonly errors: MError[];
  constructor(
      location: MLocation,
      statements: Statement[],
      scope: MScope,
      symbolUsages: MSymbolUsage[],
      errors: MError[]) {
    super(location);
    this.statements = statements;
    this.scope = scope;
    this.symbolUsages = symbolUsages;
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
}

export class Nop extends Statement {
  constructor(location: MLocation) {
    super(location);
  }
}

export class Parameter extends Ast {
  readonly identifier: Identifier;
  readonly type: TypeExpression;
  readonly defaultValue: Expression | null;
  constructor(
      location: MLocation,
      identifier: Identifier,
      type: TypeExpression,
      defaultValue: Expression | null) {
    super(location);
    this.identifier = identifier;
    this.type = type;
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
}

export class Field extends Ast {
  readonly final: boolean;
  readonly identifier: Identifier;
  readonly type: TypeExpression;
  constructor(
      location: MLocation,
      final: boolean,
      identifier: Identifier,
      type: TypeExpression) {
    super(location);
    this.final = final;
    this.identifier = identifier;
    this.type = type;
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
}

/** Variable Declaration */
export class Variable extends Statement {
  readonly final: boolean;
  readonly identifier: Identifier;
  readonly type: TypeExpression;
  readonly value: Expression;
  constructor(
      location: MLocation,
      final: boolean,
      identifier: Identifier,
      type: TypeExpression,
      value: Expression) {
    super(location);
    this.final = final;
    this.identifier = identifier;
    this.type = type;
    this.value = value;
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
}

export class Block extends Statement {
  readonly statements: Statement[];
  constructor(location: MLocation, statements: Statement[]) {
    super(location);
    this.statements = statements;
  }
}

export class Return extends Statement {
  readonly expression: Expression;
  constructor(location: MLocation, expression: Expression) {
    super(location);
    this.expression = expression;
  }
}

export class ExpressionStatement extends Statement {
  readonly expression: Expression;
  constructor(location: MLocation, expression: Expression) {
    super(location);
    this.expression = expression;
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
  abstract visitListDisplay(e: ListDisplay): R;
  abstract visitDictDisplay(e: DictDisplay): R;
  abstract visitFunctionCall(e: FunctionCall): R;
  abstract visitMethodCall(e: MethodCall): R;
  abstract visitGetField(e: GetField): R;
  abstract visitSetField(e: SetField): R;
  abstract visitLogical(e: Logical): R;
  abstract visitRaise(e: Raise): R;
}
