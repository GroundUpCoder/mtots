#include "mtots_scanner.h"
#include "mtots_common.h"

#include <string.h>

#include <stdio.h>

typedef struct {
  const char *start;
  const char *current;
  i32 line;

  /* Keeps track of how many parentheses, brackets or braces we
   * are currently nested in.
   * Newlines are ignored when groupingDepth is positive.
   */
  i32 groupingDepth;

  /* The indentation level of the current line.
   * Used to determine how many INDENT or DEDENT tokens need to be
   * emitted when processing the start of the next line.
   */
  i32 indentationLevel;

  /* non-zero potential indicates pending INDENT or DEDENT tokens */
  i32 indentationPotential;

  /* Indicates whether a synthetic newline has been emitted yet
   * This is always done at the end to ensure a final NEWLINE token
   * and all matching DEDENT tokens have been emitted.
   */
  ubool processedSyntheticNewline;
} Scanner;

Scanner scanner;

void initScanner(const char *source) {
  scanner.start = source;
  scanner.current = source;
  scanner.line = 1;
  scanner.groupingDepth = 0;
  scanner.indentationLevel = 0;
  scanner.indentationPotential = 0;
  scanner.processedSyntheticNewline = UFALSE;
}

static ubool isAlpha(char c) {
  return
    (c >= 'a' && c <= 'z') ||
    (c >= 'A' && c <= 'Z') ||
    c == '_';
}

static ubool isDigit(char c) {
  return c >= '0' && c <= '9';
}

static ubool isAtEnd() {
  return *scanner.current == '\0';
}

static char advance() {
  scanner.current++;
  return scanner.current[-1];
}

static char peek() {
  return *scanner.current;
}

static char peekNext() {
  if (isAtEnd()) {
    return '\0';
  }
  return scanner.current[1];
}

static ubool match(char expected) {
  if (isAtEnd()) {
    return UFALSE;
  }
  if (*scanner.current != expected) {
    return UFALSE;
  }
  scanner.current++;
  return UTRUE;
}

static Token makeToken(TokenType type) {
  Token token;
  token.type = type;
  token.start = scanner.start;
  token.length = scanner.current - scanner.start;
  token.line = scanner.line;
  return token;
}

static char errorMessageBuffer[512];

static Token errorToken(const char *message) {
  Token token;
  token.type = TOKEN_ERROR;
  token.start = message;
  token.length = strlen(message);
  token.line = scanner.line;
  return token;
}

static void skipWhitespace() {
  for (;;) {
    char c = peek();
    switch (c) {
      case ' ':
      case '\r':
      case '\t':
        advance();
        break;
      case '\n':
        /* If we are nested inside (), [] or {}, we
         * skip newlines */
        if (scanner.groupingDepth > 0) {
          scanner.line++;
          advance();
          break;
        }
        /* Otherwise, we cannot skip newlines */
        return;
      case '#':
        /* A comment goes until the end of the line */
        while (peek() != '\n' && !isAtEnd()) {
          advance();
        }
        break;
      default:
        return;
    }
  }
}

static TokenType checkKeyword(
    int start, int length, const char *rest, TokenType type) {
  if (scanner.current - scanner.start == start + length &&
      memcmp(scanner.start + start, rest, length) == 0) {
    return type;
  }
  return TOKEN_IDENTIFIER;
}

