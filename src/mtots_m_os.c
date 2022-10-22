#include "mtots_m_os.h"

#include "mtots_common.h"
#include "mtots_value.h"
#include "mtots_object.h"
#include "mtots_vm.h"
#include "mtots_memory.h"

#include <string.h>

static ubool implDirname(i16 argCount, Value *args, Value *out) {
  ObjString *str = AS_STRING(args[0]);
  size_t i = str->length;
  while (i > 0 && str->chars[i - 1] == PATH_SEP) {
    i--;
  }
  for (; i > 0; i--) {
    if (str->chars[i - 1] == PATH_SEP) {
      i--;
      break;
    }
  }
  *out = OBJ_VAL(copyString(str->chars, i));
  return UTRUE;
}

static TypePattern argsDirname[] = {
  { TYPE_PATTERN_STRING },
};

static CFunction funcDirname = {
  implDirname, "dirname", 1, 0, argsDirname,
};

static ubool implBasename(i16 argCount, Value *args, Value *out) {
  ObjString *str = AS_STRING(args[0]);
  size_t i = str->length, end;
  while (i > 0 && str->chars[i - 1] == PATH_SEP) {
    i--;
  }
  end = i;
  for (; i > 0; i--) {
    if (str->chars[i - 1] == PATH_SEP) {
      break;
    }
  }
  *out = OBJ_VAL(copyString(str->chars + i, end - i));
  return UTRUE;
}

static TypePattern argsBasename[] = {
  { TYPE_PATTERN_STRING },
};

static CFunction funcBasename = {
  implBasename, "basename", 1, 0, argsBasename,
};

static ubool implJoin(i16 argCount, Value *args, Value *out) {
  ObjList *list = AS_LIST(args[0]);
  size_t i, len = list->length - 1;
  char *chars, *p;
  for (i = 0; i < list->length; i++) {
    if (!IS_STRING(list->buffer[i])) {
      runtimeError(
        "os.join() requires a list of strings, but found %s in list",
        getKindName(list->buffer[i]));
      return UFALSE;
    }
    len += AS_STRING(list->buffer[i])->length;
  }
  p = chars = ALLOCATE(char, len + 1);
  for (i = 0; i < list->length; i++) {
    ObjString *s = AS_STRING(list->buffer[i]);
    if (i > 0) {
      *p++ = PATH_SEP;
    }
    memcpy(p, s->chars, s->length);
    p += s->length;
  }
  *p = '\0';
  if (p - chars != len) {
    panic("internal consistency error in os.join()");
  }
  *out = OBJ_VAL(takeString(chars, len));
  return UTRUE;
}

static TypePattern argsJoin[] = {
  { TYPE_PATTERN_LIST },
};

static CFunction funcJoin = {
  implJoin, "join", 1, 0, argsJoin,
};

static ubool impl(i16 argCount, Value *args, Value *out) {
  ObjInstance *module = AS_INSTANCE(args[0]);

  CFunction *functions[] = {
    &funcDirname,
    &funcBasename,
    &funcJoin,
  };
  size_t i;

  for (i = 0; i < sizeof(functions)/sizeof(CFunction*); i++) {
    tableSetN(&module->fields, functions[i]->name, CFUNCTION_VAL(functions[i]));
  }

  tableSetN(&module->fields, "name", OBJ_VAL(copyCString(OS_NAME)));
  tableSetN(&module->fields, "sep", OBJ_VAL(copyCString(PATH_SEP_STR)));

  return UTRUE;
}

static CFunction func = { impl, "os", 1 };

void addNativeModuleOs() {
  addNativeModule(&func);
}
