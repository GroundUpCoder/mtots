#include "mtots_m_os.h"

#include "mtots_common.h"
#include "mtots_value.h"
#include "mtots_object.h"
#include "mtots_vm.h"

static ubool impl(i16 argCount, Value *args, Value *out) {
  ObjInstance *module = AS_INSTANCE(args[0]);

  tableSetN(&module->fields, "name", OBJ_VAL(copyCString(OS_NAME)));

  return UTRUE;
}

static CFunction func = { impl, "os", 1 };

void addNativeModuleOs() {
  addNativeModule(&func);
}
