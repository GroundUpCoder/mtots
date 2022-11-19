import { MLocation } from "./location";

type LogicalOperator = (
  'not'     |
  'and'     |
  'or'
)

export abstract class Ast {
  location: MLocation;
  constructor(location: MLocation) {
    this.location = location;
  }
}

export abstract class Statement extends Ast {}

export abstract class Expression extends Ast {}

/**
 * All type expressions boil down to operator[Arg1, Arg2, ...]
 * in some form.
 *
 * Some syntactic sugar:
 *   SomeType? => optional[SomeType]
 *   A.B       => member[A, B]
 *
 */
export class TypeExpression extends Ast {
  name: string;
  args: TypeExpression[];
  constructor(location: MLocation, name: string, args: TypeExpression[]) {
    super(location);
    this.name = name;
    this.args = args;
  }
}

export class Nop extends Statement {
  constructor(location: MLocation) {
    super(location);
  }
}

export class Parameter extends Ast {
  name: string;
  type: TypeExpression;
  defaultValue: Expression | null;
  constructor(
      location: MLocation,
      name: string,
      type: TypeExpression,
      defaultValue: Expression | null) {
    super(location);
    this.name = name;
    this.type = type;
    this.defaultValue = defaultValue;
  }
}

export class Function extends Statement {
  name: string;
  parameters: Parameter[];
  returnType: TypeExpression | null;
  body: Block;
  constructor(
      location: MLocation,
      name: string,
      parameters: Parameter[],
      returnType: TypeExpression | null,
      body: Block) {
    super(location);
    this.name = name;
    this.parameters = parameters;
    this.returnType = returnType;
    this.body = body;
  }
}

export class Field extends Ast {
  name: string;
  type: TypeExpression;
  constructor(location: MLocation, name: string, type: TypeExpression) {
    super(location);
    this.name = name;
    this.type = type;
  }
}

export class Class extends Statement {
  name: string;
  bases: Expression[];
  fields: Field[];
  methods: Function[];
  constructor(
      location: MLocation,
      name: string,
      bases: Expression[],
      fields: Field[],
      methods: Function[]) {
    super(location);
    this.name = name;
    this.bases = bases;
    this.fields = fields;
    this.methods = methods;
  }
}

export class Import extends Statement {
  moduleName: string;
  alias: string | null;
  constructor(
      location: MLocation,
      moduleName: string,
      alias: string | null) {
    super(location);
    this.moduleName = moduleName;
    this.alias = alias;
  }
}

export class VariableDeclaration extends Statement {
  final: boolean;
  name: string;
  type: TypeExpression;
  value: Expression;
  constructor(
      location: MLocation,
      final: boolean,
      name: string,
      type: TypeExpression,
      value: Expression) {
    super(location);
    this.final = final;
    this.name = name;
    this.type = type;
    this.value = value;
  }
}

export class While extends Statement {
  condition: Expression;
  body: Block;
  constructor(location: MLocation, condition: Expression, body: Block) {
    super(location);
    this.condition = condition;
    this.body = body;
  }
}

export class For extends Statement {
  variable: string;
  container: Expression;
  body: Block;
  constructor(location: MLocation, variable: string, container: Expression, body: Block) {
    super(location);
    this.variable = variable;
    this.container = container;
    this.body = body;
  }
}

export class If extends Statement {
  pairs: [Expression, Block][];
  fallback: Block | null;
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
  statements: Statement[];
  constructor(location: MLocation, statements: Statement[]) {
    super(location);
    this.statements = statements;
  }
}

export class Return extends Statement {
  expression: Expression;
  constructor(location: MLocation, expression: Expression) {
    super(location);
    this.expression = expression;
  }
}

export class ExpressionStatement extends Statement {
  expression: Expression;
  constructor(location: MLocation, expression: Expression) {
    super(location);
    this.expression = expression;
  }
}

export class GetVariable extends Expression {
  name: string;
  constructor(location: MLocation, name: string) {
    super(location);
    this.name = name;
  }
}

export class SetVariable extends Expression {
  name: string;
  value: Expression;
  constructor(location: MLocation, name: string, value: Expression) {
    super(location);
    this.name = name;
    this.value = value;
  }
}

export abstract class Literal<T> extends Expression {
  value: T;
  constructor(location: MLocation, value: T) {
    super(location);
    this.value = value;
  }
}

export class NilLiteral extends Literal<null> {}
export class BoolLiteral extends Literal<boolean> {}
export class NumberLiteral extends Literal<number> {}
export class StringLiteral extends Literal<string> {}

export class FunctionCall extends Expression {
  func: Expression;
  args: Expression[];
  constructor(location: MLocation, func: Expression, args: Expression[]) {
    super(location);
    this.func = func;
    this.args = args;
  }
}

export class MethodCall extends Expression {
  owner: Expression;
  name: string;
  args: Expression[];
  constructor(location: MLocation, owner: Expression, name: string, args: Expression[]) {
    super(location);
    this.owner = owner;
    this.name = name;
    this.args = args;
  }
}

export class GetField extends Expression {
  owner: Expression;
  name: string;
  constructor(location: MLocation, owner: Expression, name: string) {
    super(location);
    this.owner = owner;
    this.name = name;
  }
}

export class SetField extends Expression {
  owner: Expression;
  name: string;
  value: Expression;
  constructor(location: MLocation, owner: Expression, name: string, value: Expression) {
    super(location);
    this.owner = owner;
    this.name = name;
    this.value = value;
  }
}

export class Logical extends Expression {
  op: LogicalOperator;
  args: Expression[];
  constructor(
      location: MLocation,
      op: LogicalOperator,
      args: Expression[]) {
    super(location);
    this.op = op;
    this.args = args;
  }
}
