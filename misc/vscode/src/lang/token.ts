import { MLocation } from "./location";

type MTokenTypeKeyword = (
  'and' | 'class' | 'def' | 'elif' | 'else' | 'false' | 'for' |
  'if' | 'nil' | 'or' | 'return' | 'super' | 'this' | 'true' |
  'var' | 'while' | 'as' | 'assert' | 'async' | 'await' | 'break' |
  'continue' | 'del' | 'except' | 'final' | 'finally' | 'from' |
  'global' | 'import' | 'in' | 'is' | 'lambda' | 'not' | 'pass' |
  'raise' | 'try' | 'with' | 'yield'
)

type MTokenTypeSymbol = (
  // grouping tokens
  '(' | ')' |
  '[' | ']' |
  '{' | '}' |

  // other single character tokens
  ':' | ';' | ',' | '.' | '-' | '+' | '/' | '%' | '*' |
  '@' | '|' | '&' | '^' | '~' | '?' | '!' | '=' | '<' | '>' |

  // double character tokens
  '//' | '!=' | '==' | '<<' | '<=' | '>>' | '>='
);

export type MTokenType = (
  'IDENTIFIER' | 'STRING' | 'NUMBER' |
  'NEWLINE' | 'INDENT' | 'DEDENT' | 'EOF' |
  MTokenTypeKeyword |
  MTokenTypeSymbol
);

export type MTokenValue = number | string | null;

export const Symbols: MTokenTypeSymbol[] = [
  // grouping tokens
  '(', ')',
  '[', ']',
  '{', '}',

  // other single character tokens
  ':', ';', ',', '.', '-', '+', '/', '%', '*',
  '@', '|', '&', '^', '~', '?', '!', '=', '<', '>',

  // double character tokens
  '//', '!=', '==', '<<', '<=', '>>', '>='
];

export const SymbolsMap: Map<string, MTokenTypeSymbol> = new Map(
  Symbols.map(symbol => [symbol, symbol])
);

export const Keywords: MTokenTypeKeyword[] = [
  'and', 'class', 'def', 'elif', 'else', 'false', 'for',
  'if', 'nil', 'or', 'return', 'super', 'this', 'true',
  'var', 'while', 'as', 'assert', 'async', 'await', 'break',
  'continue', 'del', 'except', 'final', 'finally', 'from',
  'global', 'import', 'in', 'is', 'lambda', 'not', 'pass',
  'raise', 'try', 'with', 'yield'
];

export const KeywordsMap: Map<string, MTokenTypeKeyword> = new Map(
  Keywords.map(keyword => [keyword, keyword])
);

export class MToken {
  location: MLocation;
  type: MTokenType;
  value: MTokenValue;
  constructor(location: MLocation, type: MTokenType, value: MTokenValue = null) {
    this.location = location;
    this.type = type;
    this.value = value;
  }
}
