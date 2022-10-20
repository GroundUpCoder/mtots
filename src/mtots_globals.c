#include "mtots_globals.h"
#include "mtots_value.h"
#include "mtots_vm.h"
#include "mtots_object.h"
#include "mtots_memory.h"

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

static ubool implClock(i16 argCount, Value *args, Value *out) {
  *out = NUMBER_VAL((double) clock() / CLOCKS_PER_SEC);
  return UTRUE;
}

static CFunction cfunctionClock = { implClock, "clock", 0 };

static ubool implExit(i16 argCount, Value *args, Value *out) {
  int exitCode = 0;
  if (argCount > 0) {
    if (!IS_NUMBER(args[0])) {
      runtimeError("exit() requires a number but got %s",
        getKindName(args[0]));
      return UFALSE;
    }
    exitCode = (int) AS_NUMBER(args[0]);
  }
  exit(exitCode);
  return UTRUE;
}

static CFunction cfunctionExit = { implExit, "exit", 0, 1 };

static ubool implType(i16 argCount, Value *args, Value *out) {
  *out = OBJ_VAL(getClass(args[0]));
  return UTRUE;
}

static CFunction cfunctionType = { implType, "type", 1};

static ubool reprList(
    Value *buffer, size_t length, char open, char close, Value *out) {
  size_t i, charCount = 0;
  Value *stackStart = vm.stackTop;
  char *chars, *charsTop;

  /* stringify all entries in the list and push them on the stack
    * (for GC reasons) */
  for (i = 0; i < length; i++) {
    Value strval;
    if (!implRepr(1, buffer + i, &strval)) {
      return UFALSE;
    }
    push(strval);
    if (!IS_STRING(strval)) {
      runtimeError("Non-string value returned from repr()");
      return UFALSE;
    }
    charCount += AS_STRING(strval)->length;
  }

  /* Account for the brackets '[]' and the ', ' in between
    * each item. */
  charCount += length == 0 ? 2 : 2 * length;
  charsTop = chars = ALLOCATE(char, charCount + 1);

  *charsTop++ = open;
  for (i = 0; i < length; i++) {
    ObjString *str = AS_STRING(stackStart[i]);
    if (i > 0) {
      *charsTop++ = ',';
      *charsTop++ = ' ';
    }
    memcpy(charsTop, str->chars, str->length);
    charsTop += str->length;
  }
  *charsTop++ = close;
  *charsTop++ = '\0';

  if (charCount + 1 != charsTop - chars) {
    runtimeError("Assertion failed in repr(list)");
    abort();
  }

  *out = OBJ_VAL(takeString(chars, charCount));

  /* pop() the stack back to the way it was before */
  vm.stackTop = stackStart;

  return UTRUE;
}

