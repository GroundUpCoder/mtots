#ifndef mtots_dict_h
#define mtots_dict_h

#include "mtots_value.h"

typedef struct {
  Value key;
  Value value;
} DictEntry;

typedef struct {
  /* Count is stored for the purposes of
   * keeping track of the load factor.
   * The count used for this purpose includes
   * tombstones, so the number here may not actually
   * be the number of live entries in this Dict.
   * To get the actual number of entries in this Dict,
   * see field 'size'.
   */
  size_t count;
  size_t capacity; /* 0 or (8 * <power of 2>) */
  size_t size;     /* actual number of active elements */
  DictEntry *entries;
} Dict;

void initDict(Dict *dict);
void freeDict(Dict *dict);
ubool dictGet(Dict *dict, Value key, Value *value);
ubool dictSet(Dict *dict, Value key, Value value);
ubool dictDelete(Dict *dict, Value key);
void markDict(Dict *dict);

#endif/*mtots_dict_h*/
