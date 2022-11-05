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

/* TODO: For now, this is basically just a simple join, but in
 * the future, this function will have to evolve to match the
 * behaviors you would expect from a path join */
static ubool implJoin(i16 argCount, Value *args, Value *out) {
  size_t i, len = argCount - 1;
  char *chars, *p;
  for (i = 0; i < argCount; i++) {
    if (!IS_STRING(args[i])) {
      runtimeError(
        "os.join() requires strings, but found %s",
        getKindName(args[i]));
      return UFALSE;
    }
    len += AS_STRING(args[i])->length;
  }
  p = chars = ALLOCATE(char, len + 1);
  for (i = 0; i < argCount; i++) {
    ObjString *s = AS_STRING(args[i]);
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

static CFunction funcJoin = {
  implJoin, "join", 1, MAX_ARG_COUNT,
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
    dictSetN(&module->fields, functions[i]->name, CFUNCTION_VAL(functions[i]));
  }

  dictSetN(&module->fields, "name", OBJ_VAL(copyCString(OS_NAME)));
  dictSetN(&module->fields, "sep", OBJ_VAL(copyCString(PATH_SEP_STR)));

  return UTRUE;
}

static CFunction func = { impl, "os", 1 };

void addNativeModuleOs() {
  addNativeModule(&func);
}