ubool implRepr(i16 argCount, Value *args, Value *out) {
  switch (args->type) {
    case VAL_BOOL: *out = OBJ_VAL(
      (Obj*) (AS_BOOL(*args) ? vm.trueString : vm.falseString));  break;
    case VAL_NIL: *out = OBJ_VAL(vm.nilString); break;
    case VAL_NUMBER: {
      char buffer[64];
      ubool hasDot = UFALSE;
      size_t i;
      snprintf(buffer, 64, "%f", AS_NUMBER(*args));
      for (i = 0; buffer[i] != '\0'; i++) {
        if (buffer[i] == '.') {
          hasDot = UTRUE;
          break;
        }
      }
      if (hasDot) {
        size_t len = strlen(buffer);
        while (len > 0 && buffer[len - 1] == '0') {
          buffer[--len] = '\0';
        }
        if (len > 0 && buffer[len - 1] == '.') {
          buffer[--len] = '\0';
        }
      }
      *out = OBJ_VAL(copyCString(buffer));
      break;
    }
    case VAL_CFUNCTION: {
      char buffer[MAX_IDENTIFIER_LENGTH + 16];
      snprintf(buffer, MAX_IDENTIFIER_LENGTH + 16, "<function %s>",
        AS_CFUNCTION(*args)->name);
      *out = OBJ_VAL(copyCString(buffer));
      break;
    }
    case VAL_OPERATOR: {
      switch (AS_OPERATOR(args[0])) {
        case OperatorLen:
          *out = OBJ_VAL(copyCString("<operator len>"));
          break;
        default: {
          char buffer[128];
          snprintf(
            buffer, 128, "<operator unknown(%d)>", AS_OPERATOR(args[0]));
          *out = OBJ_VAL(copyCString(buffer));
          break;
        }
      }
      break;
    }
    case VAL_SENTINEL: {
      if (IS_STOP_ITERATION(args[0])) {
        *out = OBJ_VAL(copyCString("StopIteration"));
      } else {
        char buffer[128];
        snprintf(buffer, 128, "<sentinel %d>", AS_SENTINEL(args[0]));
        *out = OBJ_VAL(copyCString(buffer));
      }
      break;
    }
    case VAL_OBJ: {
      Obj *object = AS_OBJ(*args);
      switch (object->type) {
        case OBJ_CLASS: {
          char buffer[MAX_IDENTIFIER_LENGTH + 16];
          if (AS_CLASS(args[0])->isModuleClass) {
            snprintf(buffer, MAX_IDENTIFIER_LENGTH + 16, "<module-class %s>",
              AS_CLASS(args[0])->name->chars);
          } else {
            snprintf(buffer, MAX_IDENTIFIER_LENGTH + 16, "<class %s>",
              AS_CLASS(args[0])->name->chars);
          }
          *out = OBJ_VAL(copyCString(buffer));
          break;
        }
        case OBJ_CLOSURE: {
          char buffer[MAX_IDENTIFIER_LENGTH + 16];
          snprintf(buffer, MAX_IDENTIFIER_LENGTH + 16, "<function %s>",
            AS_CLOSURE(*args)->function->name->chars);
          *out = OBJ_VAL(copyCString(buffer));
          break;
        }
        case OBJ_FUNCTION: {
          char buffer[MAX_IDENTIFIER_LENGTH + 16];
          snprintf(buffer, MAX_IDENTIFIER_LENGTH + 16, "<function %s>",
            AS_FUNCTION(*args)->name->chars);
          *out = OBJ_VAL(copyCString(buffer));
          break;
        }
        case OBJ_NATIVE_CLOSURE: {
          char buffer[MAX_IDENTIFIER_LENGTH + 16];
          snprintf(buffer, MAX_IDENTIFIER_LENGTH + 16, "<function %s>",
            AS_NATIVE_CLOSURE(*args)->name);
          *out = OBJ_VAL(copyCString(buffer));
          break;
        }
        case OBJ_INSTANCE: {
          char buffer[MAX_IDENTIFIER_LENGTH + 16];
          if (AS_INSTANCE(args[0])->klass->isModuleClass) {
            snprintf(buffer, MAX_IDENTIFIER_LENGTH + 16, "<module %s>",
              AS_INSTANCE(args[0])->klass->name->chars);
          } else {
            snprintf(buffer, MAX_IDENTIFIER_LENGTH + 16, "<%s instance>",
              AS_INSTANCE(args[0])->klass->name->chars);
          }
          *out = OBJ_VAL(copyCString(buffer));
          break;
        }
        case OBJ_STRING: {
          ObjString *str = AS_STRING(*args);
          size_t i, charCount = str->length + 2; /* account for the '"'s */
          char *chars, *charsTop;

          /* account for characters that need to be escaped */
          for (i = 0; i < str->length; i++) {
            unsigned char c = (unsigned char) str->chars[i];
            if ((c >= 1 && c <= 31) || c >= 127) {
              charCount += 3; /* explicit hex escape */
            } else {
              switch (str->chars[i]) {
                case '\\':
                case '"':
                case '\n':
                case '\0':
                  charCount++;
              }
            }
          }

          charsTop = chars = ALLOCATE(char, charCount + 1);
          *charsTop++ = '"';
          for (i = 0; i < str->length; i++) {
            unsigned char c = (unsigned char) str->chars[i];
            if ((c >= 1 && c <= 31) || c >= 127) {
              u8 digit1 = c / 16, digit2 = c % 16;
              *charsTop++ = '\\';
              *charsTop++ = 'x';
              *charsTop++ = digit1 < 10 ? '0' + digit1 : 'A' + digit1 - 10;
              *charsTop++ = digit2 < 10 ? '0' + digit2 : 'A' + digit2 - 10;
            } else {
              switch (c) {
                case '\\':
                  *charsTop++ = '\\';
                  *charsTop++ = '\\';
                  break;
                case '"':
                  *charsTop++ = '\\';
                  *charsTop++ = '"';
                  break;
                case '\n':
                  *charsTop++ = '\\';
                  *charsTop++ = 'n';
                  break;
                case '\0':
                  *charsTop++ = '\\';
                  *charsTop++ = '0';
                  break;
                default:
                  *charsTop++ = c;
                  break;
              }
            }
          }
          *charsTop++ = '"';
          *charsTop++ = '\0';

          if (charsTop - chars != charCount + 1) {
            runtimeError("Assertion failed in repr(str), %ld != %ld",
              (long)(charsTop - chars),
              (long)(charCount + 1));
            abort();
          }

          *out = OBJ_VAL(takeString(chars, charCount));
          break;
        }
        case OBJ_BYTE_ARRAY: {
          char buffer[128];
          ObjByteArray *byteArray = AS_BYTE_ARRAY(args[0]);
          snprintf(buffer, 128,
            "<ByteArray (%lu)>", (unsigned long)byteArray->length);
          *out = OBJ_VAL(copyCString(buffer));
          break;
        }
        case OBJ_BYTE_ARRAY_VIEW: {
          char buffer[128];
          ObjByteArray *byteArray = AS_BYTE_ARRAY(args[0]);
          snprintf(buffer, 128,
            "<ByteArrayView (%lu)>", (unsigned long)byteArray->length);
          *out = OBJ_VAL(copyCString(buffer));
          break;
        }
        case OBJ_LIST: {
          ObjList *list = AS_LIST(*args);
          return reprList(list->buffer, list->length, '[', ']', out);
        }
        case OBJ_TUPLE: {
          ObjTuple *tuple = AS_TUPLE(*args);
          return reprList(tuple->buffer, tuple->length, '(', ')', out);
        }
        case OBJ_DICT: {
          ObjDict *dict = AS_DICT(*args);
          size_t j, charCount = 0;
          Value *stackStart = vm.stackTop, *stackptr = vm.stackTop;
          char *chars, *charsTop;
          DictIterator di;
          DictEntry *entry;

          /* stringify all entries in the dict and push them on the stack
           * (for GC reasons) */
          initDictIterator(&di, &dict->dict);
          for (j = 0; dictIteratorNext(&di, &entry);) {
            Value strkey, strval;
            if (IS_EMPTY_KEY(entry->key)) {
              continue;
            }

            if (j > 0) {
              charCount += 2; /* space for ", " */
            }
            j++;

            if (!implRepr(1, &entry->key, &strkey)) {
              return UFALSE;
            }
            if (!IS_STRING(strkey)) {
              runtimeError("Non-string value returned from repr()");
              return UFALSE;
            }
            push(strkey);
            charCount += AS_STRING(strkey)->length;

            if (IS_NIL(entry->value)) {
              /* If the value is nil, we omit the value part of the pair */
            } else {
              charCount += 2; /* space for ": " */
              if (!implRepr(1, &entry->value, &strval)) {
                return UFALSE;
              }
              if (!IS_STRING(strval)) {
                runtimeError("Non-string value returned from repr()");
                return UFALSE;
              }
              push(strval);
              charCount += AS_STRING(strval)->length;
            }
          }
          charCount += 2; /* account for enclosing '{}' */

          charsTop = chars = ALLOCATE(char, charCount + 1);
          *charsTop++ = '{';

          initDictIterator(&di, &dict->dict);
          for (j = 0; dictIteratorNext(&di, &entry);) {
            ObjString *itemstr;
            if (IS_EMPTY_KEY(entry->key)) {
              continue;
            }

            if (j > 0) {
              *charsTop++ = ',';
              *charsTop++ = ' ';
            }
            j++;

            itemstr = AS_STRING(*stackptr++);
            memcpy(charsTop, itemstr->chars, itemstr->length);
            charsTop += itemstr->length;

            if (!IS_NIL(entry->value)) {
              /* For non-nil values, we need the ": " and the value part. */
              *charsTop++ = ':';
              *charsTop++ = ' ';

              itemstr = AS_STRING(*stackptr++);
              memcpy(charsTop, itemstr->chars, itemstr->length);
              charsTop += itemstr->length;
            }
          }

          *charsTop++ = '}';
          *charsTop++ = '\0';

          if (charCount + 1 != charsTop - chars) {
            runtimeError("Assertion failed in repr(dict)");
            abort();
          }

          *out = OBJ_VAL(takeString(chars, charCount));

          /* pop() the stack back to the way it was before */
          vm.stackTop = stackStart;
          break;
        }
        case OBJ_FILE: {
          char buffer[MAX_PATH_LENGTH + 32];
          ObjFile *file = AS_FILE(args[0]);
          snprintf(buffer, MAX_PATH_LENGTH + 32, "<file %s>", file->name->chars);
          *out = OBJ_VAL(copyCString(buffer));
          break;
        }
        case OBJ_NATIVE: {
          ObjNative *n = AS_NATIVE(args[0]);
          char buffer[MAX_IDENTIFIER_LENGTH + 16];
          snprintf(buffer, MAX_IDENTIFIER_LENGTH + 16,
            "<%s native-instance>",
            n->descriptor->klass->name->chars);
          *out = OBJ_VAL(copyCString(buffer));
          break;
        }
        case OBJ_UPVALUE:
          *out = OBJ_VAL(copyCString("<upvalue>"));
          break;
        default:
          runtimeError("Unrecognized object type %s", getObjectTypeName(object->type));
          return UFALSE;
      }
      break;
    }
    default:
      runtimeError("Unrecognized value type %s", getValueTypeName(args->type));
      return UFALSE;
  }
  return UTRUE;
}

