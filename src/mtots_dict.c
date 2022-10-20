#include "mtots_dict.h"
#include "mtots_memory.h"
#include "mtots_object.h"
#include "mtots_value.h"
#include "mtots_vm.h"

#define DICT_MAX_LOAD 0.75

void initDict(Dict *dict) {
  dict->count = 0;
  dict->capacity = 0;
  dict->size = 0;
  dict->entries = NULL;
  dict->first = NULL;
  dict->last = NULL;
}

void freeDict(Dict *dict) {
  FREE_ARRAY(DictEntry, dict->entries, dict->capacity);
  initDict(dict);
}

u32 hashval(Value value) {
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
      case OBJ_TUPLE: return AS_TUPLE(value)->hash;
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
  DictEntry *p, *first = NULL, *last = NULL;
  for (i = 0; i < capacity; i++) {
    entries[i].key = EMPTY_KEY_VAL();
    entries[i].value = NIL_VAL();
  }

  dict->count = 0;
  for (p = dict->first; p; p = p->next) {
    DictEntry *dest;

    dest = findEntry(entries, capacity, p->key);
    dest->key = p->key;
    dest->value = p->value;

    if (first == NULL) {
      dest->prev = NULL;
      dest->next = NULL;
      first = last = dest;
    } else {
      dest->prev = last;
      dest->next = NULL;
      last->next = dest;
      last = dest;
    }

    dict->count++;
  }

  FREE_ARRAY(DictEntry, dict->entries, dict->capacity);
  dict->entries = entries;
  dict->capacity = capacity;
  dict->first = first;
  dict->last = last;
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

    if (dict->last == NULL) {
      dict->first = dict->last = entry;
      entry->prev = entry->next = NULL;
    } else {
      entry->prev = dict->last;
      entry->next = NULL;
      dict->last->next = entry;
      dict->last = entry;
    }
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

  /* Update linked list */
  if (entry->prev == NULL) {
    dict->first = entry->next;
  } else {
    entry->prev->next = entry->next;
  }
  if (entry->next == NULL) {
    dict->last = entry->prev;
  } else {
    entry->next->prev = entry->prev;
  }
  entry->prev = entry->next = NULL;

  /* Place a tombstone in the entry */
  entry->key = EMPTY_KEY_VAL();
  entry->value = BOOL_VAL(1);
  dict->size--;
  return UTRUE;
}

ObjTuple *dictFindTuple(
    Dict *dict,
    Value *buffer,
    size_t length,
    u32 hash) {
  u32 index;
  if (dict->count == 0) {
    return NULL;
  }
  /* OPT: hash % dict->capacity */
  index = hash & (dict->capacity - 1);
  for (;;) {
    DictEntry *entry = &dict->entries[index];
    if (IS_EMPTY_KEY(entry->key)) {
      /* Stop if we find an empty non-tombstone entry */
      if (IS_NIL(entry->value)) {
        return NULL;
      }
    } else if (IS_TUPLE(entry->key)) {
      ObjTuple *key = AS_TUPLE(entry->key);
      if (key->length == length && key->hash == hash) {
        size_t i;
        ubool equal = UTRUE;
        for (i = 0; i < length; i++) {
          if (!valuesEqual(key->buffer[i], buffer[i])) {
            equal = UFALSE;
            break;
          }
        }
        if (equal) {
          return key; /* We found it */
        }
      }
    }
    /* OPT: (index + 1) % dict->capacity */
    index = (index + 1) & (dict->capacity - 1);
  }
}

void dictRemoveWhite(Dict *dict) {
  size_t i;
  for (i = 0; i < dict->capacity; i++) {
    DictEntry *entry = &dict->entries[i];
    if (!IS_EMPTY_KEY(entry->key) &&
        IS_OBJ(entry->key) &&
        !AS_OBJ(entry->key)->isMarked) {
      dictDelete(dict, entry->key);
    }
  }
}

void markDict(Dict *dict) {
  size_t i;
  for (i = 0; i < dict->capacity; i++) {
    DictEntry *entry = &dict->entries[i];
    if (!IS_EMPTY_KEY(entry->key)) {
      markValue(entry->key);
      markValue(entry->value);
    }
  }
}

void initDictIterator(DictIterator *di, Dict *dict) {
  di->entry = dict->first;
}

ubool dictIteratorDone(DictIterator *di) {
  return di->entry == NULL;
}

ubool dictIteratorNext(DictIterator *di, DictEntry **out) {
  if (di->entry) {
    *out = di->entry;
    di->entry = di->entry->next;
    return UTRUE;
  }
  return UFALSE;
}

ubool dictIteratorNextKey(DictIterator *di, Value *out) {
  if (di->entry) {
    *out = di->entry->key;
    di->entry = di->entry->next;
    return UTRUE;
  }
  return UFALSE;
}
