#include "mtots_class_buffer.h"
#include "mtots_vm.h"

#include <string.h>
#include <stdlib.h>


static ubool implBufferLock(i16 argCount, Value *args, Value *out) {
  ObjBuffer *bo = AS_BUFFER(args[-1]);
  bufferLock(&bo->buffer);
  return UTRUE;
}

static CFunction funcBufferLock = { implBufferLock, "lock", 0 };

static ubool implBufferIsLocked(i16 argCount, Value *args, Value *out) {
  ObjBuffer *bo = AS_BUFFER(args[-1]);
  *out = BOOL_VAL(bo->buffer.isLocked);
  return UTRUE;
}

static CFunction funcBufferIsLocked = { implBufferIsLocked, "isLocked", 0 };

static ubool implBufferAddI8(i16 argCount, Value *args, Value *out) {
  ObjBuffer *bo = AS_BUFFER(args[-1]);
  bufferAddI8(&bo->buffer, AS_NUMBER(args[0]));
  return UTRUE;
}

static TypePattern argsBufferAddI8[] = {
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcBufferAddI8 = {
  implBufferAddI8, "addI8", 1, 0, argsBufferAddI8,
};

static ubool implBufferAddU8(i16 argCount, Value *args, Value *out) {
  ObjBuffer *bo = AS_BUFFER(args[-1]);
  bufferAddU8(&bo->buffer, AS_NUMBER(args[0]));
  return UTRUE;
}

static TypePattern argsBufferAddU8[] = {
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcBufferAddU8 = {
  implBufferAddU8, "addU8", 1, 0, argsBufferAddU8,
};

static ubool implBufferAddI16(i16 argCount, Value *args, Value *out) {
  ObjBuffer *bo = AS_BUFFER(args[-1]);
  bufferAddI16(&bo->buffer, AS_NUMBER(args[0]));
  return UTRUE;
}

static TypePattern argsBufferAddI16[] = {
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcBufferAddI16 = {
  implBufferAddI16, "addI16", 1, 0, argsBufferAddI16,
};

static ubool implBufferAddU16(i16 argCount, Value *args, Value *out) {
  ObjBuffer *bo = AS_BUFFER(args[-1]);
  bufferAddU16(&bo->buffer, AS_NUMBER(args[0]));
  return UTRUE;
}

static TypePattern argsBufferAddU16[] = {
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcBufferAddU16 = {
  implBufferAddU16, "addU16", 1, 0, argsBufferAddU16,
};

static ubool implBufferAddI32(i16 argCount, Value *args, Value *out) {
  ObjBuffer *bo = AS_BUFFER(args[-1]);
  bufferAddI32(&bo->buffer, AS_NUMBER(args[0]));
  return UTRUE;
}

static TypePattern argsBufferAddI32[] = {
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcBufferAddI32 = {
  implBufferAddI32, "addI32", 1, 0, argsBufferAddI32,
};

static ubool implBufferAddU32(i16 argCount, Value *args, Value *out) {
  ObjBuffer *bo = AS_BUFFER(args[-1]);
  bufferAddU32(&bo->buffer, AS_NUMBER(args[0]));
  return UTRUE;
}

static TypePattern argsBufferAddU32[] = {
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcBufferAddU32 = {
  implBufferAddU32, "addU32", 1, 0, argsBufferAddU32,
};

static ubool implBufferAddF32(i16 argCount, Value *args, Value *out) {
  ObjBuffer *bo = AS_BUFFER(args[-1]);
  bufferAddF32(&bo->buffer, AS_NUMBER(args[0]));
  return UTRUE;
}

static TypePattern argsBufferAddF32[] = {
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcBufferAddF32 = {
  implBufferAddF32, "addF32", 1, 0, argsBufferAddF32,
};

static ubool implBufferAddF64(i16 argCount, Value *args, Value *out) {
  ObjBuffer *bo = AS_BUFFER(args[-1]);
  bufferAddF64(&bo->buffer, AS_NUMBER(args[0]));
  return UTRUE;
}

static TypePattern argsBufferAddF64[] = {
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcBufferAddF64 = {
  implBufferAddF64, "addF64", 1, 0, argsBufferAddF64,
};

static ubool implBufferGetI8(i16 argCount, Value *args, Value *out) {
  ObjBuffer *bo = AS_BUFFER(args[-1]);
  *out = NUMBER_VAL(bufferGetI8(&bo->buffer, AS_NUMBER(args[0])));
  return UTRUE;
}

static TypePattern argsBufferGetI8[] = {
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcBufferGetI8 = {
  implBufferGetI8, "getI8", 1, 0, argsBufferGetI8,
};

static ubool implBufferGetU8(i16 argCount, Value *args, Value *out) {
  ObjBuffer *bo = AS_BUFFER(args[-1]);
  *out = NUMBER_VAL(bufferGetU8(&bo->buffer, AS_NUMBER(args[0])));
  return UTRUE;
}

static TypePattern argsBufferGetU8[] = {
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcBufferGetU8 = {
  implBufferGetU8, "getU8", 1, 0, argsBufferGetU8,
};

static ubool implBufferGetI16(i16 argCount, Value *args, Value *out) {
  ObjBuffer *bo = AS_BUFFER(args[-1]);
  *out = NUMBER_VAL(bufferGetI16(&bo->buffer, AS_NUMBER(args[0])));
  return UTRUE;
}

static TypePattern argsBufferGetI16[] = {
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcBufferGetI16 = {
  implBufferGetI16, "getI16", 1, 0, argsBufferGetI16,
};

static ubool implBufferGetU16(i16 argCount, Value *args, Value *out) {
  ObjBuffer *bo = AS_BUFFER(args[-1]);
  *out = NUMBER_VAL(bufferGetU16(&bo->buffer, AS_NUMBER(args[0])));
  return UTRUE;
}

static TypePattern argsBufferGetU16[] = {
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcBufferGetU16 = {
  implBufferGetU16, "getU16", 1, 0, argsBufferGetU16,
};

static ubool implBufferGetI32(i16 argCount, Value *args, Value *out) {
  ObjBuffer *bo = AS_BUFFER(args[-1]);
  *out = NUMBER_VAL(bufferGetI32(&bo->buffer, AS_NUMBER(args[0])));
  return UTRUE;
}

static TypePattern argsBufferGetI32[] = {
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcBufferGetI32 = {
  implBufferGetI32, "getI32", 1, 0, argsBufferGetI32,
};

static ubool implBufferGetU32(i16 argCount, Value *args, Value *out) {
  ObjBuffer *bo = AS_BUFFER(args[-1]);
  *out = NUMBER_VAL(bufferGetU32(&bo->buffer, AS_NUMBER(args[0])));
  return UTRUE;
}

static TypePattern argsBufferGetU32[] = {
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcBufferGetU32 = {
  implBufferGetU32, "getU32", 1, 0, argsBufferGetU32,
};

static ubool implBufferGetF32(i16 argCount, Value *args, Value *out) {
  ObjBuffer *bo = AS_BUFFER(args[-1]);
  *out = NUMBER_VAL(bufferGetF32(&bo->buffer, AS_NUMBER(args[0])));
  return UTRUE;
}

static TypePattern argsBufferGetF32[] = {
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcBufferGetF32 = {
  implBufferGetF32, "getF32", 1, 0, argsBufferGetF32,
};

static ubool implBufferGetF64(i16 argCount, Value *args, Value *out) {
  ObjBuffer *bo = AS_BUFFER(args[-1]);
  *out = NUMBER_VAL(bufferGetF64(&bo->buffer, AS_NUMBER(args[0])));
  return UTRUE;
}

static TypePattern argsBufferGetF64[] = {
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcBufferGetF64 = {
  implBufferGetF64, "getF64", 1, 0, argsBufferGetF64,
};

static ubool implBufferSetI8(i16 argCount, Value *args, Value *out) {
  ObjBuffer *bo = AS_BUFFER(args[-1]);
  bufferSetI8(&bo->buffer, AS_NUMBER(args[0]), AS_NUMBER(args[1]));
  return UTRUE;
}

static TypePattern argsBufferSetI8[] = {
  { TYPE_PATTERN_NUMBER },
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcBufferSetI8 = {
  implBufferSetI8, "setI8", 2, 0, argsBufferSetI8,
};

static ubool implBufferSetU8(i16 argCount, Value *args, Value *out) {
  ObjBuffer *bo = AS_BUFFER(args[-1]);
  bufferSetU8(&bo->buffer, AS_NUMBER(args[0]), AS_NUMBER(args[1]));
  return UTRUE;
}

static TypePattern argsBufferSetU8[] = {
  { TYPE_PATTERN_NUMBER },
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcBufferSetU8 = {
  implBufferSetU8, "setU8", 2, 0, argsBufferSetU8,
};

static ubool implBufferSetI16(i16 argCount, Value *args, Value *out) {
  ObjBuffer *bo = AS_BUFFER(args[-1]);
  bufferSetI16(&bo->buffer, AS_NUMBER(args[0]), AS_NUMBER(args[1]));
  return UTRUE;
}

static TypePattern argsBufferSetI16[] = {
  { TYPE_PATTERN_NUMBER },
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcBufferSetI16 = {
  implBufferSetI16, "setI16", 2, 0, argsBufferSetI16,
};

static ubool implBufferSetU16(i16 argCount, Value *args, Value *out) {
  ObjBuffer *bo = AS_BUFFER(args[-1]);
  bufferSetU16(&bo->buffer, AS_NUMBER(args[0]), AS_NUMBER(args[1]));
  return UTRUE;
}

static TypePattern argsBufferSetU16[] = {
  { TYPE_PATTERN_NUMBER },
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcBufferSetU16 = {
  implBufferSetU16, "setU16", 2, 0, argsBufferSetU16,
};

static ubool implBufferSetI32(i16 argCount, Value *args, Value *out) {
  ObjBuffer *bo = AS_BUFFER(args[-1]);
  bufferSetI32(&bo->buffer, AS_NUMBER(args[0]), AS_NUMBER(args[1]));
  return UTRUE;
}

static TypePattern argsBufferSetI32[] = {
  { TYPE_PATTERN_NUMBER },
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcBufferSetI32 = {
  implBufferSetI32, "setI32", 2, 0, argsBufferSetI32,
};

static ubool implBufferSetU32(i16 argCount, Value *args, Value *out) {
  ObjBuffer *bo = AS_BUFFER(args[-1]);
  bufferSetU32(&bo->buffer, AS_NUMBER(args[0]), AS_NUMBER(args[1]));
  return UTRUE;
}

static TypePattern argsBufferSetU32[] = {
  { TYPE_PATTERN_NUMBER },
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcBufferSetU32 = {
  implBufferSetU32, "setU32", 2, 0, argsBufferSetU32,
};

static ubool implBufferSetF32(i16 argCount, Value *args, Value *out) {
  ObjBuffer *bo = AS_BUFFER(args[-1]);
  bufferSetF32(&bo->buffer, AS_NUMBER(args[0]), AS_NUMBER(args[1]));
  return UTRUE;
}

static TypePattern argsBufferSetF32[] = {
  { TYPE_PATTERN_NUMBER },
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcBufferSetF32 = {
  implBufferSetF32, "setF32", 2, 0, argsBufferSetF32,
};

static ubool implBufferSetF64(i16 argCount, Value *args, Value *out) {
  ObjBuffer *bo = AS_BUFFER(args[-1]);
  bufferSetF64(&bo->buffer, AS_NUMBER(args[0]), AS_NUMBER(args[1]));
  return UTRUE;
}

static TypePattern argsBufferSetF64[] = {
  { TYPE_PATTERN_NUMBER },
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcBufferSetF64 = {
  implBufferSetF64, "setF64", 2, 0, argsBufferSetF64,
};

void initBufferClass() {
  CFunction *methods[] = {
    &funcBufferLock,
    &funcBufferIsLocked,
    &funcBufferAddI8,
    &funcBufferAddU8,
    &funcBufferAddI16,
    &funcBufferAddU16,
    &funcBufferAddI32,
    &funcBufferAddU32,
    &funcBufferAddF32,
    &funcBufferAddF64,
    &funcBufferGetI8,
    &funcBufferGetU8,
    &funcBufferGetI16,
    &funcBufferGetU16,
    &funcBufferGetI32,
    &funcBufferGetU32,
    &funcBufferGetF32,
    &funcBufferGetF64,
    &funcBufferSetI8,
    &funcBufferSetU8,
    &funcBufferSetI16,
    &funcBufferSetU16,
    &funcBufferSetI32,
    &funcBufferSetU32,
    &funcBufferSetF32,
    &funcBufferSetF64,
  };
  size_t i;
  ObjClass *cls;

  cls = vm.bufferClass = newClassFromCString("Buffer");
  cls->isBuiltinClass = UTRUE;

  for (i = 0; i < sizeof(methods) / sizeof(CFunction*); i++) {
    methods[i]->receiverType.type = TYPE_PATTERN_BUFFER;
    mapSetN(&cls->methods, methods[i]->name, CFUNCTION_VAL(methods[i]));
  }
}