static CFunction cfunctionRepr = { implRepr, "repr", 1 };

ubool implStr(i16 argCount, Value *args, Value *out) {
  if (IS_STRING(*args)) {
    *out = *args;
    return UTRUE;
  }
  return implRepr(argCount, args, out);
}

static CFunction cfunctionStr = { implStr, "str", 1 };

static ubool implChr(i16 argCount, Value *args, Value *out) {
  char c;
  if (!IS_NUMBER(args[0])) {
    runtimeError("chr() requires a number but got %s",
      getKindName(args[0]));
    return UFALSE;
  }
  c = (char) (i32) AS_NUMBER(args[0]);
  *out = OBJ_VAL(copyString(&c, 1));
  return UTRUE;
}

static CFunction cfunctionChr = { implChr, "chr", 1 };

static ubool implOrd(i16 argCount, Value *args, Value *out) {
  ObjString *str;
  if (!IS_STRING(args[0])) {
    runtimeError("ord() requires a string but got %s",
      getKindName(args[0]));
    return UFALSE;
  }
  str = AS_STRING(args[0]);
  if (str->length != 1) {
    runtimeError(
      "ord() requires a string of length 1 but got a string of length %lu",
      (long) str->length);
    return UFALSE;
  }
  *out = NUMBER_VAL((unsigned char)str->chars[0]);
  return UTRUE;
}

