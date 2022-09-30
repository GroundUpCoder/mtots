#include "mtots_dict.h"
#include "mtots_object.h"
#include "mtots_vm.h"
#include "mtots_value.h"
#include "mtots_memory.h"

static ubool implDictGetItem(i16 argCount, Value *args, Value *out) {
  Value receiver = args[-1];
  ObjDict *dict;
  if (!IS_DICT(receiver)) {
    runtimeError("Expected dict as receiver to Dict.__getitem__()");
    return UFALSE;
  }
  dict = AS_DICT(receiver);
  if (!dictGet(&dict->dict, args[0], out)) {
    runtimeError("Key not found in dict");
    return UFALSE;
  }
  return UTRUE;
}

static CFunction funcDictGetItem = { implDictGetItem, "__getitem__", 1 };

static ubool implDictSetItem(i16 argCount, Value *args, Value *out) {
  Value receiver = args[-1];
  ObjDict *dict;
  if (!IS_DICT(receiver)) {
    runtimeError("Expected dict as receiver to Dict.__setitem__()");
    return UFALSE;
  }
  dict = AS_DICT(receiver);
  *out = BOOL_VAL(dictSet(&dict->dict, args[0], args[1]));
  return UTRUE;
}

static CFunction funcDictSetItem = { implDictSetItem, "__setitem__", 2 };

static ubool implDictDelete(i16 argCount, Value *args, Value *out) {
  Value receiver = args[-1];
  ObjDict *dict;
  if (!IS_DICT(receiver)) {
    runtimeError("Expected dict as receiver to Dict.delete()");
    return UFALSE;
  }
  dict = AS_DICT(receiver);
  *out = BOOL_VAL(dictDelete(&dict->dict, args[0]));
  return UTRUE;
}

static CFunction funcDictRemove = { implDictDelete, "delete", 1 };

static ubool implDictContains(i16 argCount, Value *args, Value *out) {
  Value receiver = args[-1], dummy;
  ObjDict *dict;
  if (!IS_DICT(receiver)) {
    runtimeError("Expected dict as receiver to Dict.__contains__()");
    return UFALSE;
  }
  dict = AS_DICT(receiver);
  *out = BOOL_VAL(dictGet(&dict->dict, args[0], &dummy));
  return UTRUE;
}

static CFunction funcDictContains = { implDictContains, "__contains__", 1 };

typedef struct {
  ObjNativeClosure obj;
  ObjDict *dict;
  size_t index;
} ObjDictIterator;

static ubool implDictIterator(
    void *it, i16 argCount, Value *args, Value *out) {
  ObjDictIterator *iter = (ObjDictIterator*)it;
  while (iter->index < iter->dict->dict.capacity) {
    DictEntry *entry = &iter->dict->dict.entries[iter->index++];
    if (!IS_EMPTY_KEY(entry->key)) {
      *out = entry->key;
      return UTRUE;
    }
  }
  *out = STOP_ITERATION_VAL();
  return UTRUE;
}

static void blackenDictIterator(void *it) {
  ObjDictIterator *li = (ObjDictIterator*)it;
  markObject((Obj*)(li->dict));
}

static ubool implDictIter(i16 argCount, Value *args, Value *out) {
  Value receiver = args[-1];
  ObjDict *dict;
  ObjDictIterator *iter;
  if (!IS_DICT(receiver)) {
    runtimeError("Expected dict as receiver to Dict.__iter__()");
    return UFALSE;
  }
  dict = AS_DICT(receiver);
  iter = NEW_NATIVE_CLOSURE(
    ObjDictIterator,
    implDictIterator,
    blackenDictIterator,
    NULL,
    "DictIterator", 0, 0);
  iter->dict = dict;
  iter->index = 0;
  *out = OBJ_VAL(iter);
  return UTRUE;
}

static CFunction funcDictIter = { implDictIter, "__iter__", 0 };

void initDictClass() {
  ObjString *tmpstr;
  CFunction *methods[] = {
    &funcDictGetItem,
    &funcDictSetItem,
    &funcDictRemove,
    &funcDictContains,
    &funcDictIter,
  };
  size_t i;
  ObjClass *cls;

  tmpstr = copyCString("Dict");
  push(OBJ_VAL(tmpstr));
  cls = vm.dictClass = newClass(tmpstr);
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
