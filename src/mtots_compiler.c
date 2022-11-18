#include "mtots_common.h"
#include "mtots_compiler.h"
#include "mtots_scanner.h"
#include "mtots_memory.h"
#include "mtots_util.h"

#if DEBUG_PRINT_CODE
#include "mtots_debug.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Parser {
  Token current;
  Token previous;
  ubool hadError;
  ubool panicMode;
} Parser;

typedef enum Precedence {
  PREC_NONE,
  PREC_ASSIGNMENT,  /* = */
  PREC_IF,          /* if and try */
  PREC_OR,          /* or */
  PREC_AND,         /* and */
  PREC_NOT,         /* not */
  PREC_COMPARISON,  /* == != < > <= >= in not-in is is-not */
  PREC_SHIFT,       /* << >> */
  PREC_BITWISE_AND, /* & */
  PREC_BITWISE_XOR, /* ^ */
  PREC_BITWISE_OR,  /* | */
  PREC_TERM,        /* + - */
  PREC_FACTOR,      /* * / */
  PREC_UNARY,       /* ! - ~ */
  PREC_CALL,        /* . () [] */
  PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(ubool canAssign);

typedef struct ParseRule {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

typedef struct Local {
  Token name;
  i16 depth;
  ubool isCaptured;
} Local;

typedef struct Upvalue {
  u8 index;
  ubool isLocal;
} Upvalue;

typedef enum ThunkType {
  TYPE_FUNCTION,
  TYPE_INITIALIZER,
  TYPE_METHOD,
  TYPE_SCRIPT
} ThunkType;

typedef struct Compiler {
  struct Compiler *enclosing;
  ObjThunk *thunk;
  ThunkType type;

  Local locals[U8_COUNT];
  i16 localCount;
  Upvalue upvalues[U8_COUNT];
  i16 scopeDepth;

  Value defaultArgs[U8_COUNT];
  i16 defaultArgsCount;
} Compiler;

typedef struct ClassCompiler {
  struct ClassCompiler *enclosing;
  ubool hasSuperclass;
} ClassCompiler;

Parser parser;
Compiler *current = NULL;
ClassCompiler *currentClass = NULL;

static Token syntheticToken(const char *text);

static Chunk *currentChunk() {
  return &current->thunk->chunk;
}

static void errorAt(Token *token, const char *message) {
  if (parser.panicMode) {
    return;
  }
  parser.panicMode = 1;
  fprintf(stderr, "[line %d] Error", token->line);

  if (token->type == TOKEN_EOF) {
    fprintf(stderr, " at end");
  } else if (token->type == TOKEN_ERROR) {
    /* Nothing */
  } else {
    fprintf(stderr, " at '%.*s'", (int) token->length, token->start);
  }

  fprintf(stderr, ": %s\n", message);
  parser.hadError = 1;
}

static void error(const char *message) {
  errorAt(&parser.previous, message);
}

static void errorAtCurrent(const char *message) {
  errorAt(&parser.current, message);
}

static void advance() {
  parser.previous = parser.current;

  for (;;) {
    parser.current = scanToken();
    if (parser.current.type != TOKEN_ERROR) {
      break;
    }

    errorAtCurrent(parser.current.start);
  }
}

static void consume(TokenType type, const char *message) {
  if (parser.current.type == type) {
    advance();
    return;
  }
  errorAtCurrent(message);
}

static ubool parseCheck(TokenType type) {
  return parser.current.type == type;
}

/* If the current token matches the given token type,
 * move past it and return true.
 * Otherwise, do nothing and return false.
 */
static ubool parseMatch(TokenType type) {
  if (!parseCheck(type)) {
    return UFALSE;
  }
  advance();
  return UTRUE;
}

static void emitByte(u8 byte) {
  writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(u8 byte1, u8 byte2) {
  emitByte(byte1);
  emitByte(byte2);
}

static void emitLoop(i32 loopStart) {
  i32 offset;

  emitByte(OP_LOOP);

  offset = currentChunk()->count - loopStart + 2;
  if (offset > U16_MAX) {
    error("Loop body too large");
  }

  emitByte((offset >> 8) & 0xFF);
  emitByte(offset & 0xFF);
}

static i32 emitJump(u8 instruction) {
  emitByte(instruction);
  emitByte(0xFF);
  emitByte(0xFF);
  return currentChunk()->count - 2;
}

static void emitReturn() {
  if (current->type == TYPE_INITIALIZER) {
    emitBytes(OP_GET_LOCAL, 0);
  } else {
    emitByte(OP_NIL);
  }

  emitByte(OP_RETURN);
}

static u8 makeConstant(Value value) {
  size_t constant = addConstant(currentChunk(), value);
  if (constant > U8_MAX) {
    error("Too many constants in one chunk");
    return 0;
  }
  return (u8) constant;
}

static void emitConstant(Value value) {
  emitBytes(OP_CONSTANT, makeConstant(value));
}

static void patchJump(i32 offset) {
  /* -2 to adjust for the bytecode for the jump offset itself */
  i32 jump = currentChunk()->count - offset - 2;

  if (jump > U16_MAX) {
    error("Too much code to jump over");
  }

  currentChunk()->code[offset    ] = (jump >> 8) & 0xFF;
  currentChunk()->code[offset + 1] = jump & 0xFF;
}

static void initCompiler(Compiler *compiler, ThunkType type) {
  Local *local;

  compiler->enclosing = current;
  compiler->thunk = NULL;
  compiler->type = type;
  compiler->localCount = 0;
  compiler->scopeDepth = 0;
  compiler->defaultArgsCount = 0;

  compiler->thunk = newFunction();

  current = compiler;
  if (type != TYPE_SCRIPT) {
    current->thunk->name = internString(
      parser.previous.start, parser.previous.length);
  }

  local = &current->locals[current->localCount++];
  local->depth = 0;
  local->isCaptured = UFALSE;
  if (type != TYPE_FUNCTION) {
    local->name.start = "this";
    local->name.length = 4;
  } else {
    local->name.start = "";
    local->name.length = 0;
  }
}

static ObjThunk *endCompiler() {
  ObjThunk *thunk;

  emitReturn();
  thunk = current->thunk;

  /* Copy over any default arguments */
  if (current->defaultArgsCount > 0) {
    thunk->defaultArgs = ALLOCATE(Value, current->defaultArgsCount);
    thunk->defaultArgsCount = current->defaultArgsCount;
    memcpy(
      thunk->defaultArgs,
      current->defaultArgs,
      sizeof(Value) * current->defaultArgsCount);
  }

#if DEBUG_PRINT_CODE
  if (!parser.hadError) {
    disassembleChunk(
      currentChunk(),
      thunk->name != NULL ?
        thunk->name->chars :
        "<script>");
  }
#endif

  current = current->enclosing;
  return thunk;
}

static void beginScope() {
  current->scopeDepth++;
}

static void endScope() {
  current->scopeDepth--;
  while (current->localCount > 0 &&
      current->locals[current->localCount - 1].depth >
      current->scopeDepth) {
    if (current->locals[current->localCount - 1].isCaptured) {
      emitByte(OP_CLOSE_UPVALUE);
    } else {
      emitByte(OP_POP);
    }
    current->localCount--;
  }
}

static void parseExpression();
static void parseStatement();
static void parseDeclaration();
static ParseRule *getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

static u8 parseIdentifierConstant(Token *name) {
  return makeConstant(STRING_VAL(internString(name->start, name->length)));
}

static ubool parseIdentifiersEqual(Token *a, Token *b) {
  if (a->length != b->length) {
    return 0;
  }
  return memcmp(a->start, b->start, a->length) == 0;
}

static i16 resolveLocal(Compiler *compiler, Token *name) {
  i16 i;
  for (i = compiler->localCount - 1; i >= 0; i--) {
    Local *local = &compiler->locals[i];
    if (parseIdentifiersEqual(name, &local->name)) {
      if (local->depth == -1) {
        error("Can't read local parseVariable in its own initializer");
      }
      return i;
    }
  }
  return -1;
}

static i16 addUpvalue(Compiler *compiler, u8 index, ubool isLocal) {
  i16 upvalueCount = compiler->thunk->upvalueCount;
  i16 i;
  for (i = 0; i < upvalueCount; i++) {
    Upvalue *upvalue = &compiler->upvalues[i];
    if (upvalue->index == index && upvalue->isLocal == isLocal) {
      return i;
    }
  }

  if (upvalueCount == U8_COUNT) {
    error("Too many closure variables in thunk");
    return 0;
  }

  compiler->upvalues[upvalueCount].isLocal = isLocal;
  compiler->upvalues[upvalueCount].index = index;
  return compiler->thunk->upvalueCount++;
}

static i16 resolveUpvalue(Compiler *compiler, Token *name) {
  i16 local;
  if (compiler->enclosing == NULL) {
    return -1;
  }

  local = resolveLocal(compiler->enclosing, name);
  if (local != -1) {
    compiler->enclosing->locals[local].isCaptured = UTRUE;
    return addUpvalue(compiler, (u8) local, UTRUE);
  }

  return -1;
}

static void addLocal(Token name) {
  Local *local;

  if (current->localCount == U8_COUNT) {
    error("Too many local variables in thunk");
    return;
  }

  local = &current->locals[current->localCount++];
  local->name = name;
  local->depth = -1;
  local->isCaptured = UFALSE;
}

/* Record the existance of a variable */
static void parseDeclareVariable() {
  Token *name;
  i16 i;
  if (current->scopeDepth == 0) {
    return;
  }

  name = &parser.previous;
  for (i = current->localCount - 1; i >= 0; i--) {
    Local *local = &current->locals[i];
    if (local->depth != -1 && local->depth < current->scopeDepth) {
      break;
    }

    if (parseIdentifiersEqual(name, &local->name)) {
      error("Already a variable with this name in this scope");
    }
  }
  addLocal(*name);
}

static u8 parseAndGetVariable(const char *errorMessage) {
  consume(TOKEN_IDENTIFIER, errorMessage);

  parseDeclareVariable();
  if (current->scopeDepth > 0) {
    return 0;
  }

  return parseIdentifierConstant(&parser.previous);
}

static void markInitialized() {
  if (current->scopeDepth == 0) {
    return;
  }
  current->locals[current->localCount - 1].depth = current->scopeDepth;
}

/* Initialize a variable with the value at the top of the stack */
static void parseDefineVariable(u8 global) {
  if (current->scopeDepth > 0) { /* local variable */
    markInitialized();
    return;
  }

  emitBytes(OP_DEFINE_GLOBAL, global);
}

static u8 parseArgumentList() {
  u8 argCount = 0;
  if (!parseCheck(TOKEN_RIGHT_PAREN)) {
    do {
      if (parseCheck(TOKEN_RIGHT_PAREN)) {
        break;
      }
      parseExpression();
      if (argCount == 255) {
        error("Can't have more than 255 arguments");
      }
      argCount++;
    } while (parseMatch(TOKEN_COMMA));
  }
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments");
  return argCount;
}

static void parseAnd(ubool canAssign) {
  i32 endJump = emitJump(OP_JUMP_IF_FALSE);

  emitByte(OP_POP);
  parsePrecedence(PREC_AND);

  patchJump(endJump);
}

static void parseBinary(ubool canAssign) {
  TokenType operatorType = parser.previous.type;
  ParseRule *rule = getRule(operatorType);
  ubool isNot = UFALSE, notIn = UFALSE;
  if (operatorType == TOKEN_IS && parseMatch(TOKEN_NOT)) {
    isNot = UTRUE;
  } else if (operatorType == TOKEN_NOT) {
    consume(
      TOKEN_IN,
      "when used as a binary operator, 'not' must always be followed by 'in'");
    notIn = UTRUE;
    operatorType = TOKEN_IN;
  }
  parsePrecedence((Precedence) (rule->precedence + 1));

  switch (operatorType) {
    case TOKEN_IS: emitByte(OP_IS); if (isNot) emitByte(OP_NOT); break;
    case TOKEN_BANG_EQUAL: emitBytes(OP_EQUAL, OP_NOT); break;
    case TOKEN_EQUAL_EQUAL: emitByte(OP_EQUAL); break;
    case TOKEN_GREATER: emitByte(OP_GREATER); break;
    case TOKEN_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT); break;
    case TOKEN_LESS: emitByte(OP_LESS); break;
    case TOKEN_LESS_EQUAL: emitBytes(OP_GREATER, OP_NOT); break;
    case TOKEN_IN: emitByte(OP_IN); if (notIn) emitByte(OP_NOT); break;
    case TOKEN_PLUS: emitByte(OP_ADD); break;
    case TOKEN_MINUS: emitByte(OP_SUBTRACT); break;
    case TOKEN_STAR: emitByte(OP_MULTIPLY); break;
    case TOKEN_SLASH: emitByte(OP_DIVIDE); break;
    case TOKEN_SLASH_SLASH: emitByte(OP_FLOOR_DIVIDE); break;
    case TOKEN_PERCENT: emitByte(OP_MODULO); break;
    case TOKEN_SHIFT_LEFT: emitByte(OP_SHIFT_LEFT); break;
    case TOKEN_SHIFT_RIGHT: emitByte(OP_SHIFT_RIGHT); break;
    case TOKEN_PIPE: emitByte(OP_BITWISE_OR); break;
    case TOKEN_AMPERSAND: emitByte(OP_BITWISE_AND); break;
    case TOKEN_CARET: emitByte(OP_BITWISE_XOR); break;
    default:
      abort();
      return; /* unreachable */
  }
}

static void parseCall(ubool canAssign) {
  u8 argCount = parseArgumentList();
  emitBytes(OP_CALL, argCount);
}

static void parseDot(ubool canAssign) {
  u8 name;

  consume(TOKEN_IDENTIFIER, "Expect property name after '.'");
  name = parseIdentifierConstant(&parser.previous);

  if (canAssign && parseMatch(TOKEN_EQUAL)) {
    parseExpression();
    emitBytes(OP_SET_FIELD, name);
  } else if (parseMatch(TOKEN_LEFT_PAREN)) {
    u8 argCount = parseArgumentList();
    emitBytes(OP_INVOKE, name);
    emitByte(argCount);
  } else {
    emitBytes(OP_GET_FIELD, name);
  }
}

static Token syntheticToken(const char *text) {
  Token token;
  token.start = text;
  token.length = strlen(text);
  return token;
}

static void parseSubscript(ubool canAssign) {
  if (parseCheck(TOKEN_COLON)) {
    /* implicit 'nil' when first slice argument is missing */
    emitByte(OP_NIL);
  } else {
    parseExpression(); /* index */
  }

  if (parseMatch(TOKEN_COLON)) {
    Token nameToken = syntheticToken("__slice__");
    u8 name = parseIdentifierConstant(&nameToken);
    if (parseCheck(TOKEN_RIGHT_BRACKET)) {
      emitByte(OP_NIL); /* implicit nil second argument, if missing */
    } else {
      parseExpression();
    }
    consume(TOKEN_RIGHT_BRACKET, "Expect ']' after slice index expression");
    emitBytes(OP_INVOKE, name);
    emitByte(2); /* argCount */
  } else {
    consume(TOKEN_RIGHT_BRACKET, "Expect ']' after index expression");
    if (canAssign && parseMatch(TOKEN_EQUAL)) {
      Token nameToken = syntheticToken("__setitem__");
      u8 name = parseIdentifierConstant(&nameToken);
      parseExpression();
      emitBytes(OP_INVOKE, name);
      emitByte(2); /* argCount */
    } else {
      Token nameToken = syntheticToken("__getitem__");
      u8 name = parseIdentifierConstant(&nameToken);
      emitBytes(OP_INVOKE, name);
      emitByte(1);
    }
  }
}

static void parseLiteral(ubool canAssign) {
  switch (parser.previous.type) {
    case TOKEN_FALSE: emitByte(OP_FALSE); break;
    case TOKEN_NIL: emitByte(OP_NIL); break;
    case TOKEN_TRUE: emitByte(OP_TRUE); break;
    default:
      abort();
      return; /* unreachable */
  }
}

static void parseGrouping(ubool canAssign) {
  parseExpression();
  consume(TOKEN_RIGHT_PAREN, "Expected ')' after expression");
}

static void parseNumber(ubool canAssign) {
  double value = strtod(parser.previous.start, NULL);
  emitConstant(NUMBER_VAL(value));
}

static void parseNumberHex(ubool canAssign) {
  double value = 0;
  size_t i, len = parser.previous.length;
  for (i = 2; i < len; i++) {
    char ch = parser.previous.start[i];
    value *= 16;
    if ('0' <= ch && ch <= '9') {
      value += ch - '0';
    } else if ('A' <= ch && ch <= 'F') {
      value += ch - 'A' + 10;
    } else if ('a' <= ch && ch <= 'f') {
      value += ch - 'a' + 10;
    } else {
      panic("Invalid hex digit %c", ch);
    }
  }
  emitConstant(NUMBER_VAL(value));
}

static void parseNumberBin(ubool canAssign) {
  double value = 0;
  size_t i, len = parser.previous.length;
  for (i = 2; i < len; i++) {
    char ch = parser.previous.start[i];
    value *= 2;
    if (ch == '1' || ch == '0') {
      value += ch - '0';
    } else {
      panic("Invalid binary digit %c", ch);
    }
  }
  emitConstant(NUMBER_VAL(value));
}

static void parseTry(ubool canAssign) {
  i32 startJump, endJump;

  startJump = emitJump(OP_TRY_START);
  parseExpression();
  endJump = emitJump(OP_TRY_END);
  consume(TOKEN_ELSE, "Expected 'else' in 'try' expression");
  patchJump(startJump);
  parseExpression();
  patchJump(endJump);
}

static void parseRaise(ubool canAssign) {
  parseExpression();
  emitByte(OP_RAISE);
}

static void parseOr(ubool canAssign) {
  i32 elseJump = emitJump(OP_JUMP_IF_FALSE);
  i32 endJump = emitJump(OP_JUMP);

  patchJump(elseJump);
  emitByte(OP_POP);

  parsePrecedence(PREC_OR);
  patchJump(endJump);
}

static void parseRawString(ubool canAssign) {
  emitConstant(STRING_VAL(internString(
    parser.previous.start + 2,
    parser.previous.length - 3)));
}

static void parseTripleQuoteRawString(ubool canAssign) {
  emitConstant(STRING_VAL(internString(
    parser.previous.start + 4,
    parser.previous.length - 7)));
}

static String *stringTokenToObjString() {
  size_t size = 0;
  char *s;
  char quote = parser.previous.start[0];

  if (!unescapeString(parser.previous.start + 1, quote, &size, NULL)) {
    error("Failed to unescape string");
    return NULL;
  }

  s = malloc(sizeof(char) * size + 1);
  unescapeString(parser.previous.start + 1, quote, NULL, s);
  s[size] = '\0';

  return internOwnedString(s, size);
}

static void parseString(ubool canAssign) {
  String *str = stringTokenToObjString();
  if (str != NULL) {
    emitConstant(STRING_VAL(str));
  }
}

static void parseNamedVariable(Token name, ubool canAssign) {
  u8 getOp, setOp;
  i16 arg = resolveLocal(current, &name);
  if (arg != -1) {
    getOp = OP_GET_LOCAL;
    setOp = OP_SET_LOCAL;
  } else if ((arg = resolveUpvalue(current, &name)) != -1) {
    getOp = OP_GET_UPVALUE;
    setOp = OP_SET_UPVALUE;
  } else {
    arg = parseIdentifierConstant(&name);
    getOp = OP_GET_GLOBAL;
    setOp = OP_SET_GLOBAL;
  }

  if (canAssign && parseMatch(TOKEN_EQUAL)) {
    parseExpression();
    emitBytes(setOp, (u8) arg);
  } else {
    emitBytes(getOp, (u8) arg);
  }
}

static void parseVariable(ubool canAssign) {
  parseNamedVariable(parser.previous, canAssign);
}

static void parseSuper(ubool canAssign) {
  u8 name, argCount;

  if (currentClass == NULL) {
    error("Can't use 'super' outside of a class");
  } else if (!currentClass->hasSuperclass) {
    error("Can't use 'super' in a class with no superclass");
  }

  consume(TOKEN_DOT, "Expect '.' after 'super'");
  consume(TOKEN_IDENTIFIER, "Expect superclass method name");
  name = parseIdentifierConstant(&parser.previous);

  parseNamedVariable(syntheticToken("this"), UFALSE);
  consume(TOKEN_LEFT_PAREN, "Expect '(' to call super method");
  argCount = parseArgumentList();
  parseNamedVariable(syntheticToken("super"), UFALSE);
  emitBytes(OP_SUPER_INVOKE, name);
  emitByte(argCount);
}

static void parseThis(ubool canAssign) {
  if (currentClass == NULL) {
    error("Can't use 'this' outside of a class");
    return;
  }

  parseVariable(UFALSE);
}

static void parseListDisplay(ubool canAssign) {
  size_t length = 0;
  for (;;) {
    if (parseMatch(TOKEN_RIGHT_BRACKET)) {
      break;
    }
    parseExpression();
    length++;
    if (!parseMatch(TOKEN_COMMA)) {
      consume(TOKEN_RIGHT_BRACKET, "Expect ']' at the end of a list display");
      break;
    }
  }
  if (length > U8_MAX) {
    error("Number of items in a list display cannot exceed 255");
    return;
  }
  emitBytes(OP_NEW_LIST, length);
}

static void parseMapDisplay(ubool canAssign) {
  size_t length = 0;
  for (;;) {
    if (parseMatch(TOKEN_RIGHT_BRACE)) {
      break;
    }
    parseExpression();
    if (parseMatch(TOKEN_COLON)) {
      parseExpression();
    } else {
      /* if the value part is missing, 'nil' is implied */
      emitByte(OP_NIL);
    }
    length++;
    if (!parseMatch(TOKEN_COMMA)) {
      consume(TOKEN_RIGHT_BRACE, "Expect '}' at the end of a dict display");
      break;
    }
  }
  if (length > U8_MAX) {
    error("Number of pairs in a dict display cannot exceed 255");
    return;
  }
  emitBytes(OP_NEW_DICT, length);
}

static void parseUnary(ubool canAssign) {
  TokenType operatorType = parser.previous.type;

  /* compile the operand */
  parsePrecedence(operatorType == TOKEN_NOT ? PREC_NOT : PREC_UNARY);

  /* Emit the operator instruction */
  switch (operatorType) {
    case TOKEN_TILDE: emitByte(OP_BITWISE_NOT); break;
    case TOKEN_NOT: emitByte(OP_NOT); break;
    case TOKEN_MINUS: emitByte(OP_NEGATE); break;
    default:
      abort();
      return; /* Unreachable */
  }
}

static ParseRule rules[TOKEN_EOF + 1];

static ParseRule newRule(ParseFn prefix, ParseFn infix, Precedence prec) {
  ParseRule rule;
  rule.prefix = prefix;
  rule.infix = infix;
  rule.precedence = prec;
  return rule;
}

void initParseRules() {
  int i;
  for (i = 0; i <= TOKEN_EOF; i++) {
    rules[i].infix = rules[i].prefix = NULL;
    rules[i].precedence = PREC_NONE;
  }
  rules[TOKEN_LEFT_PAREN] = newRule(parseGrouping, parseCall, PREC_CALL);
  rules[TOKEN_LEFT_BRACE] = newRule(parseMapDisplay, NULL, PREC_NONE);
  rules[TOKEN_LEFT_BRACKET] = newRule(parseListDisplay, parseSubscript, PREC_CALL);
  rules[TOKEN_DOT] = newRule(NULL, parseDot, PREC_CALL);
  rules[TOKEN_MINUS] = newRule(parseUnary, parseBinary, PREC_TERM);
  rules[TOKEN_PERCENT] = newRule(NULL, parseBinary, PREC_FACTOR);
  rules[TOKEN_PLUS] = newRule(NULL, parseBinary, PREC_TERM);
  rules[TOKEN_SLASH] = newRule(NULL, parseBinary, PREC_FACTOR);
  rules[TOKEN_STAR] = newRule(NULL, parseBinary, PREC_FACTOR);
  rules[TOKEN_PIPE] = newRule(NULL, parseBinary, PREC_BITWISE_OR);
  rules[TOKEN_AMPERSAND] = newRule(NULL, parseBinary, PREC_BITWISE_AND);
  rules[TOKEN_CARET] = newRule(NULL, parseBinary, PREC_BITWISE_XOR);
  rules[TOKEN_TILDE] = newRule(parseUnary, NULL, PREC_NONE);
  rules[TOKEN_SHIFT_LEFT] = newRule(NULL, parseBinary, PREC_SHIFT);
  rules[TOKEN_SHIFT_RIGHT] = newRule(NULL, parseBinary, PREC_SHIFT);
  rules[TOKEN_BANG_EQUAL] = newRule(NULL, parseBinary, PREC_COMPARISON);
  rules[TOKEN_EQUAL_EQUAL] = newRule(NULL, parseBinary, PREC_COMPARISON);
  rules[TOKEN_GREATER] = newRule(NULL, parseBinary, PREC_COMPARISON);
  rules[TOKEN_GREATER_EQUAL] = newRule(NULL, parseBinary, PREC_COMPARISON);
  rules[TOKEN_LESS] = newRule(NULL, parseBinary, PREC_COMPARISON);
  rules[TOKEN_LESS_EQUAL] = newRule(NULL, parseBinary, PREC_COMPARISON);
  rules[TOKEN_SLASH_SLASH] = newRule(NULL, parseBinary, PREC_FACTOR);
  rules[TOKEN_IDENTIFIER] = newRule(parseVariable, NULL, PREC_NONE);
  rules[TOKEN_STRING] = newRule(parseString, NULL, PREC_NONE);
  rules[TOKEN_RAW_STRING] = newRule(parseRawString, NULL, PREC_NONE);
  rules[TOKEN_TRIPLE_QUOTE_RAW_STRING] =
    newRule(parseTripleQuoteRawString, NULL, PREC_NONE);
  rules[TOKEN_NUMBER] = newRule(parseNumber, NULL, PREC_NONE);
  rules[TOKEN_NUMBER_HEX] = newRule(parseNumberHex, NULL, PREC_NONE);
  rules[TOKEN_NUMBER_BIN] = newRule(parseNumberBin, NULL, PREC_NONE);
  rules[TOKEN_AND] = newRule(NULL, parseAnd, PREC_AND);
  rules[TOKEN_FALSE] = newRule(parseLiteral, NULL, PREC_NONE);
  rules[TOKEN_NIL] = newRule(parseLiteral, NULL, PREC_NONE);
  rules[TOKEN_OR] = newRule(NULL, parseOr, PREC_OR);
  rules[TOKEN_SUPER] = newRule(parseSuper, NULL, PREC_NONE);
  rules[TOKEN_THIS] = newRule(parseThis, NULL, PREC_NONE);
  rules[TOKEN_TRUE] = newRule(parseLiteral, NULL, PREC_NONE);
  rules[TOKEN_IN] = newRule(NULL, parseBinary, PREC_COMPARISON);
  rules[TOKEN_IS] = newRule(NULL, parseBinary, PREC_COMPARISON);
  rules[TOKEN_NOT] = newRule(parseUnary, parseBinary, PREC_COMPARISON);
  rules[TOKEN_RAISE] = newRule(parseRaise, NULL, PREC_NONE);
  rules[TOKEN_TRY] = newRule(parseTry, NULL, PREC_NONE);
}

static void parsePrecedence(Precedence precedence) {
  ParseFn prefixRule;
  ubool canAssign = precedence <= PREC_ASSIGNMENT;
  advance();
  prefixRule = getRule(parser.previous.type)->prefix;
  if (prefixRule == NULL) {
    printf("type = %d\n", parser.previous.type);
    error("Expected expression");
    abort();
    return;
  }
  prefixRule(canAssign);

  while (precedence <= getRule(parser.current.type)->precedence) {
    ParseFn infixRule;
    advance();
    infixRule = getRule(parser.previous.type)->infix;
    infixRule(canAssign);
  }
}

static ParseRule *getRule(TokenType type) {
  return &rules[type];
}

static void parseExpression() {
  parsePrecedence(PREC_ASSIGNMENT);
}

static void block(ubool newScope) {
  ubool atLeastOneDeclaration = UFALSE;

  if (newScope) {
    beginScope();
  }

  while (parseMatch(TOKEN_NEWLINE));
  consume(TOKEN_INDENT, "Expect INDENT at begining of block");
  while (parseMatch(TOKEN_NEWLINE));
  while (!parseCheck(TOKEN_DEDENT) && !parseCheck(TOKEN_EOF)) {
    atLeastOneDeclaration = UTRUE;
    parseDeclaration();
    while (parseMatch(TOKEN_NEWLINE));
  }

  consume(TOKEN_DEDENT, "Expect DEDENT after block");

  if (!atLeastOneDeclaration) {
    error("Expected an indented block");
  }

  if (newScope) {
    endScope();
  }
}

static Value parseDefaultArgument() {
  if (parseMatch(TOKEN_NIL)) {
    return NIL_VAL();
  } else if (parseMatch(TOKEN_TRUE)) {
    return BOOL_VAL(UTRUE);
  } else if (parseMatch(TOKEN_FALSE)) {
    return BOOL_VAL(UFALSE);
  } else if (parseMatch(TOKEN_NUMBER)) {
    double value = strtod(parser.previous.start, NULL);
    return NUMBER_VAL(value);
  } else if (parseMatch(TOKEN_STRING)) {
    String *str = stringTokenToObjString();
    return str ? STRING_VAL(str) : NIL_VAL();
  }
  error("Expected default argument expression");
  return NIL_VAL();
}

static void parseFunction(ThunkType type) {
  Compiler compiler;
  ObjThunk *thunk;
  i16 i;

  initCompiler(&compiler, type);
  compiler.thunk->moduleName = compiler.enclosing->thunk->moduleName;
  beginScope();

  consume(TOKEN_LEFT_PAREN, "Expect '(' after function name");
  if (!parseCheck(TOKEN_RIGHT_PAREN)) {
    do {
      u8 constant;
      current->thunk->arity++;
      if (current->thunk->arity > 255) {
        errorAtCurrent("Can't have more than 255 parameters");
      }
      constant = parseAndGetVariable("Expect parameter name");
      parseDefineVariable(constant);
      if (compiler.defaultArgsCount > 0 && !parseCheck(TOKEN_EQUAL)) {
        error("non-optional argument may not follow an optional argument");
      }
      if (parseMatch(TOKEN_EQUAL)) {
        compiler.defaultArgs[compiler.defaultArgsCount++] = parseDefaultArgument();
      }
    } while (parseMatch(TOKEN_COMMA));
  }
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters");

  consume(TOKEN_COLON, "Expect ':' before function body");
  while (parseMatch(TOKEN_NEWLINE));
  block(UFALSE);

  thunk = endCompiler();
  emitBytes(OP_CLOSURE, makeConstant(THUNK_VAL(thunk)));

  for (i = 0; i < thunk->upvalueCount; i++) {
    emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
    emitByte(compiler.upvalues[i].index);
  }
}

static void parseMethod() {
  u8 constant;
  ThunkType type;
  consume(TOKEN_DEF, "Expect 'def' to start method definition");
  consume(TOKEN_IDENTIFIER, "Expect method name");
  constant = parseIdentifierConstant(&parser.previous);

  type = TYPE_METHOD;
  if (parser.previous.length == 8 &&
      memcmp(parser.previous.start, "__init__", 8) == 0) {
    type = TYPE_INITIALIZER;
  }
  parseFunction(type);
  emitBytes(OP_METHOD, constant);
}

static void parseClassDeclaration() {
  u8 nameConstant;
  Token className;
  ClassCompiler classCompiler;

  consume(TOKEN_IDENTIFIER, "Expect class name");
  className = parser.previous;
  nameConstant = parseIdentifierConstant(&parser.previous);
  parseDeclareVariable();

  emitBytes(OP_CLASS, nameConstant);
  parseDefineVariable(nameConstant);

  classCompiler.hasSuperclass = UFALSE;
  classCompiler.enclosing = currentClass;
  currentClass = &classCompiler;

  if (parseMatch(TOKEN_LEFT_PAREN)) {
    if (!parseMatch(TOKEN_RIGHT_PAREN)) {
      parseExpression();

      /* TODO: add a 'full-name' parseCheck instead - we should parseCheck
       * both the class name and module the class is from */
      /*
      if (parseIdentifiersEqual(&className, &parser.previous)) {
        error("A class can't inherit from itself");
      }
      */

      beginScope();
      addLocal(syntheticToken("super"));
      parseDefineVariable(0);

      parseNamedVariable(className, UFALSE);
      emitByte(OP_INHERIT);
      classCompiler.hasSuperclass = UTRUE;

      consume(TOKEN_RIGHT_PAREN, "Expect ')' after superclass expression");
    }
  }

  parseNamedVariable(className, UFALSE);
  consume(TOKEN_COLON, "Expect ':' before class body");
  while (parseMatch(TOKEN_NEWLINE));
  consume(TOKEN_INDENT, "Expect INDENT before class body");
  while (parseMatch(TOKEN_NEWLINE));
  if (parseMatch(TOKEN_STRING) ||
      parseMatch(TOKEN_RAW_STRING) ||
      parseMatch(TOKEN_TRIPLE_QUOTE_RAW_STRING)) {
    while (parseMatch(TOKEN_NEWLINE));
  }
  while (!parseCheck(TOKEN_DEDENT) && !parseCheck(TOKEN_EOF)) {
    parseMethod();
    while (parseMatch(TOKEN_NEWLINE));
  }
  consume(TOKEN_DEDENT, "Expect DEDENT after class body");
  emitByte(OP_POP);

  if (classCompiler.hasSuperclass) {
    endScope();
  }

  currentClass = currentClass->enclosing;
}

static void parseFunDeclaration() {
  u8 global = parseAndGetVariable("Expect function name");
  markInitialized();
  parseFunction(TYPE_FUNCTION);
  parseDefineVariable(global);
}

static void consumeStatementDelimiter(const char *message) {
  if (!parseMatch(TOKEN_NEWLINE)) {
    consume(TOKEN_SEMICOLON, message);
  }
}

static void parseDecoratedFunDeclaration() {
  u8 global;
  size_t wrapCount = 0, i;
  ubool named = UFALSE;

  do {
    parseExpression();
    consumeStatementDelimiter(
      "Expected statement delimiter after decorator expression");
    wrapCount++;
  } while (parseMatch(TOKEN_AT));

  consume(
    TOKEN_DEF,
    "Expect 'def' to start function after decorator expression");
  if (parseCheck(TOKEN_IDENTIFIER)) {
    named = UTRUE;
    global = parseAndGetVariable("Expect function name");
    markInitialized();
  }
  parseFunction(TYPE_FUNCTION);

  for (i = 0; i < wrapCount; i++) {
    emitBytes(OP_CALL, 1);
  }

  if (named) {
    parseDefineVariable(global);
  } else {
    emitByte(OP_POP);
  }
}

static void parseVarDeclaration() {
  u8 global = parseAndGetVariable("Expect variable name");

  if (parseMatch(TOKEN_EQUAL)) {
    parseExpression();
  } else {
    emitByte(OP_NIL);
  }
  consumeStatementDelimiter(
    "Expected statement delimiter after variable declaration");

  parseDefineVariable(global);
}

static void parseExpressionStatement() {
  parseExpression();
  consumeStatementDelimiter(
    "Expected statement delimiter after expression");
  emitByte(OP_POP);
}

static void parseForInStatement() {
  i32 jump, loopStart;
  Token variableToken;

  beginScope(); /* ensures that scopeDepth > 0 */

  /* define the loop variable */
  consume(TOKEN_IDENTIFIER, "Expect loop variable name for for-in statement");
  variableToken = parser.previous;

  consume(TOKEN_IN, "Expect 'in' in for-in statement");
  parseExpression();          /* container/iterable */
  emitByte(OP_GET_ITER); /* replace container/iterable with an iterator */
  addLocal(syntheticToken("@iterator"));
  parseDefineVariable(0);
  loopStart = currentChunk()->count;
  emitByte(OP_GET_NEXT); /* gets next value returned by the iterator */
  jump = emitJump(OP_JUMP_IF_STOP_ITERATION);

  beginScope();
  addLocal(variableToken); /* next item is already on TOS */
  parseDefineVariable(0);
  consume(TOKEN_COLON, "Expect ':' to begin for-in loop body");
  block(UFALSE);
  endScope();

  emitLoop(loopStart);
  patchJump(jump);
  emitByte(OP_POP); /* StopIteration */

  /* iterator does not need an explicit
   * OP_POP, because iterator was added as a local
   * variable as '@iterator', so endScope() will pop it
   * automatically */
  endScope();
}

static void parseForStatement() {
  i32 loopStart, exitJump;

  if (parseCheck(TOKEN_IDENTIFIER)) {
    parseForInStatement();
    return;
  }

  beginScope();

  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'");
  if (parseMatch(TOKEN_SEMICOLON)) {
    /* No initializer */
  } else if (parseMatch(TOKEN_VAR)) {
    parseVarDeclaration();
  } else {
    parseExpressionStatement();
  }

  loopStart = currentChunk()->count;
  exitJump = -1;
  if (!parseMatch(TOKEN_SEMICOLON)) {
    parseExpression();
    consume(TOKEN_SEMICOLON, "Expect ';' after loop condition");

    /* Jump out of hte loop if the condition is false */
    exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP); /* Condition */
  }

  if (!parseMatch(TOKEN_RIGHT_PAREN)) {
    i32 bodyJump = emitJump(OP_JUMP);
    i32 incrementStart = currentChunk()->count;
    parseExpression();
    emitByte(OP_POP);
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses");

    emitLoop(loopStart);
    loopStart = incrementStart;
    patchJump(bodyJump);
  }

  consume(TOKEN_COLON, "Expect ':' for for body");
  block(UTRUE);
  emitLoop(loopStart);

  if (exitJump != -1) {
    patchJump(exitJump);
    emitByte(OP_POP); /* Condition */
  }

  endScope();
}

static void parseIfStatement() {
  i32 thenJump, i;
  i32 endJumps[MAX_ELIF_CHAIN_COUNT], endJumpsCount = 0;

  parseExpression();
  consume(TOKEN_COLON, "Expect ':' after condition");

  thenJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);
  block(UTRUE);
  endJumps[endJumpsCount++] = emitJump(OP_JUMP);

  patchJump(thenJump);
  emitByte(OP_POP);

  while (parseMatch(TOKEN_ELIF)) {
    i32 endJump;
    if (endJumpsCount >= MAX_ELIF_CHAIN_COUNT) {
      error("Too many chained 'elif' clauses");
    }
    parseExpression();
    consume(TOKEN_COLON, "Expect ':' after elif condition");
    thenJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    block(UTRUE);
    endJump = emitJump(OP_JUMP);
    patchJump(thenJump);
    emitByte(OP_POP);

    if (endJumpsCount < MAX_ELIF_CHAIN_COUNT) {
      endJumps[endJumpsCount++] = endJump;
    }
  }

  if (parseMatch(TOKEN_ELSE)) {
    consume(TOKEN_COLON, "Expect ':' after 'else'");
    block(UTRUE);
  }

  for (i = 0; i < endJumpsCount; i++) {
    patchJump(endJumps[i]);
  }
}