static TokenType identifierType() {
  switch (scanner.current - scanner.start) {
    case 0:
    case 1:
      break;
    case 2:
      switch (scanner.start[0]) {
        case 'a':
          switch (scanner.start[1]) {
            case 's': return TOKEN_AS;
          }
          break;
        case 'i':
          switch (scanner.start[1]) {
            case 'f': return TOKEN_IF;
            case 'n': return TOKEN_IN;
            case 's': return TOKEN_IS;
          }
          break;
        case 'o':
          switch (scanner.start[1]) {
            case 'r': return TOKEN_OR;
          }
          break;
      }
      break;
    default:
      switch (scanner.start[0]) {
        case 'a':
          switch (scanner.start[1]) {
            case 'n': return checkKeyword(2, 1, "d", TOKEN_AND);
            case 's':
              switch (scanner.start[2]) {
                case 's': return checkKeyword(3, 3, "ert", TOKEN_ASSERT);
                case 'y': return checkKeyword(3, 2, "nc", TOKEN_ASYNC);
              }
              break;
            case 'w': return checkKeyword(2, 3, "ait", TOKEN_AWAIT);
          }
          break;
        case 'b': return checkKeyword(1, 4, "reak", TOKEN_BREAK);
        case 'c':
          switch (scanner.start[1]) {
            case 'l': return checkKeyword(2, 3, "ass", TOKEN_CLASS);
            case 'o': return checkKeyword(2, 6, "ntinue", TOKEN_CONTINUE);
          }
          break;
        case 'd':
          switch (scanner.start[1]) {
            case 'e':
              switch (scanner.start[2]) {
                case 'f': return checkKeyword(3, 0, "", TOKEN_DEF);
                case 'l': return checkKeyword(3, 0, "", TOKEN_DEL);
              }
              break;
          }
          break;
        case 'e':
          switch (scanner.start[1]) {
            case 'l':
              switch (scanner.start[2]) {
                case 'i': return checkKeyword(3, 1, "f", TOKEN_ELIF);
                case 's': return checkKeyword(3, 1, "e", TOKEN_ELSE);
              }
              break;
            case 'x': return checkKeyword(2, 4, "cept", TOKEN_EXCEPT);
          }
          break;
        case 'f':
          switch (scanner.start[1]) {
            case 'a': return checkKeyword(2, 3, "lse", TOKEN_FALSE);
            case 'i':
              if (scanner.current - scanner.start == 5) {
                return checkKeyword(2, 3, "nal", TOKEN_FINAL);
              }
              return checkKeyword(2, 5, "nally", TOKEN_FINALLY);
            case 'o': return checkKeyword(2, 1, "r", TOKEN_FOR);
            case 'r': return checkKeyword(2, 2, "om", TOKEN_FROM);
          }
          break;
        case 'g': return checkKeyword(1, 5, "lobal", TOKEN_GLOBAL);
        case 'i': return checkKeyword(1, 5, "mport", TOKEN_IMPORT);
        case 'l': return checkKeyword(1, 5, "ambda", TOKEN_LAMBDA);
        case 'n':
          switch (scanner.start[1]) {
            case 'i': return checkKeyword(2, 1, "l", TOKEN_NIL);
            case 'o': return checkKeyword(2, 1, "t", TOKEN_NOT);
          }
          break;
        case 'p': return checkKeyword(1, 3, "ass", TOKEN_PASS);
        case 'r':
          switch (scanner.start[1]) {
            case 'a': return checkKeyword(2, 3, "ise", TOKEN_RAISE);
            case 'e': return checkKeyword(2, 4, "turn", TOKEN_RETURN);
          }
          break;
        case 's': return checkKeyword(1, 4, "uper", TOKEN_SUPER);
        case 't':
          switch (scanner.start[1]) {
            case 'h': return checkKeyword(2, 2, "is", TOKEN_THIS);
            case 'r':
              switch (scanner.start[2]) {
                case 'u': return checkKeyword(3, 1, "e", TOKEN_TRUE);
                case 'y': return checkKeyword(3, 0, "", TOKEN_TRY);
              }
              break;
          }
          break;
        case 'v': return checkKeyword(1, 2, "ar", TOKEN_VAR);
        case 'w':
          switch (scanner.start[1]) {
            case 'h': return checkKeyword(2, 3, "ile", TOKEN_WHILE);
            case 'i': return checkKeyword(2, 2, "th", TOKEN_WITH);
          }
          break;
        case 'y': return checkKeyword(1, 4, "ield", TOKEN_YIELD);
      }
  }
  return TOKEN_IDENTIFIER;
}

static Token identifier() {
  while (isAlpha(peek()) || isDigit(peek())) {
    advance();
  }
  if (scanner.current - scanner.start > MAX_IDENTIFIER_LENGTH) {
    snprintf(
      errorMessageBuffer,
      sizeof(errorMessageBuffer),
      "An identifier may not exceed %d characters",
      MAX_IDENTIFIER_LENGTH);
    return errorToken(errorMessageBuffer);
  }
  return makeToken(identifierType());
}

