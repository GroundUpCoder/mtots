#include "mtots_dict.h"
#include "mtots_object.h"
#include "mtots_vm.h"
#include "mtots_value.h"
#include "mtots_memory.h"

static ubool implDictGetItem(i16 argCount, Value *args, Value *out) {
  ObjDict *dict = AS_DICT(args[-1]);
  if (!dictGet(&dict->dict, args[0], out)) {
    runtimeError("Key not found in dict");
    return UFALSE;
  }
  return UTRUE;
}

static CFunction funcDictGetItem = { implDictGetItem, "__getitem__", 1 };

static ubool implDictSetItem(i16 argCount, Value *args, Value *out) {
  ObjDict *dict = AS_DICT(args[-1]);
  *out = BOOL_VAL(dictSet(&dict->dict, args[0], args[1]));
  return UTRUE;
}

static CFunction funcDictSetItem = { implDictSetItem, "__setitem__", 2 };

static ubool implDictDelete(i16 argCount, Value *args, Value *out) {
  ObjDict *dict = AS_DICT(args[-1]);
  *out = BOOL_VAL(dictDelete(&dict->dict, args[0]));
  return UTRUE;
}

static CFunction funcDictDelete = { implDictDelete, "delete", 1 };

static ubool implDictContains(i16 argCount, Value *args, Value *out) {
  Value dummy;
  ObjDict *dict = AS_DICT(args[-1]);
  *out = BOOL_VAL(dictGet(&dict->dict, args[0], &dummy));
  return UTRUE;
}

static CFunction funcDictContains = { implDictContains, "__contains__", 1 };

typedef struct {
  ObjNativeClosure obj;
  ObjDict *dict;
  DictIterator di;
} ObjDictIterator;

static ubool implDictIterator(
    void *it, i16 argCount, Value *args, Value *out) {
  ObjDictIterator *iter = (ObjDictIterator*)it;
  if (dictIteratorNextKey(&iter->di, out)) {
    return UTRUE;
  }
  *out = STOP_ITERATION_VAL();
  return UTRUE;
}

static void blackenDictIterator(void *it) {
  ObjDictIterator *li = (ObjDictIterator*)it;
  markObject((Obj*)(li->dict));
}

static ubool implDictIter(i16 argCount, Value *args, Value *out) {
  ObjDict *dict = AS_DICT(args[-1]);
  ObjDictIterator *iter;
  iter = NEW_NATIVE_CLOSURE(
    ObjDictIterator,
    implDictIterator,
    blackenDictIterator,
    NULL,
    "DictIterator", 0, 0);
  iter->dict = dict;
  initDictIterator(&iter->di, &dict->dict);
  *out = OBJ_VAL(iter);
  return UTRUE;
}

static CFunction funcDictIter = { implDictIter, "__iter__", 0 };

void initDictClass() {
  ObjString *tmpstr;
  CFunction *methods[] = {
    &funcDictGetItem,
    &funcDictSetItem,
    &funcDictDelete,
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
    methods[i]->receiverType.type = TYPE_PATTERN_DICT;
    tableSetN(&cls->methods, methods[i]->name, CFUNCTION_VAL(methods[i]));
  }
}
