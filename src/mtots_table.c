#include "mtots_table.h"
#include "mtots_memory.h"
#include "mtots_object.h"
#include "mtots_value.h"
#include "mtots_stack.h"

#include <stdlib.h>
#include <string.h>

#define TABLE_MAX_LOAD 0.75

void initTable(Table *table) {
  table->count = 0;
  table->capacity = 0;
  table->size = 0;
  table->entries = NULL;
}

void freeTable(Table *table) {
  FREE_ARRAY(Entry, table->entries, table->capacity);
  initTable(table);
}

/* NOTE: capacity should always be non-zero.
 * If findEntry was a non-static function, I probably would do the check
 * in the function itself. But since it is static, all places where
 * it can be called are within this file.
 */
static Entry *findEntry(Entry *entries, size_t capacity, ObjString *key) {
  /* OPT: key->hash % capacity */
  u32 index = key->hash & (capacity - 1);
  Entry *tombstone = NULL;
  for (;;) {
    Entry *entry = &entries[index];
    if (entry->key == NULL) {
      if (IS_NIL(entry->value)) {
        /* Empty entry */
        return tombstone != NULL ? tombstone : entry;
      } else if (tombstone == NULL) {
        /* We found a tombstone */
        tombstone = entry;
      }
    } else if (entry->key == key) {
      /* We found the key */
      return entry;
    }
    /* OPT: (index + 1) % capacity */
    index = (index + 1) & (capacity - 1);
  }
}

ubool tableGet(Table *table, ObjString *key, Value *value) {
  Entry *entry;

  if (table->count == 0) {
    return UFALSE;
  }

  entry = findEntry(table->entries, table->capacity, key);
  if (entry->key == NULL) {
    return UFALSE;
  }

  *value = entry->value;
  return UTRUE;
}

static void adjustCapacity(Table *table, size_t capacity) {
  size_t i;
  Entry *entries = ALLOCATE(Entry, capacity);
  for (i = 0; i < capacity; i++) {
    entries[i].key = NULL;
    entries[i].value = NIL_VAL();
  }

  table->count = 0;
  for (i = 0; i < table->capacity; i++) {
    Entry *entry = &table->entries[i], *dest;
    if (entry->key == NULL) continue;

    dest = findEntry(entries, capacity, entry->key);
    dest->key = entry->key;
    dest->value = entry->value;
    table->count++;
  }

  FREE_ARRAY(Entry, table->entries, table->capacity);
  table->entries = entries;
  table->capacity = capacity;
}

ubool tableSet(Table *table, ObjString *key, Value value) {
  Entry *entry;
  ubool isNewKey;

  if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
    size_t capacity = GROW_CAPACITY(table->capacity);
    adjustCapacity(table, capacity);
  }
  entry = findEntry(table->entries, table->capacity, key);
  isNewKey = entry->key == NULL;

  if (isNewKey) {
    /* If entry->value is not nil, we're reusing a tombstone
    * so we don't want to increment the count since tombstones
    * are already included in count.
    * We include tombstones in the count so that the loadfactor
    * is sensitive to slots occupied by tombstones */
    if (IS_NIL(entry->value)) {
      table->count++;
    }
    table->size++;
  }
  entry->key = key;
  entry->value = value;
  return isNewKey;
}

/* Like tableSet, but a version that's more convenient for use in
 * native code in two ways:
 *   1) the key parameter is a C-string that is automatically be converted
 *      to an ObjString and properly retained and released using the stack
 *   2) the value parameter will retained and popped from the stack, so that
 *      even if tableSet or copyCString triggers a reallocation, the value
 *      will not be collected.
 */
ubool tableSetN(Table *table, const char *key, Value value) {
  ubool result;
  ObjString *keystr;

  push(value);
  keystr = copyCString(key);
  push(OBJ_VAL(keystr));
  result = tableSet(table, keystr, value);
  pop(); /* keystr */
  pop(); /* value */
  return result;
}

ubool tableDelete(Table *table, ObjString *key) {
  Entry *entry;

  if (table->count == 0) {
    return UFALSE;
  }

  entry = findEntry(table->entries, table->capacity, key);
  if (entry->key == NULL) {
    return UFALSE;
  }

  /* Place a tombstone in the entry */
  entry->key = NULL;
  entry->value = BOOL_VAL(1);
  table->size--;
  return UTRUE;
}

void tableAddAll(Table *from, Table *to) {
  size_t i;
  for (i = 0; i < from->capacity; i++) {
    Entry *entry = &from->entries[i];
    if (entry->key != NULL) {
      tableSet(to, entry->key, entry->value);
    }
  }
}

ObjString *tableFindString(
    Table *table,
    const char *chars,
    size_t length,
    u32 hash) {
  u32 index;
  if (table->count == 0) {
    return NULL;
  }
  /* OPT: hash % table->capacity */
  index = hash & (table->capacity - 1);
  for (;;) {
    Entry *entry = &table->entries[index];
    if (entry->key == NULL) {
      /* Stop if we find an empty non-tombstone entry */
      if (IS_NIL(entry->value)) {
        return NULL;
      }
    } else if (entry->key->length == length &&
        entry->key->hash == hash &&
        memcmp(entry->key->chars, chars, length) == 0) {
      /* We found it */
      return entry->key;
    }
    /* OPT: (index + 1) % table->capacity */
    index = (index + 1) & (table->capacity - 1);
  }
}

void tableRemoveWhite(Table *table) {
  size_t i;
  for (i = 0; i < table->capacity; i++) {
    Entry *entry = &table->entries[i];
    if (entry->key != NULL && !entry->key->obj.isMarked) {
      tableDelete(table, entry->key);
    }
  }
}

void markTable(Table *table) {
  size_t i;
  for (i = 0; i < table->capacity; i++) {
    Entry *entry = &table->entries[i];
    markObject((Obj*)entry->key);
    markValue(entry->value);
  }
}

void initTableIterator(TableIterator *ti, Table *table) {
  ti->capacity = table->capacity;
  ti->index = 0;
  ti->entries = table->entries;
}

ubool tableIteratorNext(TableIterator *ti, Entry **out) {
  for (; ti->index < ti->capacity; ti->index++) {
    Entry *entry = &ti->entries[ti->index];
    if (entry->key == NULL) continue;
    *out = entry;
    ti->index++;
    return UTRUE;
  }
  return UFALSE;
}
