#include "mtots_util_scanner.h"
#include "mtots_common.h"

#include <string.h>

#include <stdio.h>

typedef struct Scanner {
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

static char advanceScanner() {
  scanner.current++;
  return scanner.current[-1];
}

static char peekScanner() {
  return *scanner.current;
}

static char peekNextInScanner() {
  if (isAtEnd()) {
    return '\0';
  }
  return scanner.current[1];
}

static ubool matchScanner(char expected) {
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

static char scannerErrorMessageBuffer[512];

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
    char c = peekScanner();
    switch (c) {
      case ' ':
      case '\r':
      case '\t':
        advanceScanner();
        break;
      case '\n':
        /* If we are nested inside (), [] or {}, we
         * skip newlines */
        if (scanner.groupingDepth > 0) {
          scanner.line++;
          advanceScanner();
          break;
        }
        /* Otherwise, we cannot skip newlines */
        return;
      case '#':
        /* A comment goes until the end of the line */
        while (peekScanner() != '\n' && !isAtEnd()) {
          advanceScanner();
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

static TokenType scanIdentifierType() {
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

static Token scanIdentifier() {
  while (isAlpha(peekScanner()) || isDigit(peekScanner())) {
    advanceScanner();
  }
  if (scanner.current - scanner.start > MAX_IDENTIFIER_LENGTH) {
    snprintf(
      scannerErrorMessageBuffer,
      sizeof(scannerErrorMessageBuffer),
      "An identifier may not exceed %d characters",
      MAX_IDENTIFIER_LENGTH);
    return errorToken(scannerErrorMessageBuffer);
  }
  return makeToken(scanIdentifierType());
}

static Token scanNumber() {
  if (scanner.start[0] == '0') {
    if (matchScanner('x')) {
      /* hex number */
      while (isDigit(peekScanner()) ||
          (peekScanner() >= 'A' && peekScanner() <= 'F') ||
          (peekScanner() >= 'a' && peekScanner() <= 'f')) {
        advanceScanner();
      }
      return makeToken(TOKEN_NUMBER_HEX);
    }
    if (matchScanner('b')) {
      /* binary number */
      while (peekScanner() == '0' || peekScanner() == '1') {
        advanceScanner();
      }
      return makeToken(TOKEN_NUMBER_BIN);
    }
  }

  while (isDigit(peekScanner())) {
    advanceScanner();
  }

  /* Look for a fractional part */
  if (peekScanner() == '.' && isDigit(peekNextInScanner())) {
    advanceScanner(); /* consume the '.' */
    while (isDigit(peekScanner())) {
      advanceScanner();
    }
  }

  return makeToken(TOKEN_NUMBER);
}

static Token scanTripleQuoteString(char quoteChar) {
  while (!isAtEnd() &&
      !(
        scanner.current[0] == quoteChar &&
        scanner.current[1] == quoteChar &&
        scanner.current[2] == quoteChar
      )) {
    if (peekScanner() == '\n') {
      scanner.line++;
    }
    if (peekScanner() == '\\') {
      advanceScanner();
      if (isAtEnd()) {
        return errorToken("Expected string escape but got EOF");
      }
    }
    advanceScanner();
  }

  if (isAtEnd()) {
    return errorToken("Unterminated string literal (triple quotes)");
  }

  advanceScanner(); /* The closing quote */
  advanceScanner(); /* The closing quote */
  advanceScanner(); /* The closing quote */
  return makeToken(TOKEN_STRING);
}

static Token scanString(char quote) {
  if (peekScanner() == quote && peekNextInScanner() == quote) {
    advanceScanner();
    advanceScanner();
    return scanTripleQuoteString(quote);
  }
  while (peekScanner() != quote && !isAtEnd()) {
    if (peekScanner() == '\n') {
      scanner.line++;
    }
    if (peekScanner() == '\\') {
      advanceScanner();
      if (isAtEnd()) {
        return errorToken("Expected string escape but got EOF");
      }
    }
    advanceScanner();
  }

  if (isAtEnd()) {
    return errorToken("Unterminated string literal");
  }

  advanceScanner(); /* The closing quote */
  return makeToken(TOKEN_STRING);
}

static Token scanRawString(char quote) {
  while (peekScanner() != quote && !isAtEnd()) {
    if (peekScanner() == '\n') {
      scanner.line++;
    }
    advanceScanner();
  }

  if (isAtEnd()) {
    return errorToken("Unterminated raw string literal");
  }

  advanceScanner(); /* The closing quote */
  return makeToken(TOKEN_RAW_STRING);
}

static Token scanRawTripleQuoteString(char quote) {
  size_t quoteRun = 0;
  while (quoteRun < 3 && !isAtEnd()) {
    if (peekScanner() == '\n') {
      scanner.line++;
    }
    if (peekScanner() == quote) {
      quoteRun++;
    } else {
      quoteRun = 0;
    }
    advanceScanner();
  }

  if (isAtEnd()) {
    return errorToken("Unterminated raw triple quote string literal");
  }

  return makeToken(TOKEN_RAW_STRING);
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

  c = advanceScanner();

  if (c == 'r' && (peekScanner() == '"' || peekScanner() == '\'')) {
    char quote = advanceScanner();
    if (peekScanner() == quote) {
      advanceScanner();
      if (peekScanner() == quote) {
        advanceScanner();
        return scanRawTripleQuoteString(quote);
      } else {
        return makeToken(TOKEN_RAW_STRING);
      }
    }
    return scanRawString(quote);
  }

  if (isAlpha(c)) {
    return scanIdentifier();
  }

  if (isDigit(c)) {
    return scanNumber();
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
    case '/': return makeToken(matchScanner('/') ? TOKEN_SLASH_SLASH : TOKEN_SLASH);
    case '%': return makeToken(TOKEN_PERCENT);
    case '*': return makeToken(TOKEN_STAR);
    case '@': return makeToken(TOKEN_AT);
    case '|': return makeToken(TOKEN_PIPE);
    case '&': return makeToken(TOKEN_AMPERSAND);
    case '^': return makeToken(TOKEN_CARET);
    case '~': return makeToken(TOKEN_TILDE);
    case '?': return makeToken(TOKEN_QMARK);
    case '!':
      return makeToken(
        matchScanner('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
    case '=':
      return makeToken(
        matchScanner('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
    case '<':
      return makeToken(
        matchScanner('<') ? TOKEN_SHIFT_LEFT :
        matchScanner('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
    case '>':
      return makeToken(
        matchScanner('>') ? TOKEN_SHIFT_RIGHT :
        matchScanner('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
    case '"': return scanString('"');
    case '\'': return scanString('\'');
    case '\n': {
      Token newlineToken;
      i32 spaceCount = 0, newIndentationLevel;

      newlineToken = makeToken(TOKEN_NEWLINE);

      /* Collapse consecutive newlines into a single newline token
       * This is necessary to properly handle indentation with
       * empty blank lines
       */
      scanner.line++;
      while (peekScanner() == '\n' || peekScanner() == '\r') {
        if (peekScanner() == '\n') {
          scanner.line++;
        }
        advanceScanner();
      }

      /* Handle indentation */
      while (peekScanner() == ' ') {
        advanceScanner();
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