static CFunction cfunctionOrd = { implOrd, "ord", 1 };

static ubool implPrint(i16 argCount, Value *args, Value *out) {
  Value strVal;
  if (!implStr(argCount, args, &strVal)) {
    return UFALSE;
  }
  if (!IS_STRING(strVal)) {
    runtimeError("'str' returned a non-string value");
    return UFALSE;
  }
  printf("%s\n", AS_STRING(strVal)->chars);
  return UTRUE;
}

static CFunction cfunctionPrint = { implPrint, "print", 1 };

typedef struct {
  ObjNativeClosure obj;
  double start, stop, step;
} ObjRangeIterator;

static ubool implRangeIterator(
    void *it, i16 argCount, Value *args, Value *out) {
  ObjRangeIterator *iter = (ObjRangeIterator*)it;
  if (iter->step >= 0) {
    if (iter->start >= iter->stop) {
      *out = STOP_ITERATION_VAL();
      return UTRUE;
    }
  } else {
    if (iter->start <= iter->stop) {
      *out = STOP_ITERATION_VAL();
      return UTRUE;
    }
  }
  *out = NUMBER_VAL(iter->start);
  iter->start += iter->step;
  return UTRUE;
}

static ObjRangeIterator *newRangeIterator(
    double start, double stop, double step) {
  ObjRangeIterator *iter = NEW_NATIVE_CLOSURE(
    ObjRangeIterator, implRangeIterator, NULL, NULL, "rangeiter",
    0, 0);
  iter->start = start;
  iter->stop = stop;
  iter->step = step;
  return iter;
}

