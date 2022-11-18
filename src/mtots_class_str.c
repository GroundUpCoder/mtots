#include "mtots_class_str.h"
#include "mtots_vm.h"
#include "mtots_globals.h"
#include "mtots_memory.h"

#include <string.h>
#include <stdlib.h>

static ubool implStrGetItem(i16 argCount, Value *args, Value *out) {
  Value receiver = args[-1];
  String *str;
  i32 index;
  if (!IS_STRING(receiver)) {
    runtimeError("Expected string as receiver to String.__getitem__()");
    return UFALSE;
  }
  str = AS_STRING(receiver);
  if (!IS_NUMBER(args[0])) {
    runtimeError(
      "Expected index to String.__getitem__() "
      "to be a number but got %s",
      getKindName(args[0]));
    return UFALSE;
  }
  index = AS_NUMBER(args[0]);
  if (index < 0) {
    index += str->length;
  }
  if (index < 0 || index >= str->length) {
    runtimeError("List index out of bounds");
    return UFALSE;
  }
  *out = STRING_VAL(internString(str->chars + index, 1));
  return UTRUE;
}

static CFunction funcStrGetItem = { implStrGetItem, "__getitem__", 1 };

static ubool implStrSlice(i16 argCount, Value *args, Value *out) {
  Value receiver = args[-1];
  String *str;
  i32 lower, upper;
  if (!IS_STRING(receiver)) {
    runtimeError("Expected string as receiver to String.__slice__()");
    return UFALSE;
  }
  str = AS_STRING(receiver);
  if (IS_NIL(args[0])) {
    lower = 0;
  } else {
    if (!IS_NUMBER(args[0])) {
      runtimeError(
        "Expected argument 'lower' to String.__slice__() "
        "to be a number but got %s",
        getKindName(args[0]));
      return UFALSE;
    }
    lower = AS_NUMBER(args[0]);
  }
  if (lower < 0) {
    lower += str->length;
  }
  if (lower < 0 || lower >= str->length) {
    runtimeError("Lower slice index out of bounds");
    return UFALSE;
  }
  if (IS_NIL(args[1])) {
    upper = str->length;
  } else {
    if (!IS_NUMBER(args[1])) {
      runtimeError(
        "Expected argument 'upper' to String.__slice__() "
        "to be a number but got %s",
        getKindName(args[1]));
      return UFALSE;
    }
    upper = AS_NUMBER(args[1]);
  }
  if (upper < 0) {
    upper += str->length;
  }
  if (upper < 0 || upper > str->length) {
    runtimeError("Upper slice index out of bounds");
    return UFALSE;
  }
  *out = STRING_VAL(internString(str->chars + lower, upper - lower));
  return UTRUE;
}

static CFunction funcStrSlice = { implStrSlice, "__slice__", 2 };

static ubool implStrMod(i16 argCount, Value *args, Value *out) {
  Value receiver = args[-1];
  String *fmt;
  ObjList *arglist, *strlist;
  size_t i, j, k, length = 0;
  char *buffer;

  /* i: index fmt->chars
   * j: index arglist->buffer
   * k: index buffer (output string buffer) */

  if (!IS_STRING(receiver)) {
    runtimeError("Expected string as receiver to String.__mod__()");
    return UFALSE;
  }
  fmt = AS_STRING(receiver);
  if (!IS_LIST(args[0])) {
    runtimeError("Expected List as argument to String.__mod__()");
    return UFALSE;
  }
  arglist = AS_LIST(args[0]);
  strlist = newList(arglist->length);
  push(LIST_VAL(strlist));

  /* compute length and convert all the arguments to strings */
  for (i = j = 0; i < fmt->length; i++) {
    if (fmt->chars[i] == '%') {
      i++;
      if (i < fmt->length && fmt->chars[i] != '%') {
        Value itemstrval = NIL_VAL();
        if (j >= arglist->length) {
          if (fmt->chars[i] != 'r' && fmt->chars[i] != 's') {
            runtimeError("Invalid format indicator '%%%c'", fmt->chars[i]);
          } else {
            runtimeError("Not enough arguments for format string");
          }
          return UFALSE;
        }
        switch (fmt->chars[i]) {
          case 's':
            if (!implStr(1, arglist->buffer + j, &itemstrval)) {
              return UFALSE;
            }
            break;
          case 'r':
            if (!implRepr(1, arglist->buffer + j, &itemstrval)) {
              return UFALSE;
            }
            break;
          default:
            runtimeError("Invalid format indicator '%%%c'", fmt->chars[i]);
            return UFALSE;
        }
        if (!IS_STRING(itemstrval)) {
          runtimeError("str or repr returned non-string value");
          return UFALSE;
        }
        length += AS_STRING(itemstrval)->length;
        strlist->buffer[j++] = itemstrval;
      } else {
        length++;
      }
    } else {
      length++;
    }
  }
  if (j < arglist->length) {
    runtimeError("Too many arguments for format string");
    return UFALSE;
  }

  /* construct the actual string */
  buffer = ALLOCATE(char, length + 1);
  for (i = j = k = 0; i < fmt->length; i++) {
    if (fmt->chars[i] == '%') {
      i++;
      if (i < fmt->length && fmt->chars[i] != '%') {
        String *itemstr = AS_STRING(strlist->buffer[j++]);
        memcpy(buffer + k, itemstr->chars, itemstr->length);
        k += itemstr->length;
      } else {
        buffer[k++] = '%';
      }
    } else {
      buffer[k++] = fmt->chars[i];
    }
  }
  buffer[k] = '\0';

  /* check some assertions */
  if (length != k) {
    abort();
  }
  if (strlist->length != j) {
    abort();
  }

  *out = STRING_VAL(internString(buffer, length));

  pop(); /* strlist */
  return UTRUE;
}

