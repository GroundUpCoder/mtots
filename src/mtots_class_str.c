#include "mtots_class_str.h"
#include "mtots_object.h"
#include "mtots_value.h"
#include "mtots_vm.h"
#include "mtots_globals.h"
#include "mtots_class_list.h"
#include "mtots_memory.h"

#include <string.h>
#include <stdlib.h>

static ubool implStrGetItem(i16 argCount, Value *args, Value *out) {
  Value receiver = args[-1];
  ObjString *str;
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
  *out = OBJ_VAL(copyString(str->chars + index, 1));
  return UTRUE;
}

static CFunction funcStrGetItem = { implStrGetItem, "__getitem__", 1 };

static ubool implStrSlice(i16 argCount, Value *args, Value *out) {
  Value receiver = args[-1];
  ObjString *str;
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
  *out = OBJ_VAL(copyString(str->chars + lower, upper - lower));
  return UTRUE;
}

static CFunction funcStrSlice = { implStrSlice, "__slice__", 2 };

static ubool implStrMod(i16 argCount, Value *args, Value *out) {
  Value receiver = args[-1];
  ObjString *fmt;
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
  push(OBJ_VAL(strlist));

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
        ObjString *itemstr = AS_STRING(strlist->buffer[j++]);
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

  *out = OBJ_VAL(copyString(buffer, length));

  pop(); /* strlist */
  return UTRUE;
}

static CFunction funcStrMod = { implStrMod, "__mod__", 1 };

void initStringClass() {
  ObjString *tmpstr;
  CFunction *methods[] = {
    &funcStrGetItem,
    &funcStrSlice,
    &funcStrMod
  };
  size_t i;
  ObjClass *cls;

  tmpstr = copyCString("String");
  push(OBJ_VAL(tmpstr));
  cls = vm.stringClass = newClass(tmpstr);
  cls->isBuiltinClass = UTRUE;
  pop();

  for (i = 0; i < sizeof(methods) / sizeof(CFunction*); i++) {
    tmpstr = copyCString(methods[i]->name);
    push(OBJ_VAL(tmpstr));
    tableSet(
      &cls->methods, tmpstr, CFUNCTION_VAL(methods[i]));
    pop();
  }
}