static ubool implRange(i16 argCount, Value *args, Value *out) {
  double start = 0, stop, step = 1;
  i32 i = 0;
  for (i = 0; i < argCount; i++) {
    if (!IS_NUMBER(args[i])) {
      panic(
        "range() requires number arguments but got %s for argument %d",
        getKindName(args[i]), i);
    }
  }
  switch (argCount) {
    case 1:
      stop = AS_NUMBER(args[0]);
      break;
    case 2:
      start = AS_NUMBER(args[0]);
      stop = AS_NUMBER(args[1]);
      break;
    case 3:
      start = AS_NUMBER(args[0]);
      stop = AS_NUMBER(args[1]);
      step = AS_NUMBER(args[2]);
      break;
    default:
      panic("Invalid argc to range() (%d)", argCount);
      return UFALSE;
  }
  *out = OBJ_VAL(newRangeIterator(start, stop, step));
  return UTRUE;
}

static CFunction cfunctionRange = { implRange, "range", 1, 3 };

static ubool implOpen(i16 argCount, Value *args, Value *out) {
  FileMode mode = FILE_MODE_READ;
  const char *filename;
  if (!IS_STRING(args[0])) {
    runtimeError("open() expects string but got %s", getKindName(args[0]));
    return UFALSE;
  }
  filename = AS_STRING(args[0])->chars;
  if (argCount == 2) {
    if (!IS_STRING(args[1])) {
      runtimeError(
        "open() mode must be a string but got %s",
        getKindName(args[1]));
      return UFALSE;
    }
    mode = stringToFileMode(AS_STRING(args[1])->chars);
    if (mode == FILE_MODE_INVALID) {
      runtimeError("Invalid mode string %s", AS_STRING(args[1])->chars);
      return UFALSE;
    }
  }
  *out = OBJ_VAL(openFile(filename, mode));
  return UTRUE;
}

static CFunction cfunctionOpen = { implOpen, "open", 1, 2 };

static ubool implFloat(i16 argCount, Value *args, Value *out) {
  Value arg = args[0];
  if (IS_NUMBER(arg)) {
    *out = arg;
    return UTRUE;
  }
  if (IS_STRING(arg)) {
    ObjString *str = AS_STRING(arg);
    const char *ptr = str->chars;
    ubool decimalPoint = UFALSE;
    if (*ptr == '-' || *ptr == '+') {
      ptr++;
    }
    if (*ptr == '.') {
      ptr++;
      decimalPoint = UTRUE;
    }
    if ('0' <= *ptr && *ptr <= '9') {
      ptr++;
      while ('0' <= *ptr && *ptr <= '9') {
        ptr++;
      }
      if (!decimalPoint && *ptr == '.') {
        decimalPoint = UTRUE;
        ptr++;
        while ('0' <= *ptr && *ptr <= '9') {
          ptr++;
        }
      }
      if (*ptr == 'e' || *ptr == 'E') {
        ptr++;
        if (*ptr == '-' || *ptr == '+') {
          ptr++;
        }
        while ('0' <= *ptr && *ptr <= '9') {
          ptr++;
        }
      }
      if (*ptr == '\0') {
        *out = NUMBER_VAL(strtod(str->chars, NULL));
        return UTRUE;
      }
    }
    runtimeError("Could not convert string to float: %s", str->chars);
    return UFALSE;
  }
  runtimeError("%s is not convertible to float", getKindName(arg));
  return UFALSE;
}