static void parseImportStatement() {
  u8 moduleName, alias;

  consume(TOKEN_IDENTIFIER, "Expect module name after 'import'");
  moduleName = parseIdentifierConstant(&parser.previous);

  if (parseMatch(TOKEN_AS)) {
    consume(TOKEN_IDENTIFIER, "Expect module alias after 'as'");
  }

  /* parseAndGetVariable but without consuming */
  parseDeclareVariable();
  alias = current->scopeDepth > 0 ? 0 : parseIdentifierConstant(&parser.previous);

  /* actually import module */
  emitBytes(OP_IMPORT, moduleName);
  parseDefineVariable(alias); /* store in variable */

  consumeStatementDelimiter(
    "Expect statement delimiter after import statement");
}

static void parseReturnStatement() {
  if (current->type == TYPE_SCRIPT) {
    error("Can't return from top-level code");
  }

  if (parseMatch(TOKEN_SEMICOLON)) {
    emitReturn();
  } else {
    if (current->type == TYPE_INITIALIZER) {
      error("Can't return a value from an initializer");
    }

    parseExpression();
    consumeStatementDelimiter("Expect newline or ';' after return value");
    emitByte(OP_RETURN);
  }
}

static void parseWhileStatement() {
  i32 exitJump, loopStart;

  loopStart = currentChunk()->count;
  parseExpression();
  consume(TOKEN_COLON, "Expect ':' after condition");

  exitJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);
  block(UTRUE);
  emitLoop(loopStart);

  patchJump(exitJump);
  emitByte(OP_POP);
}

