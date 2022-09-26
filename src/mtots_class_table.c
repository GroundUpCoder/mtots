#include "mtots_class_table.h"
#include "mtots_vm.h"

static ubool implTableLen(i16 argCount, Value *args, Value *out) {
  Value receiver = args[-1];
  ObjInstance *table;
  if (!IS_INSTANCE(receiver)) {
    runtimeError(
      "Expected instance as receiver to Table.__len__() but got %s",
      getKindName(receiver));
    return UFALSE;
  }
  table = AS_INSTANCE(receiver);
  *out = NUMBER_VAL(table->fields.size);
  return UTRUE;
}

static CFunction funcTableLen = { implTableLen, "__len__", 0 };

static ubool implTableGetItem(i16 argCount, Value *args, Value *out) {
  Value receiver = args[-1];
  ObjInstance *table;
  ObjString *key;
  if (!IS_INSTANCE(receiver)) {
    runtimeError(
      "Expected instance as receiver to Table.__getitem__() but got %s",
      getKindName(receiver));
    return UFALSE;
  }
  table = AS_INSTANCE(receiver);
  if (!IS_STRING(args[0])) {
    runtimeError(
      "Table.__getitem__() expects a string argument but got %s",
      getKindName(args[0]));
    return UFALSE;
  }
  key = AS_STRING(args[0]);
  if (!tableGet(&table->fields, key, out)) {
    runtimeError("key %s not found in Table", key->chars);
    return UFALSE;
  }
  return UTRUE;
}

static CFunction funcTableGetItem = { implTableGetItem, "__getitem__", 1 };

/* Retrieves all the keys in a table as a list
 * Order is unspcified */
static ubool implTableGetKeys(i16 argCount, Value *args, Value *out) {
  Value receiver = args[-1];
  ObjInstance *table;
  ObjList *list;
  size_t i, j, len;
  if (!IS_INSTANCE(receiver)) {
    runtimeError(
      "Expected instance as receiver to Table.getKeys() but got %s",
      getKindName(receiver));
    return UFALSE;
  }
  table = AS_INSTANCE(receiver);
  len = table->fields.size;
  list = newList(len);
  for (i = j = 0; i < table->fields.capacity; i++) {
    Entry *entry = &table->fields.entries[i];
    if (entry->key != NULL) {
      if (j >= len) {
        panic("ASSERTION FAILED, invalid table size");
      }
      list->buffer[j++] = OBJ_VAL(entry->key);
    }
  }
  if (j != len) {
    panic("ASSERTION FAILED, invalid table size");
  }
  *out = OBJ_VAL(list);
  return UTRUE;
}

static CFunction funcTableGetKeys = { implTableGetKeys, "getKeys", 0 };

/* "Reverse get" or inverse lookup
 *
 * Find a key with the given value
 * If more than one entry has the same value, this function
 * may return any one of those keys
 *
 * Implementation is a slow linear search, but still, a method like
 * this comes handy sometimes */
static ubool implTableRget(i16 argCount, Value *args, Value *out) {
  Value receiver = args[-1], targetValue = args[0];
  ObjInstance *table;
  size_t i;
  if (!IS_INSTANCE(receiver)) {
    runtimeError(
      "Expected instance as receiver to Table.rget() but got %s",
      getKindName(receiver));
    return UFALSE;
  }
  table = AS_INSTANCE(receiver);
  for (i = 0; i < table->fields.capacity; i++) {
    Entry *entry = &table->fields.entries[i];
    if (entry->key != NULL) {
      if (valuesEqual(entry->value, targetValue)) {
        *out = OBJ_VAL(entry->key);
        return UTRUE;
      }
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
  runtimeError("No entry with given value found in Table");
  return UFALSE;
}

static CFunction funcTableRget = { implTableRget, "rget", 1, 2 };

static ubool implTableDelete(i16 argCount, Value *args, Value *out) {
  Value receiver = args[-1];
  ObjString *key;
  ObjInstance *table;
  if (!IS_INSTANCE(receiver)) {
    runtimeError(
      "Expected instance as receiver to Table.delete() but got %s",
      getKindName(receiver));
    return UFALSE;
  }
  table = AS_INSTANCE(receiver);
  if (!IS_STRING(args[0])) {
    runtimeError(
      "Table.delete() expects string but got %s",
      getKindName(args[0]));
    return UFALSE;
  }
  key = AS_STRING(args[0]);
  *out = BOOL_VAL(tableDelete(&table->fields, key));
  return UTRUE;
}

static CFunction funcTableDelete = { implTableDelete, "delete", 1 };

void initTableClass() {
  CFunction *methods[] = {
    &funcTableLen,
    &funcTableGetItem,
    &funcTableGetKeys,
    &funcTableRget,
    &funcTableDelete,
  };
  size_t i;
  ObjClass *cls;

  cls = vm.tableClass = newClassFromCString("Table");

  /* Technically Table is a builtin class, but instances of Table
   * are actually just normal ObjInstances */
  cls->isBuiltinClass = UFALSE;

  for (i = 0; i < sizeof(methods) / sizeof(CFunction*); i++) {
    tableSetN(&cls->methods, methods[i]->name, CFUNCTION_VAL(methods[i]));
  }
}
