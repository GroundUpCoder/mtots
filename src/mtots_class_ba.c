#include "mtots_class_ba.h"
#include "mtots_vm.h"

#include <string.h>

static ubool implByteArrayRaw(i16 argCount, Value *args, Value *out) {
  ObjByteArray *ba = AS_BYTE_ARRAY(args[-1]);
  *out = STRING_VAL(internString((char*)ba->buffer, ba->length));
  return UTRUE;
}

static CFunction funcByteArrayRaw = { implByteArrayRaw, "raw" };

static ubool implByteArrayView(i16 argCount, Value *args, Value *out) {
  ObjByteArray *ba = AS_BYTE_ARRAY(args[-1]);
  double low = 0, high = ba->length;
  size_t ilow, ihigh;
  if (argCount > 0) {
    low = AS_NUMBER(args[0]);
    if (low < 0) {
      low += ba->length;
    }
    if (low < 0 || low >= ba->length) {
      runtimeError("Index %f (low) out of range of ByteArray (%lu)",
        low, (unsigned long) ba->length);
      return UFALSE;
    }
  }
  if (argCount > 1) {
    high = AS_NUMBER(args[1]);
  }
  if (high < 0) {
    high += ba->length;
  }
  if (high < 0 || high > ba->length) {
    runtimeError("Index %f (high) out of range of ByteArray (%lu)",
      high, (unsigned long) ba->length);
    return UFALSE;
  }
  ilow = (size_t)low;
  ihigh = (size_t)high;
  if (ihigh < ilow) {
    ihigh = ilow;
  }
  *out = BYTE_ARRAY_VIEW_VAL(newByteArrayView(ihigh - ilow, ba->buffer + ilow, ba));
  return UTRUE;
}

static TypePattern argsByteArrayView[] = {
  { TYPE_PATTERN_NUMBER },
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcByteArrayView = {
  implByteArrayView, "view", 0, 2,
  argsByteArrayView,
};

static ubool implByteArrayGetItem(i16 argCount, Value *args, Value *out) {
  ObjByteArray *ba = AS_BYTE_ARRAY(args[-1]);
  i32 index;
  index = AS_NUMBER(args[0]);
  if (index < 0) {
    index += ba->length;
  }
  if (index < 0 || index >= ba->length) {
    runtimeError("Index %lu out of range of ByteArray (%lu)",
      (unsigned long) index,
      (unsigned long) ba->length);
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
  ObjByteArray *ba = AS_BYTE_ARRAY(args[-1]);
  i32 index;
  u8 value;
  index = AS_NUMBER(args[0]);
  if (index < 0) {
    index += ba->length;
  }
  if (index < 0 || index >= ba->length) {
    runtimeError("Index %lu out of range of ByteArray (%lu)",
      (unsigned long) index,
      (unsigned long) ba->length);
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
  if (index >= ba->length) {
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
  if (index >= ba->length) {
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
  if (index >= ba->length || index + 2 > ba->length) {
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
  if (index >= ba->length || index + 2 > ba->length) {
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
  if (index >= ba->length || index + 4 > ba->length) {
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
  if (index >= ba->length || index + 4 > ba->length) {
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
  if (index >= ba->length || index + 4 > ba->length) {
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
  if (index >= ba->length || index + 4 > ba->length) {
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

static CFunction *methods[] = {
  &funcByteArrayRaw,
  &funcByteArrayView,
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

void initByteArrayClass() {
  String *tmpstr;
  size_t i;
  ObjClass *cls;

  tmpstr = internCString("ByteArray");
  push(STRING_VAL(tmpstr));
  cls = vm.byteArrayClass = newClass(tmpstr);
  cls->isBuiltinClass = UTRUE;
  pop();

  for (i = 0; i < sizeof(methods) / sizeof(CFunction*); i++) {
    tmpstr = internCString(methods[i]->name);
    methods[i]->receiverType.type = TYPE_PATTERN_BYTE_ARRAY_OR_VIEW;
    push(STRING_VAL(tmpstr));
    mapSetStr(
      &cls->methods, tmpstr, CFUNCTION_VAL(methods[i]));
    pop();
  }
}

void initByteArrayViewClass() {
  String *tmpstr;
  size_t i;
  ObjClass *cls;

  tmpstr = internCString("ByteArrayView");
  push(STRING_VAL(tmpstr));
  cls = vm.byteArrayViewClass = newClass(tmpstr);
  cls->isBuiltinClass = UTRUE;
  pop();

  for (i = 0; i < sizeof(methods) / sizeof(CFunction*); i++) {
    tmpstr = internCString(methods[i]->name);
    methods[i]->receiverType.type = TYPE_PATTERN_BYTE_ARRAY_OR_VIEW;
    push(STRING_VAL(tmpstr));
    mapSetStr(
      &cls->methods, tmpstr, CFUNCTION_VAL(methods[i]));
    pop();
  }
}
