#ifndef mtots_dict_h
#define mtots_dict_h

#include "mtots_value.h"

struct ObjTuple;

typedef struct DictEntry {
  Value key;
  Value value;
  struct DictEntry *prev;
  struct DictEntry *next;
} DictEntry;

/* TODO: Do what Python does with Dict layout
 * https://mail.python.org/pipermail/python-dev/2012-December/123028.html
 */
typedef struct Dict {
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
  DictEntry *first;
  DictEntry *last;
} Dict;

typedef struct DictIterator {
  DictEntry *entry;
} DictIterator;

u32 hashval(Value value);

void initDict(Dict *dict);
void freeDict(Dict *dict);
ubool dictGet(Dict *dict, Value key, Value *value);
ubool dictSet(Dict *dict, Value key, Value value);
ubool dictSetN(Dict *dict, const char *key, Value value);
ubool dictDelete(Dict *dict, Value key);
struct ObjTuple *dictFindTuple(
    Dict *dict,
    Value *buffer,
    size_t length,
    u32 hash);
void dictRemoveWhite(Dict *dict);
void markDict(Dict *dict);

void initDictIterator(DictIterator *di, Dict *dict);
ubool dictIteratorNext(DictIterator *di, DictEntry **out);
ubool dictIteratorNextKey(DictIterator *di, Value *out);

#endif/*mtots_dict_h*/
