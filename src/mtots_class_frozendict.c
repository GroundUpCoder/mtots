#include "mtots_vm.h"

static ubool implFrozenDictGetItem(i16 argCount, Value *args, Value *out) {
  ObjFrozenDict *dict = AS_FROZEN_DICT(args[-1]);
  if (!mapGet(&dict->map, args[0], out)) {
    runtimeError("Key not found in dict");
    return UFALSE;
  }
  return UTRUE;
}

static CFunction funcFrozenDictGetItem = { implFrozenDictGetItem, "__getitem__", 1 };

static ubool implFrozenDictContains(i16 argCount, Value *args, Value *out) {
  Value dummy;
  ObjFrozenDict *dict = AS_FROZEN_DICT(args[-1]);
  *out = BOOL_VAL(mapGet(&dict->map, args[0], &dummy));
  return UTRUE;
}

static CFunction funcFrozenDictContains = { implFrozenDictContains, "__contains__", 1 };

typedef struct ObjDictIterator {
  ObjNativeClosure obj;
  ObjFrozenDict *dict;
  MapIterator di;
} ObjDictIterator;

static ubool implFrozenDictIterator(
    void *it, i16 argCount, Value *args, Value *out) {
  ObjDictIterator *iter = (ObjDictIterator*)it;
  if (mapIteratorNextKey(&iter->di, out)) {
    return UTRUE;
  }
  *out = STOP_ITERATION_VAL();
  return UTRUE;
}

static void blackenFrozenDictIterator(void *it) {
  ObjDictIterator *li = (ObjDictIterator*)it;
  markObject((Obj*)(li->dict));
}

static ubool implFrozenDictIter(i16 argCount, Value *args, Value *out) {
  ObjFrozenDict *dict = AS_FROZEN_DICT(args[-1]);
  ObjDictIterator *iter;
  iter = NEW_NATIVE_CLOSURE(
    ObjDictIterator,
    implFrozenDictIterator,
    blackenFrozenDictIterator,
    NULL,
    "FrozenDictIterator", 0, 0);
  iter->dict = dict;
  initMapIterator(&iter->di, &dict->map);
  *out = OBJ_VAL_EXPLICIT((Obj*)iter);
  return UTRUE;
}

static CFunction funcFrozenDictIter = { implFrozenDictIter, "__iter__", 0 };

/**
 * "Reverse Get" or inverse lookup
 *
 * Find a key with the given value
 * If more than one entry has the same value, this function
 * will return the first matching key that would've been returned
 * in an iteration of the mapionary
 *
 * Implementation is a slow linear search, but a method like this
 * is still handy sometimes.
 */
static ubool implFrozenDictRget(i16 argCount, Value *args, Value *out) {
  ObjFrozenDict *dict = AS_FROZEN_DICT(args[-1]);
  Value value = args[0];
  MapIterator di;
  MapEntry *entry;

  initMapIterator(&di, &dict->map);
  while (mapIteratorNext(&di, &entry)) {
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
  runtimeError("No entry with given value found in FrozenDict");
  return UFALSE;
}

static CFunction funcFrozenDictRget = { implFrozenDictRget, "rget", 1, 2 };

void initFrozenDictClass() {
  String *tmpstr;
  CFunction *methods[] = {
    &funcFrozenDictGetItem,
    &funcFrozenDictContains,
    &funcFrozenDictIter,
    &funcFrozenDictRget,
  };
  size_t i;
  ObjClass *cls;

  tmpstr = internCString("FrozenDict");
  push(STRING_VAL(tmpstr));
  cls = vm.frozenDictClass = newClass(tmpstr);
  cls->isBuiltinClass = UTRUE;
  pop();

  for (i = 0; i < sizeof(methods) / sizeof(CFunction*); i++) {
    methods[i]->receiverType.type = TYPE_PATTERN_FROZEN_DICT;
    mapSetN(&cls->methods, methods[i]->name, CFUNCTION_VAL(methods[i]));
  }
}
