#include "mtots_class_class.h"

#include "mtots_vm.h"

static ubool implClassGetName(i16 argCount, Value *args, Value *out) {
  ObjClass *cls = AS_CLASS(args[-1]);
  *out = STRING_VAL(cls->name);
  return UTRUE;
}

static CFunction funcClassGetName = { implClassGetName, "getName", 0 };

void initClassClass() {
  String *tmpstr;
  CFunction *methods[] = {
    &funcClassGetName,
  };
  size_t i;
  ObjClass *cls;

  tmpstr = internCString("Class");
  push(STRING_VAL(tmpstr));
  cls = vm.classClass = newClass(tmpstr);
  cls->isBuiltinClass = UTRUE;
  pop();

  for (i = 0; i < sizeof(methods) / sizeof(CFunction*); i++) {
    tmpstr = internCString(methods[i]->name);
    push(STRING_VAL(tmpstr));
    methods[i]->receiverType.type = TYPE_PATTERN_CLASS;
    mapSetStr(
      &cls->methods, tmpstr, CFUNCTION_VAL(methods[i]));
    pop();
  }
}
