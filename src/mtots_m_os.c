#include "mtots_m_os.h"

#include "mtots_vm.h"

#include <string.h>
#include <stdlib.h>

static ubool implDirname(i16 argCount, Value *args, Value *out) {
  String *string = AS_STRING(args[0]);
  const char *chars = string->chars;
  size_t i = string->length;
  while (i > 0 && chars[i - 1] == PATH_SEP) {
    i--;
  }
  for (; i > 0; i--) {
    if (chars[i - 1] == PATH_SEP) {
      i--;
      break;
    }
  }
  *out = STRING_VAL(internString(chars, i));
  return UTRUE;
}

static TypePattern argsDirname[] = {
  { TYPE_PATTERN_STRING },
};

static CFunction funcDirname = { implDirname, "dirname", 1, 0, argsDirname };

static ubool implBasename(i16 argCount, Value *args, Value *out) {
  String *string = AS_STRING(args[0]);
  const char *chars = string->chars;
  size_t i = string->length, end;
  while (i > 0 && chars[i - 1] == PATH_SEP) {
    i--;
  }
  end = i;
  for (; i > 0; i--) {
    if (chars[i - 1] == PATH_SEP) {
      break;
    }
  }
  *out = STRING_VAL(internString(chars + i, end - i));
  return UTRUE;
}

static TypePattern argsBasename[] = {
  { TYPE_PATTERN_STRING },
};

static CFunction funcBasename = {implBasename, "basename", 1, 0, argsBasename};

/* TODO: For now, this is basically just a simple join, but in
 * the future, this function will have to evolve to match the
 * behaviors you would expect from a path join */
static ubool implJoin(i16 argCount, Value *args, Value *out) {
  ObjList *list = AS_LIST(args[0]);
  size_t i, len = list->length;
  StringBuffer sb;

  initStringBuffer(&sb);

  for (i = 0; i < len; i++) {
    String *item;
    if (i > 0) {
      sbputchar(&sb, PATH_SEP);
    }
    if (!IS_STRING(list->buffer[i])) {
      panic("Expected String but got %s", getKindName(list->buffer[i]));
    }
    item = AS_STRING(list->buffer[i]);
    sbputstrlen(&sb, item->chars, item->length);
  }

  *out = STRING_VAL(internString(sb.chars, sb.length));

  freeStringBuffer(&sb);

  return UTRUE;
}

static TypePattern argsJoin[] = {
  { TYPE_PATTERN_LIST },
};

static CFunction funcJoin = { implJoin, "join", 1, 0, argsJoin };

static ubool impl(i16 argCount, Value *args, Value *out) {
  ObjInstance *module = AS_INSTANCE(args[0]);
  CFunction *cfunctions[] = {
    &funcDirname,
    &funcBasename,
    &funcJoin,
  };
  size_t i;

  for (i = 0; i < sizeof(cfunctions)/sizeof(CFunction*); i++) {
    mapSetN(&module->fields, cfunctions[i]->name, CFUNCTION_VAL(cfunctions[i]));
  }

  mapSetN(&module->fields, "name", STRING_VAL(internCString(OS_NAME)));
  mapSetN(&module->fields, "sep", STRING_VAL(internCString(PATH_SEP_STR)));

  return UTRUE;
}

static CFunction func = { impl, "os", 1 };

void addNativeModuleOs() {
  addNativeModule(&func);
}