static CFunction funcFloat = { implFloat, "float", 1 };

static ubool implInt(i16 argCount, Value *args, Value *out) {
  Value arg = args[0];
  if (IS_NUMBER(arg)) {
    *out = NUMBER_VAL(floor(AS_NUMBER(arg)));
    return UTRUE;
  }
  if (IS_STRING(arg)) {
    ObjString *str = AS_STRING(arg);
    const char *ptr = str->chars;
    if (*ptr == '-' || *ptr == '+') {
      ptr++;
    }
    if ('0' <= *ptr && *ptr <= '9') {
      ptr++;
      while ('0' <= *ptr && *ptr <= '9') {
        ptr++;
      }
      if (*ptr == '\0') {
        *out = NUMBER_VAL(strtod(str->chars, NULL));
        return UTRUE;
      }
    }
    runtimeError("Could not convert string to int: %s", str->chars);
    return UFALSE;
  }
  runtimeError("%s is not convertible to int", getKindName(arg));
  return UFALSE;
}

static CFunction funcInt = { implInt, "int", 1 };

static ubool implSin(i16 argCount, Value *args, Value *out) {
  *out = NUMBER_VAL(sin(AS_NUMBER(args[0])));
  return UTRUE;
}

static TypePattern argsSin[] = {
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcSin = {
  implSin, "sin", sizeof(argsSin)/sizeof(TypePattern), 0, argsSin };

static ubool implCos(i16 argCount, Value *args, Value *out) {
  *out = NUMBER_VAL(cos(AS_NUMBER(args[0])));
  return UTRUE;
}

static TypePattern argsCos[] = {
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcCos = {
  implCos, "cos", sizeof(argsCos)/sizeof(TypePattern), 0, argsCos };

static ubool implTan(i16 argCount, Value *args, Value *out) {
  *out = NUMBER_VAL(tan(AS_NUMBER(args[0])));
  return UTRUE;
}

static TypePattern argsTan[] = {
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcTan = {
  implTan, "tan", sizeof(argsTan)/sizeof(TypePattern), 0, argsTan };

static ubool implAbs(i16 argCount, Value *args, Value *out) {
  double value = AS_NUMBER(args[0]);
  *out = NUMBER_VAL(value < 0 ? -value : value);
  return UTRUE;
}

static TypePattern argsAbs[] = {
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcAbs = {
  implAbs, "abs", sizeof(argsAbs)/sizeof(TypePattern), 0, argsAbs };

static ubool implTuple(i16 argCount, Value *args, Value *out) {
  ObjList *list = AS_LIST(args[0]);
  *out = OBJ_VAL(copyTuple(list->buffer, list->length));
  return UTRUE;
}

static TypePattern argsTuple[] = {
  { TYPE_PATTERN_LIST },
};

static CFunction cfunctionTuple = { implTuple, "__tuple__", 1, 0, argsTuple };

static ubool implSort(i16 argCount, Value *args, Value *out) {
  ObjList *list = AS_LIST(args[0]);
  ObjList *keys =
    argCount < 2 ?
      NULL :
      IS_NIL(args[1]) ?
        NULL :
        AS_LIST(args[1]);
  sortList(list, keys);
  return UTRUE;
}

static TypePattern argsSort[] = {
  { TYPE_PATTERN_LIST },
  { TYPE_PATTERN_LIST_OR_NIL },
};

static CFunction cfunctionSort = { implSort, "__sort__", 1, 2, argsSort };

static void defineStandardIOGlobals() {
  ObjString *name;

  name = copyCString("stdin");
  push(OBJ_VAL(name));
  vm.stdinFile = newFile(stdin, UTRUE, name, FILE_MODE_READ);
  pop(); /* name */
  defineGlobal("stdin", OBJ_VAL(vm.stdinFile));

  name = copyCString("stdout");
  push(OBJ_VAL(name));
  vm.stdoutFile = newFile(stdout, UTRUE, name, FILE_MODE_WRITE);
  pop(); /* name */
  defineGlobal("stdout", OBJ_VAL(vm.stdoutFile));

  name = copyCString("stderr");
  push(OBJ_VAL(name));
  vm.stderrFile = newFile(stderr, UTRUE, name, FILE_MODE_WRITE);
  pop(); /* name */
  defineGlobal("stderr", OBJ_VAL(vm.stderrFile));
}

void defineDefaultGlobals() {
  defineGlobal("PI", NUMBER_VAL(M_PI));
  defineGlobal("NAN", NUMBER_VAL(0.0/0.0));
  defineGlobal("INFINITY", NUMBER_VAL(1.0/0.0));

  defineGlobal("len", OPERATOR_VAL(OperatorLen));

  defineGlobal("clock", CFUNCTION_VAL(&cfunctionClock));
  defineGlobal("exit", CFUNCTION_VAL(&cfunctionExit));
  defineGlobal("type", CFUNCTION_VAL(&cfunctionType));
  defineGlobal("repr", CFUNCTION_VAL(&cfunctionRepr));
  defineGlobal("str", CFUNCTION_VAL(&cfunctionStr));
  defineGlobal("chr", CFUNCTION_VAL(&cfunctionChr));
  defineGlobal("ord", CFUNCTION_VAL(&cfunctionOrd));
  defineGlobal("print", CFUNCTION_VAL(&cfunctionPrint));
  defineGlobal("range", CFUNCTION_VAL(&cfunctionRange));
  defineGlobal("open", CFUNCTION_VAL(&cfunctionOpen));
  defineGlobal("float", CFUNCTION_VAL(&funcFloat));
  defineGlobal("int", CFUNCTION_VAL(&funcInt));
  defineGlobal("sin", CFUNCTION_VAL(&funcSin));
  defineGlobal("cos", CFUNCTION_VAL(&funcCos));
  defineGlobal("tan", CFUNCTION_VAL(&funcTan));
  defineGlobal("abs", CFUNCTION_VAL(&funcAbs));
  defineGlobal("StopIteration", STOP_ITERATION_VAL());

  defineGlobal("__sort__", CFUNCTION_VAL(&cfunctionSort));
  defineGlobal("__tuple__", CFUNCTION_VAL(&cfunctionTuple));

  tableSet(&vm.globals, vm.sentinelClass->name, OBJ_VAL(vm.sentinelClass));
  tableSet(&vm.globals, vm.nilClass->name, OBJ_VAL(vm.nilClass));
  tableSet(&vm.globals, vm.boolClass->name, OBJ_VAL(vm.boolClass));
  tableSet(&vm.globals, vm.numberClass->name, OBJ_VAL(vm.numberClass));
  tableSet(&vm.globals, vm.stringClass->name, OBJ_VAL(vm.stringClass));
  tableSet(&vm.globals, vm.byteArrayClass->name, OBJ_VAL(vm.byteArrayClass));
  tableSet(&vm.globals, vm.byteArrayViewClass->name, OBJ_VAL(vm.byteArrayViewClass));
  tableSet(&vm.globals, vm.listClass->name, OBJ_VAL(vm.listClass));
  tableSet(&vm.globals, vm.tupleClass->name, OBJ_VAL(vm.tupleClass));
  tableSet(&vm.globals, vm.dictClass->name, OBJ_VAL(vm.dictClass));
  tableSet(&vm.globals, vm.functionClass->name, OBJ_VAL(vm.functionClass));
  tableSet(&vm.globals, vm.operatorClass->name, OBJ_VAL(vm.operatorClass));
  tableSet(&vm.globals, vm.classClass->name, OBJ_VAL(vm.classClass));
  tableSet(&vm.globals, vm.fileClass->name, OBJ_VAL(vm.fileClass));

  defineStandardIOGlobals();
}
