#ifndef mtots_table_h
#define mtots_table_h

#include "mtots_common.h"
#include "mtots_value.h"

typedef struct {
  ObjString *key;
  Value value;
} Entry;

typedef struct {
  size_t count;
  size_t capacity; /* 0 or (8 * <power of 2>) */
  size_t size;     /* actual number of active elements */
  Entry *entries;
} Table;

void initTable(Table *table);
void freeTable(Table *table);
ubool tableGet(Table *table, ObjString *key, Value *value);
ubool tableSet(Table *table, ObjString *key, Value value);
ubool tableSetN(Table *table, const char *key, Value value);
ubool tableDelete(Table *table, ObjString *key);
void tableAddAll(Table *from, Table *to);
ObjString *tableFindString(
  Table *table, const char *chars, size_t length, u32 hash);
void tableRemoveWhite(Table *table);
void markTable(Table *table);

#endif/*mtots_table_h*/
