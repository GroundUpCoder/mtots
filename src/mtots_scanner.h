#ifndef mtots_scanner_h
#define mtots_scanner_h

#include <stddef.h>

typedef enum {
  /* Single-character tokens. */
  TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
  TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
  TOKEN_LEFT_BRACKET, TOKEN_RIGHT_BRACKET,
  TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PERCENT, TOKEN_PLUS,
  TOKEN_COLON, TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR,
  /* One or two character tokens. */
  TOKEN_BANG, TOKEN_BANG_EQUAL,
  TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
  TOKEN_GREATER, TOKEN_GREATER_EQUAL,
  TOKEN_LESS, TOKEN_LESS_EQUAL,
  /* Literals. */
  TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_RAW_STRING, TOKEN_NUMBER,
  TOKEN_NUMBER_HEX, TOKEN_NUMBER_BIN,
  /* Keywords. */
  TOKEN_AND, TOKEN_CLASS, TOKEN_DEF, TOKEN_ELIF, TOKEN_ELSE,
  TOKEN_FALSE, TOKEN_FOR, TOKEN_IF, TOKEN_NIL, TOKEN_OR,
  TOKEN_RETURN, TOKEN_SUPER, TOKEN_THIS, TOKEN_TRUE,
  TOKEN_VAR, TOKEN_WHILE,

  /* Additional Keywords */
  TOKEN_AS, TOKEN_ASSERT, TOKEN_ASYNC, TOKEN_AWAIT, TOKEN_BREAK,
  TOKEN_CONTINUE, TOKEN_DEL, TOKEN_EXCEPT, TOKEN_FINAL, TOKEN_FINALLY,
  TOKEN_FROM, TOKEN_GLOBAL, TOKEN_IMPORT, TOKEN_IN, TOKEN_IS, TOKEN_LAMBDA,
  TOKEN_NOT, TOKEN_PASS, TOKEN_RAISE, TOKEN_TRY,
  TOKEN_WITH, TOKEN_YIELD,

  /* Whitespace Tokens */
  TOKEN_NEWLINE, TOKEN_INDENT, TOKEN_DEDENT,

  TOKEN_ERROR, TOKEN_EOF
} TokenType;

typedef struct {
  TokenType type;
  const char *start;
  size_t length;
  int line;
} Token;

void initScanner(const char *source);
Token scanToken();

#endif/*mtots_scanner_h*/
