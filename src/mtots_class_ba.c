#include "mtots_class_ba.h"
#include "mtots_vm.h"

static ubool implByteArrayGetItem(i16 argCount, Value *args, Value *out) {
  Value receiver = args[-1];
  ObjByteArray *ba;
  i32 index;
  if (!IS_BYTE_ARRAY(receiver)) {
    runtimeError("Expected ByteArray as receiver to ByteArray.__getitem__()");
    return UFALSE;
  }
  ba = AS_BYTE_ARRAY(receiver);
  index = AS_NUMBER(args[0]);
  if (index < 0) {
    index += ba->size;
  }
  if (index < 0 || index >= ba->size) {
    runtimeError("Index %lu out of range of ByteArray (%lu)",
      (unsigned long) index,
      (unsigned long) ba->size);
    return UFALSE;
  }
  *out = NUMBER_VAL(ba->buffer[index]);
  return UTRUE;
}

static TypePattern argsByteArrayGetItem[] = {
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcByteArrayGetItem = { implByteArrayGetItem, "__getitem__",
  sizeof(argsByteArrayGetItem)/sizeof(TypePattern), 0,
  argsByteArrayGetItem };

static ubool implByteArraySetItem(i16 argCount, Value *args, Value *out) {
  Value receiver = args[-1];
  ObjByteArray *ba;
  i32 index;
  u8 value;
  if (!IS_BYTE_ARRAY(receiver)) {
    runtimeError("Expected ByteArray as receiver to ByteArray.__setitem__()");
    return UFALSE;
  }
  ba = AS_BYTE_ARRAY(receiver);
  index = AS_NUMBER(args[0]);
  if (index < 0) {
    index += ba->size;
  }
  if (index < 0 || index >= ba->size) {
    runtimeError("Index %lu out of range of ByteArray (%lu)",
      (unsigned long) index,
      (unsigned long) ba->size);
    return UFALSE;
  }
  value = AS_NUMBER(args[1]);
  ba->buffer[index] = value;
  return UTRUE;
}

static TypePattern argsByteArraySetItem[] = {
  { TYPE_PATTERN_NUMBER },
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcByteArraySetItem = {
  implByteArraySetItem, "__setitem__",
  sizeof(argsByteArraySetItem)/sizeof(TypePattern), 0,
  argsByteArraySetItem };

void initByteArrayClass() {
  ObjString *tmpstr;
  CFunction *methods[] = {
    &funcByteArrayGetItem,
    &funcByteArraySetItem,
  };
  size_t i;
  ObjClass *cls;

  tmpstr = copyCString("ByteArray");
  push(OBJ_VAL(tmpstr));
  cls = vm.byteArrayClass = newClass(tmpstr);
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