static Token number() {
  if (scanner.start[0] == '0') {
    if (match('x')) {
      /* hex number */
      while (isDigit(peek()) ||
          (peek() >= 'A' && peek() <= 'F') ||
          (peek() >= 'a' && peek() <= 'f')) {
        advance();
      }
      return makeToken(TOKEN_NUMBER_HEX);
    }
    if (match('b')) {
      /* binary number */
      while (peek() == '0' || peek() == '1') {
        advance();
      }
      return makeToken(TOKEN_NUMBER_BIN);
    }
  }

  while (isDigit(peek())) {
    advance();
  }

  /* Look for a fractional part */
  if (peek() == '.' && isDigit(peekNext())) {
    advance(); /* consume the '.' */
    while (isDigit(peek())) {
      advance();
    }
  }

  return makeToken(TOKEN_NUMBER);
}

static Token string(char quote, TokenType type) {
  while (peek() != quote && !isAtEnd()) {
    if (peek() == '\n') {
      scanner.line++;
    }
    advance();
  }

  if (isAtEnd()) {
    return errorToken("Unterminated string");
  }

  advance(); /* The closing quote */
  return makeToken(type);
}

Token scanToken() {
  char c;

  if (scanner.indentationPotential > 0) {
    scanner.indentationPotential--;
    return makeToken(TOKEN_INDENT);
  }

  if (scanner.indentationPotential < 0) {
    scanner.indentationPotential++;
    return makeToken(TOKEN_DEDENT);
  }

  skipWhitespace();
  scanner.start = scanner.current;

  if (isAtEnd()) {
    if (!scanner.processedSyntheticNewline) {
      scanner.indentationPotential = -scanner.indentationLevel;
      scanner.processedSyntheticNewline = UTRUE;
      return makeToken(TOKEN_NEWLINE);
    }
    return makeToken(TOKEN_EOF);
  }

  c = advance();

  if (c == 'r' && (peek() == '"' || peek() == '\'')) {
    char quote = advance();
    return string(quote, TOKEN_RAW_STRING);
  }

  if (isAlpha(c)) {
    return identifier();
  }

  if (isDigit(c)) {
    return number();
  }

  switch (c) {
    case '(': scanner.groupingDepth++; return makeToken(TOKEN_LEFT_PAREN);
    case ')': scanner.groupingDepth--; return makeToken(TOKEN_RIGHT_PAREN);
    case '{': scanner.groupingDepth++; return makeToken(TOKEN_LEFT_BRACE);
    case '}': scanner.groupingDepth--; return makeToken(TOKEN_RIGHT_BRACE);
    case '[': scanner.groupingDepth++; return makeToken(TOKEN_LEFT_BRACKET);
    case ']': scanner.groupingDepth--; return makeToken(TOKEN_RIGHT_BRACKET);
    case ':': return makeToken(TOKEN_COLON);
    case ';': return makeToken(TOKEN_SEMICOLON);
    case ',': return makeToken(TOKEN_COMMA);
    case '.': return makeToken(TOKEN_DOT);
    case '-': return makeToken(TOKEN_MINUS);
    case '+': return makeToken(TOKEN_PLUS);
    case '/': return makeToken(match('/') ? TOKEN_SLASH_SLASH : TOKEN_SLASH);
    case '%': return makeToken(TOKEN_PERCENT);
    case '*': return makeToken(TOKEN_STAR);
    case '@': return makeToken(TOKEN_AT);
    case '!':
      return makeToken(
        match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
    case '=':
      return makeToken(
        match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
    case '<':
      return makeToken(
        match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
    case '>':
      return makeToken(
        match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
    case '"': return string('"', TOKEN_STRING);
    case '\'': return string('\'', TOKEN_STRING);
    case '\n': {
      Token newlineToken;
      i32 spaceCount = 0, newIndentationLevel;

      newlineToken = makeToken(TOKEN_NEWLINE);

      /* Collapse consecutive newlines into a single newline token
       * This is necessary to properly handle indentation with
       * empty blank lines
       */
      scanner.line++;
      while (peek() == '\n') {
        advance();
        scanner.line++;
      }

      /* Handle indentation */
      while (peek() == ' ') {
        advance();
        spaceCount++;
      }
      if (spaceCount % 2 == 1) {
        return errorToken("Indentations must always be a multiple of 2");
      }
      newIndentationLevel = spaceCount / 2;
      scanner.indentationPotential =
        newIndentationLevel - scanner.indentationLevel;
      scanner.indentationLevel = newIndentationLevel;

      return newlineToken;
    }
  }

  return errorToken("Unexpected character");
}
