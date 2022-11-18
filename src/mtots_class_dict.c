#include "mtots_vm.h"

static ubool implDictGetItem(i16 argCount, Value *args, Value *out) {
  ObjDict *dict = AS_DICT(args[-1]);
  if (!mapGet(&dict->dict, args[0], out)) {
    runtimeError("Key not found in dict");
    return UFALSE;
  }
  return UTRUE;
}

static CFunction funcDictGetItem = { implDictGetItem, "__getitem__", 1 };

static ubool implDictSetItem(i16 argCount, Value *args, Value *out) {
  ObjDict *dict = AS_DICT(args[-1]);
  *out = BOOL_VAL(mapSet(&dict->dict, args[0], args[1]));
  return UTRUE;
}

static CFunction funcDictSetItem = { implDictSetItem, "__setitem__", 2 };

static ubool implDictDelete(i16 argCount, Value *args, Value *out) {
  ObjDict *dict = AS_DICT(args[-1]);
  *out = BOOL_VAL(mapDelete(&dict->dict, args[0]));
  return UTRUE;
}

static CFunction funcDictDelete = { implDictDelete, "delete", 1 };

static ubool implDictContains(i16 argCount, Value *args, Value *out) {
  Value dummy;
  ObjDict *dict = AS_DICT(args[-1]);
  *out = BOOL_VAL(mapGet(&dict->dict, args[0], &dummy));
  return UTRUE;
}

static CFunction funcDictContains = { implDictContains, "__contains__", 1 };

typedef struct ObjDictIterator {
  ObjNativeClosure obj;
  ObjDict *dict;
  MapIterator di;
} ObjDictIterator;

static ubool implDictIterator(
    void *it, i16 argCount, Value *args, Value *out) {
  ObjDictIterator *iter = (ObjDictIterator*)it;
  if (mapIteratorNextKey(&iter->di, out)) {
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
    "MapIterator", 0, 0);
  iter->dict = dict;
  initMapIterator(&iter->di, &dict->dict);
  *out = OBJ_VAL_EXPLICIT((Obj*)iter);
  return UTRUE;
}

static CFunction funcDictIter = { implDictIter, "__iter__", 0 };

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
static ubool implDictRget(i16 argCount, Value *args, Value *out) {
  ObjDict *dict = AS_DICT(args[-1]);
  Value value = args[0];
  MapIterator di;
  MapEntry *entry;

  initMapIterator(&di, &dict->dict);
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
  runtimeError("No entry with given value found in Map");
  return UFALSE;
}

static CFunction funcDictRget = { implDictRget, "rget", 1, 2 };

void initDictClass() {
  String *tmpstr;
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

  tmpstr = internCString("Map");
  push(STRING_VAL(tmpstr));
  cls = vm.mapClass = newClass(tmpstr);
  cls->isBuiltinClass = UTRUE;
  pop();

  for (i = 0; i < sizeof(methods) / sizeof(CFunction*); i++) {
    methods[i]->receiverType.type = TYPE_PATTERN_DICT;
    mapSetN(&cls->methods, methods[i]->name, CFUNCTION_VAL(methods[i]));
  }
}
