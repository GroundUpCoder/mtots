#include "mtots_class_class.h"

#include "mtots_vm.h"

static ubool implClassGetName(i16 argCount, Value *args, Value *out) {
  ObjClass *cls = AS_CLASS(args[-1]);
  *out = OBJ_VAL(cls->name);
  return UTRUE;
}

static CFunction funcClassGetName = { implClassGetName, "getName", 0 };

void initClassClass() {
  ObjString *tmpstr;
  CFunction *methods[] = {
    &funcClassGetName,
  };
  size_t i;
  ObjClass *cls;

  tmpstr = copyCString("Class");
  push(OBJ_VAL(tmpstr));
  cls = vm.classClass = newClass(tmpstr);
  cls->isBuiltinClass = UTRUE;
  pop();

  for (i = 0; i < sizeof(methods) / sizeof(CFunction*); i++) {
    tmpstr = copyCString(methods[i]->name);
    push(OBJ_VAL(tmpstr));
    methods[i]->receiverType.type = TYPE_PATTERN_CLASS;
    tableSet(
      &cls->methods, tmpstr, CFUNCTION_VAL(methods[i]));
    pop();
  }
}
