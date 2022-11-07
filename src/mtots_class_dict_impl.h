#ifndef mtots_class_dict_impl_h
#define mtots_class_dict_impl_h
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

typedef struct ObjDictIterator {
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

/**
 * "Reverse Get" or inverse lookup
 *
 * Find a key with the given value
 * If more than one entry has the same value, this function
 * will return the first matching key that would've been returned
 * in an iteration of the dictionary
 *
 * Implementation is a slow linear search, but a method like this
 * is still handy sometimes.
 */
static ubool implDictRget(i16 argCount, Value *args, Value *out) {
  ObjDict *dict = AS_DICT(args[-1]);
  Value value = args[0];
  DictIterator di;
  DictEntry *entry;

  initDictIterator(&di, &dict->dict);
  while (dictIteratorNext(&di, &entry)) {
    if (valuesEqual(entry->value, value)) {
      *out = entry->key;
      return UTRUE;
    }
  }

  if (argCount > 1) {
    /* If the optional second argument is provided, we return that
     * when a matching entry is not found */
    *out = args[1];
    return UTRUE;
  }
  /* If no entry is found, and no optional argument is provided
   * we throw an error */
  runtimeError("No entry with given value found in Dict");
  return UFALSE;
}

static CFunction funcDictRget = { implDictRget, "rget", 1, 2 };

void initDictClass() {
  ObjString *tmpstr;
  CFunction *methods[] = {
    &funcDictGetItem,
    &funcDictSetItem,
    &funcDictDelete,
    &funcDictContains,
    &funcDictIter,
    &funcDictRget,
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
    dictSetN(&cls->methods, methods[i]->name, CFUNCTION_VAL(methods[i]));
  }
}
#endif/*mtots_class_dict_impl_h*/