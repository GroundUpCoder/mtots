#include "mtots_class_tuple.h"
#include "mtots_vm.h"
#include <stdlib.h>

static ubool implTupleMul(i16 argCount, Value *args, Value *out) {
  Value receiver = args[-1], *buffer;
  ObjTuple *tuple;
  size_t r, rep = AS_U32(args[0]);
  if (!IS_TUPLE(receiver)) {
    runtimeError("Expected tuple as receiver to Tuple.append()");
    return UFALSE;
  }
  tuple = AS_TUPLE(receiver);
  buffer = malloc(sizeof(Value) * tuple->length * rep);
  for (r = 0; r < rep; r++) {
    size_t i;
    for (i = 0; i < tuple->length; i++) {
      buffer[r * tuple->length + i] = tuple->buffer[i];
    }
  }
  *out = TUPLE_VAL(copyTuple(buffer, tuple->length * rep));
  free(buffer);
  return UTRUE;
}

static TypePattern argsTupleMul[] = {
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcTupleMul = { implTupleMul, "__mul__",
  sizeof(argsTupleMul)/sizeof(TypePattern), 0, argsTupleMul };

static ubool implTupleGetItem(i16 argCount, Value *args, Value *out) {
  Value receiver = args[-1];
  ObjTuple *tuple;
  i32 index;
  if (!IS_TUPLE(receiver)) {
    runtimeError("Expected tuple as receiver to Tuple.__getitem__()");
    return UFALSE;
  }
  tuple = AS_TUPLE(receiver);
  if (!IS_NUMBER(args[0])) {
    runtimeError("Expcted Tuple index to be a number");
    return UFALSE;
  }
  index = (i32) AS_NUMBER(args[0]);
  if (index < 0) {
    index += tuple->length;
  }
  if (index < 0 || index >= tuple->length) {
    runtimeError("Tuple index out of bounds");
    return UFALSE;
  }
  *out = tuple->buffer[index];
  return UTRUE;
}

static CFunction funcTupleGetItem = { implTupleGetItem, "__getitem__", 1 };

typedef struct ObjTupleIterator {
  ObjNativeClosure obj;
  ObjTuple *tuple;
  size_t index;
} ObjTupleIterator;

static ubool implTupleIterator(
    void *it, i16 argCount, Value *args, Value *out) {
  ObjTupleIterator *iter = (ObjTupleIterator*)it;
  if (iter->index < iter->tuple->length) {
    *out = iter->tuple->buffer[iter->index++];
  } else {
    *out = STOP_ITERATION_VAL();
  }
  return UTRUE;
}

static void blackenTupleIterator(void *it) {
  ObjTupleIterator *li = (ObjTupleIterator*)it;
  markObject((Obj*)(li->tuple));
}

static ubool implTupleIter(i16 argCount, Value *args, Value *out) {
  Value receiver = args[-1];
  ObjTuple *tuple;
  ObjTupleIterator *iter;
  if (!IS_TUPLE(receiver)) {
    runtimeError("Expected tuple as receiver to Tuple.__iter__()");
    return UFALSE;
  }
  tuple = AS_TUPLE(receiver);
  iter = NEW_NATIVE_CLOSURE(
    ObjTupleIterator,
    implTupleIterator,
    blackenTupleIterator,
    NULL,
    "TupleIterator", 0, 0);
  iter->tuple = tuple;
  iter->index = 0;
  *out = OBJ_VAL_EXPLICIT((Obj*)iter);
  return UTRUE;
}

static CFunction funcTupleIter = { implTupleIter, "__iter__", 0 };

void initTupleClass() {
  String *tmpstr;
  CFunction *methods[] = {
    &funcTupleMul,
    &funcTupleGetItem,
    &funcTupleIter,
  };
  size_t i;
  ObjClass *cls;

  tmpstr = internCString("Tuple");
  push(STRING_VAL(tmpstr));
  cls = vm.tupleClass = newClass(tmpstr);
  cls->isBuiltinClass = UTRUE;
  pop();

  for (i = 0; i < sizeof(methods) / sizeof(CFunction*); i++) {
    methods[i]->receiverType.type = TYPE_PATTERN_TUPLE;
    tmpstr = internCString(methods[i]->name);
    push(STRING_VAL(tmpstr));
    mapSetStr(
      &cls->methods, tmpstr, CFUNCTION_VAL(methods[i]));
    pop();
  }
}
