#include "mtots_m_json.h"

#include "mtots_common.h"
#include "mtots_value.h"
#include "mtots_object.h"
#include "mtots_vm.h"

#include "mtots_m_json_parse.h"

static ubool implLoads(i16 argCount, Value *args, Value *out) {
  ObjString *str = AS_STRING(args[0]);
  JSONParseState state;
  initJSONParseState(&state, str->chars);
  if (!parseJSON(&state)) {
    return UFALSE;
  }
  *out = pop();
  return UTRUE;
}

static TypePattern argsLoads[] = {
  { TYPE_PATTERN_STRING },
};

static CFunction funcLoads = { implLoads, "loads", 1, 0, argsLoads };

static ubool impl(i16 argCount, Value *args, Value *out) {
  ObjInstance *module = AS_INSTANCE(args[0]);
  CFunction *functions[] = {
    &funcLoads,
  };
  size_t i;

  for (i = 0; i < sizeof(functions)/sizeof(CFunction*); i++) {
    tableSetN(&module->fields, functions[i]->name, CFUNCTION_VAL(functions[i]));
  }

  return UTRUE;
}

static CFunction func = { impl, "json", 1 };

void addNativeModuleJson() {
  addNativeModule(&func);
}
