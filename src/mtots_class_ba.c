#include "mtots_class_ba.h"
#include "mtots_vm.h"

#include <string.h>

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

static ubool implByteArrayI8(i16 argCount, Value *args, Value *out) {
  ObjByteArray *ba = AS_BYTE_ARRAY(args[-1]);
  size_t index = (size_t) AS_NUMBER(args[0]);
  if (index >= ba->size) {
    runtimeError("ByteArray overflow");
    return UFALSE;
  }
  if (argCount > 1) {
    i8 value = (i8) AS_NUMBER(args[1]);
    memcpy(ba->buffer + index, (void*) &value, 1);
    return UTRUE;
  } else {
    i8 value;
    memcpy((void*) &value, ba->buffer + index, 1);
    *out = NUMBER_VAL((double)value);
    return UTRUE;
  }
}

static TypePattern argsByteArrayI8[] = {
  { TYPE_PATTERN_NUMBER },
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcByteArrayI8 = {
  implByteArrayI8, "i8", 1, 2, argsByteArrayI8,
};

static ubool implByteArrayU8(i16 argCount, Value *args, Value *out) {
  ObjByteArray *ba = AS_BYTE_ARRAY(args[-1]);
  size_t index = (size_t) AS_NUMBER(args[0]);
  if (index >= ba->size) {
    runtimeError("ByteArray overflow");
    return UFALSE;
  }
  if (argCount > 1) {
    u8 value = (u8) AS_NUMBER(args[1]);
    memcpy(ba->buffer + index, (void*) &value, 1);
    return UTRUE;
  } else {
    u8 value;
    memcpy((void*) &value, ba->buffer + index, 1);
    *out = NUMBER_VAL((double)value);
    return UTRUE;
  }
}

static TypePattern argsByteArrayU8[] = {
  { TYPE_PATTERN_NUMBER },
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcByteArrayU8 = {
  implByteArrayU8, "u8", 1, 2, argsByteArrayU8,
};

static ubool implByteArrayI16(i16 argCount, Value *args, Value *out) {
  ObjByteArray *ba = AS_BYTE_ARRAY(args[-1]);
  size_t index = (size_t) AS_NUMBER(args[0]);
  if (index >= ba->size || index + 2 > ba->size) {
    runtimeError("ByteArray overflow");
    return UFALSE;
  }
  if (argCount > 1) {
    i16 value = (i16) AS_NUMBER(args[1]);
    memcpy(ba->buffer + index, (void*) &value, 2);
    return UTRUE;
  } else {
    i16 value;
    memcpy((void*) &value, ba->buffer + index, 2);
    *out = NUMBER_VAL((double)value);
    return UTRUE;
  }
}

static TypePattern argsByteArrayI16[] = {
  { TYPE_PATTERN_NUMBER },
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcByteArrayI16 = {
  implByteArrayI16, "i16", 1, 2, argsByteArrayI16,
};

static ubool implByteArrayU16(i16 argCount, Value *args, Value *out) {
  ObjByteArray *ba = AS_BYTE_ARRAY(args[-1]);
  size_t index = (size_t) AS_NUMBER(args[0]);
  if (index >= ba->size || index + 2 > ba->size) {
    runtimeError("ByteArray overflow");
    return UFALSE;
  }
  if (argCount > 1) {
    u16 value = (u16) AS_U32(args[1]);
    memcpy(ba->buffer + index, (void*) &value, 2);
    return UTRUE;
  } else {
    u16 value;
    memcpy((void*) &value, ba->buffer + index, 2);
    *out = NUMBER_VAL((double)value);
    return UTRUE;
  }
}

static TypePattern argsByteArrayU16[] = {
  { TYPE_PATTERN_NUMBER },
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcByteArrayU16 = {
  implByteArrayU16, "u16", 1, 2, argsByteArrayU16,
};

static ubool implByteArrayI32(i16 argCount, Value *args, Value *out) {
  ObjByteArray *ba = AS_BYTE_ARRAY(args[-1]);
  size_t index = (size_t) AS_NUMBER(args[0]);
  if (index >= ba->size || index + 4 > ba->size) {
    runtimeError("ByteArray overflow");
    return UFALSE;
  }
  if (argCount > 1) {
    i32 value = AS_I32(args[1]);
    memcpy(ba->buffer + index, (void*) &value, 4);
    return UTRUE;
  } else {
    i32 value;
    memcpy((void*) &value, ba->buffer + index, 4);
    *out = NUMBER_VAL((double)value);
    return UTRUE;
  }
}

static TypePattern argsByteArrayI32[] = {
  { TYPE_PATTERN_NUMBER },
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcByteArrayI32 = {
  implByteArrayI32, "i32", 1, 2, argsByteArrayI32,
};

static ubool implByteArrayU32(i16 argCount, Value *args, Value *out) {
  ObjByteArray *ba = AS_BYTE_ARRAY(args[-1]);
  size_t index = (size_t) AS_NUMBER(args[0]);
  if (index >= ba->size || index + 4 > ba->size) {
    runtimeError("ByteArray overflow");
    return UFALSE;
  }
  if (argCount > 1) {
    u32 value = AS_U32(args[1]);
    memcpy(ba->buffer + index, (void*) &value, 4);
    return UTRUE;
  } else {
    u32 value;
    memcpy((void*) &value, ba->buffer + index, 4);
    *out = NUMBER_VAL((double)value);
    return UTRUE;
  }
}

static TypePattern argsByteArrayU32[] = {
  { TYPE_PATTERN_NUMBER },
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcByteArrayU32 = {
  implByteArrayU32, "u32", 1, 2, argsByteArrayU32,
};

static ubool implByteArrayF32(i16 argCount, Value *args, Value *out) {
  ObjByteArray *ba = AS_BYTE_ARRAY(args[-1]);
  size_t index = (size_t) AS_NUMBER(args[0]);
  if (index >= ba->size || index + 4 > ba->size) {
    runtimeError("ByteArray overflow");
    return UFALSE;
  }
  if (argCount > 1) {
    float value = (float) AS_NUMBER(args[1]);
    memcpy(ba->buffer + index, (void*) &value, 4);
    return UTRUE;
  } else {
    float value;
    memcpy((void*) &value, ba->buffer + index, 4);
    *out = NUMBER_VAL((double)value);
    return UTRUE;
  }
}

static TypePattern argsByteArrayF32[] = {
  { TYPE_PATTERN_NUMBER },
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcByteArrayF32 = {
  implByteArrayF32, "f32", 1, 2, argsByteArrayF32,
};

static ubool implByteArrayF64(i16 argCount, Value *args, Value *out) {
  ObjByteArray *ba = AS_BYTE_ARRAY(args[-1]);
  size_t index = (size_t) AS_NUMBER(args[0]);
  if (index >= ba->size || index + 4 > ba->size) {
    runtimeError("ByteArray overflow");
    return UFALSE;
  }
  if (argCount > 1) {
    double value = AS_NUMBER(args[1]);
    memcpy(ba->buffer + index, (void*) &value, 4);
    return UTRUE;
  } else {
    double value;
    memcpy((void*) &value, ba->buffer + index, 4);
    *out = NUMBER_VAL((double)value);
    return UTRUE;
  }
}

static TypePattern argsByteArrayF64[] = {
  { TYPE_PATTERN_NUMBER },
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcByteArrayF64 = {
  implByteArrayF64, "f64", 1, 2, argsByteArrayF64,
};

void initByteArrayClass() {
  ObjString *tmpstr;
  CFunction *methods[] = {
    &funcByteArrayGetItem,
    &funcByteArraySetItem,
    &funcByteArrayF32,
    &funcByteArrayF64,
    &funcByteArrayI8,
    &funcByteArrayU8,
    &funcByteArrayI16,
    &funcByteArrayU16,
    &funcByteArrayI32,
    &funcByteArrayU32,
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
    methods[i]->receiverType.type = TYPE_PATTERN_BYTE_ARRAY;
    push(OBJ_VAL(tmpstr));
    tableSet(
      &cls->methods, tmpstr, CFUNCTION_VAL(methods[i]));
    pop();
  }
}