static void synchronizeParser() {
  parser.panicMode = 0;

  while (parser.current.type != TOKEN_EOF) {
    if (parser.previous.type == TOKEN_SEMICOLON) {
      return;
    }
    switch (parser.current.type) {
      case TOKEN_CLASS:
      case TOKEN_DEF:
      case TOKEN_VAR:
      case TOKEN_FINAL:
      case TOKEN_FOR:
      case TOKEN_IF:
      case TOKEN_WHILE:
      case TOKEN_RETURN:
        return;
      default:
        break;
    }
    advance();
  }
}

static void parseDeclaration() {
  if (parseMatch(TOKEN_CLASS)) {
    parseClassDeclaration();
  } else if (parseMatch(TOKEN_DEF)) {
    parseFunDeclaration();
  } else if (parseMatch(TOKEN_VAR) || parseMatch(TOKEN_FINAL)) {
    parseVarDeclaration();
  } else if (parseMatch(TOKEN_AT)) {
    parseDecoratedFunDeclaration();
  } else {
    parseStatement();
  }

  if (parser.panicMode) {
    synchronizeParser();
  }
}

static void parseStatement() {
  if (parseMatch(TOKEN_FOR)) {
    parseForStatement();
  } else if (parseMatch(TOKEN_IF)) {
    parseIfStatement();
  } else if (parseMatch(TOKEN_RETURN)) {
    parseReturnStatement();
  } else if (parseMatch(TOKEN_WHILE)) {
    parseWhileStatement();
  } else if (parseMatch(TOKEN_IMPORT)) {
    parseImportStatement();
  } else if (parseMatch(TOKEN_NEWLINE) || parseMatch(TOKEN_SEMICOLON)) {
    /* nop statement */
  } else if (parseMatch(TOKEN_PASS)) {
    /* pass statement */
    consumeStatementDelimiter(
      "Expected statement delimiter at end of pass statement");
  } else {
    parseExpressionStatement();
  }
}

ObjThunk *compile(const char *source, String *moduleName) {
  Compiler compiler;
  ObjThunk *thunk;
  initScanner(source);
  initCompiler(&compiler, TYPE_SCRIPT);
  compiler.thunk->moduleName = moduleName;
  parser.hadError = 0;
  parser.panicMode = 0;
  advance();

  while (!parseMatch(TOKEN_EOF)) {
    parseDeclaration();
  }

  thunk = endCompiler();
  return parser.hadError ? NULL : thunk;
}

void markCompilerRoots() {
  Compiler *compiler = current;
  while (compiler != NULL) {
    i16 i;
    for (i = 0; i < compiler->defaultArgsCount; i++) {
      markValue(compiler->defaultArgs[i]);
    }
    markObject((Obj*)compiler->thunk);
    compiler = compiler->enclosing;
  }
}