static CFunction funcStrMod = { implStrMod, "__mod__", 1 };

static char DEFAULT_STRIP_SET[] = " \t\r\n";

static ubool containsChar(const char *charSet, char ch) {
  for (; *charSet != '\0'; charSet++) {
    if (*charSet == ch) {
      return UTRUE;
    }
  }
  return UFALSE;
}

static ubool implStrStrip(i16 argCount, Value *args, Value *out) {
  String *str = AS_STRING(args[-1]);
  const char *stripSet = DEFAULT_STRIP_SET;
  const char *start = str->chars;
  const char *end = str->chars + str->length;
  if (argCount > 0) {
    stripSet = AS_STRING(args[0])->chars;
  }
  while (start < end && containsChar(stripSet, start[0])) {
    start++;
  }
  while (start < end && containsChar(stripSet, end[-1])) {
    end--;
  }
  *out = STRING_VAL(internString(start, end - start));
  return UTRUE;
}

static TypePattern argsStrStrip[] = {
  { TYPE_PATTERN_STRING },
};

static CFunction funcStrStrip = { implStrStrip, "strip", 0, 1, argsStrStrip };

static size_t cStrReplace(
    const char *s, const char *oldstr, const char *newstr, char *out) {
  char *outp = out;
  size_t len = 0, oldstrlen = strlen(oldstr), newstrlen = strlen(newstr);

  while (*s) {
    if (strncmp(s, oldstr, oldstrlen) == 0) {
      len += newstrlen;
      if (outp) {
        memcpy(outp, newstr, newstrlen);
        outp += newstrlen;
      }
      s += oldstrlen;
    } else {
      len++;
      if (outp) *outp++ = *s;
      s++;
    }
  }

  return len;
}

static ubool implStrReplace(i16 argCount, Value *args, Value *out) {
  String *orig = AS_STRING(args[-1]);
  String *oldstr = AS_STRING(args[0]);
  String *newstr = AS_STRING(args[1]);
  size_t len = cStrReplace(orig->chars, oldstr->chars, newstr->chars, NULL);
  char *chars = malloc(sizeof(char) * (len + 1));
  cStrReplace(orig->chars, oldstr->chars, newstr->chars, chars);
  chars[len] = '\0';
  *out = STRING_VAL(internOwnedString(chars, len));
  return UTRUE;
}

static TypePattern argsStrReplace[] = {
  { TYPE_PATTERN_STRING },
  { TYPE_PATTERN_STRING },
};

static CFunction funcStrReplace = { implStrReplace, "replace", 2, 0,
  argsStrReplace };

static ubool implStrJoin(i16 argCount, Value *args, Value *out) {
  String *sep = AS_STRING(args[-1]);
  ObjList *list = AS_LIST(args[0]);
  size_t i, len = (list->length - 1) * sep->length;
  char *chars, *p;
  for (i = 0; i < list->length; i++) {
    if (!IS_STRING(list->buffer[i])) {
      runtimeError(
        "String.join() requires a list of strings, but found %s in the list",
        getKindName(list->buffer[i]));
      return UFALSE;
    }
    len += AS_STRING(list->buffer[i])->length;
  }
  chars = p = malloc(sizeof(char) * (len + 1));
  for (i = 0; i < list->length; i++) {
    String *item = AS_STRING(list->buffer[i]);
    if (i > 0) {
      memcpy(p, sep->chars, sep->length);
      p += sep->length;
    }
    memcpy(p, item->chars, item->length);
    p += item->length;
  }
  *p = '\0';
  if (p - chars != len) {
    panic("Consistency error in String.join()");
  }
  *out = STRING_VAL(internOwnedString(chars, len));
  return UTRUE;
}

static TypePattern argsStrJoin[] = {
  { TYPE_PATTERN_LIST },
};

static CFunction funcStrJoin = {
  implStrJoin, "join", 1, 0, argsStrJoin,
};

void initStringClass() {
  String *tmpstr;
  CFunction *methods[] = {
    &funcStrGetItem,
    &funcStrSlice,
    &funcStrMod,
    &funcStrStrip,
    &funcStrReplace,
    &funcStrJoin,
  };
  size_t i;
  ObjClass *cls;

  tmpstr = internCString("String");
  push(STRING_VAL(tmpstr));
  cls = vm.stringClass = newClass(tmpstr);
  cls->isBuiltinClass = UTRUE;
  pop();

  for (i = 0; i < sizeof(methods) / sizeof(CFunction*); i++) {
    methods[i]->receiverType.type = TYPE_PATTERN_STRING;
    tmpstr = internCString(methods[i]->name);
    push(STRING_VAL(tmpstr));
    mapSetStr(
      &cls->methods, tmpstr, CFUNCTION_VAL(methods[i]));
    pop();
  }
}
