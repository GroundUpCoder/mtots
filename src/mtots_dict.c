#include "mtots_dict.h"
#include "mtots_memory.h"
#include "mtots_object.h"
#include "mtots_value.h"
#include "mtots_vm.h"

#define DICT_MAX_LOAD 0.75

STATIC_INLINE ubool isNan(double x) {
  return x != x;
}

void initDict(Dict *dict) {
  dict->count = 0;
  dict->capacity = 0;
  dict->size = 0;
  dict->entries = NULL;
}

void freeDict(Dict *dict) {
  FREE_ARRAY(DictEntry, dict->entries, dict->capacity);
  initDict(dict);
}

static u32 hashval(Value value) {
  switch (value.type) {
    /* hash values for bool taken from Java */
    case VAL_BOOL: return AS_BOOL(value) ? 1231 : 1237;
    case VAL_NIL: return 17;
    case VAL_NUMBER: {
      double x = AS_NUMBER(value);
      i32 ix = (i32) x;
      if (x == (double)ix) {
        return (u32) ix;
      }
    }
    case VAL_CFUNCTION: break;
    case VAL_OPERATOR: break;
    case VAL_SENTINEL: return (u32) AS_SENTINEL(value);
    case VAL_OBJ: switch (AS_OBJ(value)->type) {
      case OBJ_STRING: return AS_STRING(value)->hash;
      default: break;
    }
  }
  panic("%s values are not hashable", getKindName(value));
  return 0;
}

/* NOTE: capacity should always be non-zero.
 * If findEntry was a non-static function, I probably would do the check
 * in the function itself. But since it is static, all places where
 * it can be called are within this file.
 */
static DictEntry *findEntry(DictEntry *entries, size_t capacity, Value key) {
  /* OPT: key->hash % capacity */
  u32 index = hashval(key) & (capacity - 1);
  DictEntry *tombstone = NULL;
  for (;;) {
    DictEntry *entry = &entries[index];
    if (IS_EMPTY_KEY(entry->key)) {
      if (IS_NIL(entry->value)) {
        /* Empty entry */
        return tombstone != NULL ? tombstone : entry;
      } else if (tombstone == NULL) {
        /* We found a tombstone */
        tombstone = entry;
      }
    } else if (valuesEqual(entry->key, key)) {
      /* We found the key */
      return entry;
    }
    /* OPT: (index + 1) % capacity */
    index = (index + 1) & (capacity - 1);
  }
}

ubool dictGet(Dict *dict, Value key, Value *value) {
  DictEntry *entry;

  if (dict->count == 0) {
    return UFALSE;
  }

  entry = findEntry(dict->entries, dict->capacity, key);
  if (IS_EMPTY_KEY(entry->key)) {
    return UFALSE;
  }

  *value = entry->value;
  return UTRUE;
}

static void adjustCapacity(Dict *dict, size_t capacity) {
  size_t i;
  DictEntry *entries = ALLOCATE(DictEntry, capacity);
  for (i = 0; i < capacity; i++) {
    entries[i].key = EMPTY_KEY_VAL();
    entries[i].value = NIL_VAL();
  }

  dict->count = 0;
  for (i = 0; i < dict->capacity; i++) {
    DictEntry *entry = &dict->entries[i], *dest;
    if (IS_EMPTY_KEY(entry->key)) continue;

    dest = findEntry(entries, capacity, entry->key);
    dest->key = entry->key;
    dest->value = entry->value;
    dict->count++;
  }

  FREE_ARRAY(DictEntry, dict->entries, dict->capacity);
  dict->entries = entries;
  dict->capacity = capacity;
}

ubool dictSet(Dict *dict, Value key, Value value) {
  DictEntry *entry;
  ubool isNewKey;

  if (dict->count + 1 > dict->capacity * DICT_MAX_LOAD) {
    size_t capacity = GROW_CAPACITY(dict->capacity);
    adjustCapacity(dict, capacity);
  }
  entry = findEntry(dict->entries, dict->capacity, key);
  isNewKey = IS_EMPTY_KEY(entry->key);

  if (isNewKey) {
    /* If entry->value is not nil, we're reusing a tombstone
    * so we don't want to increment the count since tombstones
    * are already included in count.
    * We include tombstones in the count so that the loadfactor
    * is sensitive to slots occupied by tombstones */
    if (IS_NIL(entry->value)) {
      dict->count++;
    }
    dict->size++;
  }
  entry->key = key;
  entry->value = value;
  return isNewKey;
}

ubool dictDelete(Dict *dict, Value key) {
  DictEntry *entry;

  if (dict->count == 0) {
    return UFALSE;
  }

  entry = findEntry(dict->entries, dict->capacity, key);
  if (IS_EMPTY_KEY(entry->key)) {
    return UFALSE;
  }

  /* Place a tombstone in the entry */
  entry->key = EMPTY_KEY_VAL();
  entry->value = BOOL_VAL(1);
  dict->size--;
  return UTRUE;
}

void markDict(Dict *dict) {
  size_t i;
  for (i = 0; i < dict->capacity; i++) {
    DictEntry *entry = &dict->entries[i];
    markValue(entry->key);
    markValue(entry->value);
  }
}
