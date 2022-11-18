#include "mtots_globals.h"
#include "mtots_vm.h"

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#define PI   3.14159265358979323846264338327950288

static ubool implClock(i16 argCount, Value *args, Value *out) {
  *out = NUMBER_VAL((double) clock() / CLOCKS_PER_SEC);
  return UTRUE;
}

static CFunction cfunctionClock = { implClock, "clock", 0 };

static ubool implExit(Ref out, RefSet args) {
  int exitCode = 0;
  if (args.length > 0) {
    exitCode = getNumber(refAt(args, 0));
  }
  exit(exitCode);
  return UTRUE;
}

static CFunc cfuncExit = { implExit, "exit", 0, 1 };

static ubool implType(Ref out, RefSet args) {
  getClass(out, refAt(args, 0));
  return UTRUE;
}

static CFunc cfuncType = { implType, "type", 1};

static ubool implRepr(i16 argCount, Value *args, Value *out) {
  StringBuffer sb;
  initStringBuffer(&sb);
  if (!valueRepr(&sb, args[0])) {
    freeStringBuffer(&sb);
    return UFALSE;
  }
  *out = STRING_VAL(internString(sb.chars, sb.length));
  freeStringBuffer(&sb);
  return UTRUE;
}

static CFunction cfunctionRepr = { implRepr, "repr", 1 };

static ubool implStr(i16 argCount, Value *args, Value *out) {
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
  *out = STRING_VAL(internString(&c, 1));
  return UTRUE;
}

static CFunction cfunctionChr = { implChr, "chr", 1 };

static ubool implOrd(i16 argCount, Value *args, Value *out) {
  String *str;
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
  *out = NUMBER_VAL((u8)str->chars[0]);
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

typedef struct ObjRangeIterator {
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
  *out = OBJ_VAL_EXPLICIT((Obj*)newRangeIterator(start, stop, step));
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
  *out = FILE_VAL(openFile(filename, mode));
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
    String *str = AS_STRING(arg);
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
    String *str = AS_STRING(arg);
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
  *out = TUPLE_VAL(copyTuple(list->buffer, list->length));
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
  String *name;

  name = internCString("stdin");
  push(STRING_VAL(name));
  vm.stdinFile = newFile(stdin, UTRUE, name, FILE_MODE_READ);
  pop(); /* name */
  defineGlobal("stdin", FILE_VAL(vm.stdinFile));

  name = internCString("stdout");
  push(STRING_VAL(name));
  vm.stdoutFile = newFile(stdout, UTRUE, name, FILE_MODE_WRITE);
  pop(); /* name */
  defineGlobal("stdout", FILE_VAL(vm.stdoutFile));

  name = internCString("stderr");
  push(STRING_VAL(name));
  vm.stderrFile = newFile(stderr, UTRUE, name, FILE_MODE_WRITE);
  pop(); /* name */
  defineGlobal("stderr", FILE_VAL(vm.stderrFile));
}

void defineDefaultGlobals() {
  defineGlobal("PI", NUMBER_VAL(PI));

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
  {
    double one = 1.0;
    double zero = one - one;
    defineGlobal("NAN", NUMBER_VAL(zero / zero));
    defineGlobal("INFINITY", NUMBER_VAL(one / zero));
  }
#else
  defineGlobal("NAN", NUMBER_VAL(0.0/0.0));
  defineGlobal("INFINITY", NUMBER_VAL(1.0/0.0));
#endif

  defineGlobal("len", OPERATOR_VAL(OperatorLen));

  defineGlobal("clock", CFUNCTION_VAL(&cfunctionClock));
  defineGlobal("exit", CFUNC_VAL(&cfuncExit));
  defineGlobal("type", CFUNC_VAL(&cfuncType));
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

  mapSetStr(&vm.globals, vm.sentinelClass->name, CLASS_VAL(vm.sentinelClass));
  mapSetStr(&vm.globals, vm.nilClass->name, CLASS_VAL(vm.nilClass));
  mapSetStr(&vm.globals, vm.boolClass->name, CLASS_VAL(vm.boolClass));
  mapSetStr(&vm.globals, vm.numberClass->name, CLASS_VAL(vm.numberClass));
  mapSetStr(&vm.globals, vm.stringClass->name, CLASS_VAL(vm.stringClass));
  mapSetStr(&vm.globals, vm.byteArrayClass->name, CLASS_VAL(vm.byteArrayClass));
  mapSetStr(&vm.globals, vm.byteArrayViewClass->name, CLASS_VAL(vm.byteArrayViewClass));
  mapSetStr(&vm.globals, vm.listClass->name, CLASS_VAL(vm.listClass));
  mapSetStr(&vm.globals, vm.tupleClass->name, CLASS_VAL(vm.tupleClass));
  mapSetStr(&vm.globals, vm.mapClass->name, CLASS_VAL(vm.mapClass));
  mapSetStr(&vm.globals, vm.functionClass->name, CLASS_VAL(vm.functionClass));
  mapSetStr(&vm.globals, vm.operatorClass->name, CLASS_VAL(vm.operatorClass));
  mapSetStr(&vm.globals, vm.classClass->name, CLASS_VAL(vm.classClass));
  mapSetStr(&vm.globals, vm.fileClass->name, CLASS_VAL(vm.fileClass));

  defineStandardIOGlobals();
}
