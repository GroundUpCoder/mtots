#include "mtots_class_class.h"

#include "mtots_vm.h"

static ubool implClassGetName(i16 argCount, Value *args, Value *out) {
  ObjClass *cls = AS_CLASS(args[0]);
  *out = STRING_VAL(cls->name);
  return UTRUE;
}

static TypePattern argsClassGetName[] = {
  {TYPE_PATTERN_CLASS },
};
static CFunction funcClassGetName = {
  implClassGetName, "getName", 1, 0, argsClassGetName
};

void initClassClass() {
  String *tmpstr;
  CFunction *staticMethods[] = {
    &funcClassGetName,
  };
  size_t i;
  ObjClass *cls;

  tmpstr = internCString("Class");
  push(STRING_VAL(tmpstr));
  cls = vm.classClass = newClass(tmpstr);
  cls->isBuiltinClass = UTRUE;
  pop();

  for (i = 0; i < sizeof(staticMethods) / sizeof(CFunction*); i++) {
    tmpstr = internCString(staticMethods[i]->name);
    push(STRING_VAL(tmpstr));
    mapSetStr(
      &cls->staticMethods, tmpstr, CFUNCTION_VAL(staticMethods[i]));
    pop();
  }
}
