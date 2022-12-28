#include "mtots_ops.h"
#include "mtots_vm.h"

#include <stdlib.h>
#include <string.h>

ubool valuesIs(Value a, Value b) {
  if (a.type != b.type) {
    return UFALSE;
  }
  switch (a.type) {
    case VAL_BOOL: return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NIL: return UTRUE;
    case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
    case VAL_STRING: return AS_STRING(a) == AS_STRING(b);
    case VAL_CFUNCTION: return AS_CFUNCTION(a) == AS_CFUNCTION(b);
    case VAL_OPERATOR: return AS_OPERATOR(a) == AS_OPERATOR(b);
    case VAL_SENTINEL: return AS_SENTINEL(a) == AS_SENTINEL(b);
    case VAL_OBJ: return AS_OBJ(a) == AS_OBJ(b);
  }
  abort();
  return UFALSE; /* Unreachable */
}

ubool mapsEqual(Map *a, Map *b) {
  MapIterator di;
  MapEntry *entry;
  if (a->size != b->size) {
    return UFALSE;
  }
  initMapIterator(&di, a);
  while (mapIteratorNext(&di, &entry)) {
    Value key = entry->key;
    if (!IS_EMPTY_KEY(key)) {
      Value value1 = entry->value, value2;
      if (!mapGet(b, key, &value2)) {
        return UFALSE;
      }
      if (!valuesEqual(value1, value2)) {
        return UFALSE;
      }
    }
  }
  return UTRUE;
}

ubool valuesEqual(Value a, Value b) {
  if (a.type != b.type) {
    return UFALSE;
  }
  switch (a.type) {
    case VAL_BOOL: return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NIL: return UTRUE;
    case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
    case VAL_STRING: return AS_STRING(a) == AS_STRING(b);
    case VAL_CFUNCTION: return AS_CFUNCTION(a) == AS_CFUNCTION(b);
    case VAL_OPERATOR: return AS_OPERATOR(a) == AS_OPERATOR(b);
    case VAL_SENTINEL: return AS_SENTINEL(a) == AS_SENTINEL(b);
    case VAL_OBJ: {
      Obj *objA = AS_OBJ(a);
      Obj *objB = AS_OBJ(b);
      if (objA->type != objB->type) {
        return UFALSE;
      }
      if (objA == objB) {
        return UTRUE;
      }
      switch (objA->type) {
        case OBJ_BUFFER: {
          ObjBuffer *bA = (ObjBuffer*)objA, *bB = (ObjBuffer*)objB;
          if (bA->buffer.length != bB->buffer.length) {
            return UFALSE;
          }
          return memcmp(bA->buffer.data, bB->buffer.data, bA->buffer.length) == 0;
        }
        case OBJ_LIST: {
          ObjList *listA = (ObjList*)objA, *listB = (ObjList*)objB;
          size_t i;
          if (listA->length != listB->length) {
            return UFALSE;
          }
          for (i = 0; i < listA->length; i++) {
            if (!valuesEqual(listA->buffer[i], listB->buffer[i])) {
              return UFALSE;
            }
          }
          return UTRUE;
        }
        case OBJ_DICT: {
          ObjDict *dictA = (ObjDict*)objA, *dictB = (ObjDict*)objB;
          return mapsEqual(&dictA->map, &dictB->map);
        }
        default: return objA == objB;
      }
    }
  }
  abort();
  return UFALSE; /* Unreachable */
}

ubool valueLessThan(Value a, Value b) {
  if (a.type != b.type) {
    panic(
      "'<' requires values of the same type but got %s and %s",
      getKindName(a), getKindName(b));
  }
  switch (a.type) {
    case VAL_BOOL: return AS_BOOL(a) < AS_BOOL(b);
    case VAL_NIL: return UFALSE;
    case VAL_NUMBER: return AS_NUMBER(a) < AS_NUMBER(b);
    case VAL_STRING: {
      String *strA = AS_STRING(a);
      String *strB = AS_STRING(b);
      size_t lenA = strA->length;
      size_t lenB = strB->length;
      size_t len = lenA < lenB ? lenA : lenB;
      size_t i;
      const char *charsA = strA->chars;
      const char *charsB = strB->chars;
      if (strA == strB) {
        return UFALSE;
      }
      for (i = 0; i < len; i++) {
        if (charsA[i] != charsB[i]) {
          return charsA[i] < charsB[i];
        }
      }
      return lenA < lenB;
    }
    case VAL_CFUNCTION: break;
    case VAL_OPERATOR: break;
    case VAL_SENTINEL: break;
    case VAL_OBJ: {
      Obj *objA = AS_OBJ(a);
      Obj *objB = AS_OBJ(b);
      if (objA->type != objB->type) {
        panic(
          "'<' requires values of the same type but got %s and %s",
          getKindName(a), getKindName(b));
      }
      switch (objA->type) {
        case OBJ_LIST: {
          ObjList *listA = (ObjList*)objA;
          ObjList *listB = (ObjList*)objB;
          size_t lenA = listA->length;
          size_t lenB = listB->length;
          size_t len = lenA < lenB ? lenA : lenB;
          size_t i;
          Value *bufA = listA->buffer;
          Value *bufB = listB->buffer;
          if (listA == listB) {
            return UFALSE;
          }
          for (i = 0; i < len; i++) {
            if (!valuesEqual(bufA[i], bufB[i])) {
              return valueLessThan(bufA[i], bufB[i]);
            }
          }
          return lenA < lenB;
        }
        case OBJ_TUPLE: {
          ObjTuple *tupleA = (ObjTuple*)objA;
          ObjTuple *tupleB = (ObjTuple*)objB;
          size_t lenA = tupleA->length;
          size_t lenB = tupleB->length;
          size_t len = lenA < lenB ? lenA : lenB;
          size_t i;
          Value *bufA = tupleA->buffer;
          Value *bufB = tupleB->buffer;
          if (tupleA == tupleB) {
            return UFALSE;
          }
          for (i = 0; i < len; i++) {
            if (!valuesEqual(bufA[i], bufB[i])) {
              return valueLessThan(bufA[i], bufB[i]);
            }
          }
          return lenA < lenB;
        }
        default: break;
      }
      break;
    }
  }
  panic("%s does not support '<'", getKindName(a));
  return UFALSE;
}

typedef struct SortEntry {
  Value key;
  Value value;
} SortEntry;

/* Basic mergesort.
 * TODO: Consider using timsort instead */
void sortList(ObjList *list, ObjList *keys) {
  SortEntry *buffer, *src, *dst;
  size_t i, len = list->length, width;
  /* TODO: this check is untested - come back and
    * actually think this through when there is more bandwidth */
  if (len >= ((size_t)(-1)) / 4) {
    panic("sortList(): list too long (%lu)", (long) len);
  }
  if (keys != NULL && len != keys->length) {
    panic(
      "sortList(): item list and key list lengths do not match: "
      "%lu, %lu",
      (unsigned long) list->length, (unsigned long) keys->length);
  }
  /* TODO: Consider falling back to an in-place sorting algorithm
   * if we run do not have enough memory for the buffer (maybe qsort?) */
  /* NOTE: We call malloc directly instead of ALLOCATE because
   * I don't want to potentially trigger a GC call here.
   * And besides, none of the memory we allocate in this call should
   * outlive this call */
  buffer = malloc(sizeof(SortEntry) * 2 * len);
  src = buffer;
  dst = buffer + len;
  for (i = 0; i < len; i++) {
    src[i].key = keys == NULL ? list->buffer[i] : keys->buffer[i];
    src[i].value = list->buffer[i];
  }

  /* bottom up merge sort */
  for (width = 1; width < len; width *= 2) {
    for (i = 0; i < len; i += 2 * width) {
      size_t low  = i;
      size_t mid  = i +     width < len ? i +     width : len;
      size_t high = i + 2 * width < len ? i + 2 * width : len;
      size_t a = low, b = mid, j;
      for (j = low; j < high; j++) {
        dst[j] =
          b < high && (a >= mid || valueLessThan(src[b].key, src[a].key)) ?
            src[b++] : src[a++];
      }
    }
    {
      SortEntry *tmp = src;
      src = dst;
      dst = tmp;
    }
  }

  /* copy contents back into the list */
  for (i = 0; i < len; i++) {
    list->buffer[i] = src[i].value;
  }

  free(buffer);
}

static ubool mapRepr(StringBuffer *out, Map *map) {
  MapIterator mi;
  MapEntry *entry;
  size_t i;

  sbputchar(out, '{');
  initMapIterator(&mi, map);
  for (i = 0; mapIteratorNext(&mi, &entry); i++) {
    if (i > 0) {
      sbputchar(out, ',');
      sbputchar(out, ' ');
    }
    if (!valueRepr(out, entry->key)) {
      return UFALSE;
    }
    if (!IS_NIL(entry->value)) {
      sbputchar(out, ':');
      sbputchar(out, ' ');
      if (!valueRepr(out, entry->value)) {
        return UFALSE;
      }
    }
  }
  sbputchar(out, '}');
  return UTRUE;
}

ubool valueRepr(StringBuffer *out, Value value) {
  switch (value.type) {
    case VAL_BOOL: sbprintf(out, AS_BOOL(value) ? "true" : "false"); return UTRUE;
    case VAL_NIL: sbprintf(out, "nil"); return UTRUE;
    case VAL_NUMBER: StringBufferWriteNumber(out, AS_NUMBER(value)); return UTRUE;
    case VAL_STRING: {
      String *str = AS_STRING(value);
      sbputchar(out, '"');
      if (!escapeString2(out, str->chars, str->length, NULL)) {
        return UFALSE;
      }
      sbputchar(out, '"');
      return UTRUE;
    }
    case VAL_CFUNCTION: sbprintf(out, "<function %s>", AS_CFUNCTION(value)->name); return UTRUE;
    case VAL_OPERATOR: sbprintf(out, "<operator %d>", AS_OPERATOR(value)); return UTRUE;
    case VAL_SENTINEL: sbprintf(out, "<sentinel %d>", AS_SENTINEL(value)); return UTRUE;
    case VAL_OBJ: {
      Obj *obj = AS_OBJ(value);
      switch (obj->type) {
        case OBJ_CLASS: sbprintf(out, "<class %s>", AS_CLASS(value)->name->chars); return UTRUE;
        case OBJ_CLOSURE:
          sbprintf(out, "<function %s>", AS_CLOSURE(value)->thunk->name->chars);
          return UTRUE;
        case OBJ_THUNK: sbprintf(out, "<thunk %s>", AS_THUNK(value)->name->chars); return UTRUE;
        case OBJ_NATIVE_CLOSURE:
          sbprintf(out, "<native-closure %s>", AS_NATIVE_CLOSURE(value)->name);
          return UTRUE;
        case OBJ_INSTANCE:
          if (AS_INSTANCE(value)->klass->isModuleClass) {
            sbprintf(out, "<module %s>", AS_INSTANCE(value)->klass->name->chars);
          } else {
            sbprintf(out, "<%s instance>", AS_INSTANCE(value)->klass->name->chars);
          }
          return UTRUE;
        case OBJ_BUFFER: {
          ObjBuffer *bufObj = AS_BUFFER(value);
          Buffer *buf = &bufObj->buffer;
          StringEscapeOptions opts;
          initStringEscapeOptions(&opts);
          opts.shorthandControlCodes = UFALSE;
          opts.tryUnicode = UFALSE;
          sbputchar(out, 'b');
          sbputchar(out, '"');
          escapeString2(out, (const char*)buf->data, buf->length, &opts);
          sbputchar(out, '"');
          return UTRUE;
        }
        case OBJ_LIST: {
          ObjList *list = AS_LIST(value);
          size_t i, len = list->length;
          sbputchar(out, '[');
          for (i = 0; i < len; i++) {
            if (i > 0) {
              sbputchar(out, ',');
              sbputchar(out, ' ');
            }
            if (!valueRepr(out, list->buffer[i])) {
              return UFALSE;
            }
          }
          sbputchar(out, ']');
          return UTRUE;
        }
        case OBJ_TUPLE: {
          ObjTuple *tuple = AS_TUPLE(value);
          size_t i, len = tuple->length;
          sbputchar(out, '(');
          for (i = 0; i < len; i++) {
            if (i > 0) {
              sbputchar(out, ',');
              sbputchar(out, ' ');
            }
            if (!valueRepr(out, tuple->buffer[i])) {
              return UFALSE;
            }
          }
          sbputchar(out, ')');
          return UTRUE;
        }
        case OBJ_DICT: {
          ObjDict *dict = AS_DICT(value);
          return mapRepr(out, &dict->map);
        }
        case OBJ_FROZEN_DICT: {
          ObjFrozenDict *dict = AS_FROZEN_DICT(value);
          sbputstr(out, "final");
          return mapRepr(out, &dict->map);
        }
        case OBJ_FILE:
          sbprintf(out, "<file %s>", AS_FILE(value)->name->chars);
          return UTRUE;
        case OBJ_NATIVE:
          sbprintf(out, "<%s native-instance>",
            AS_NATIVE(value)->descriptor->klass->name->chars);
          return UTRUE;
        case OBJ_UPVALUE:
          sbprintf(out, "<upvalue>");
          return UTRUE;
      }
    }
  }
  panic("unrecognized value type %s", getKindName(value));
  return UFALSE;
}

ubool valueStr(StringBuffer *out, Value value) {
  if (IS_STRING(value)) {
    String *string = AS_STRING(value);
    sbputstrlen(out, string->chars, string->length);
    return UTRUE;
  }
  return valueRepr(out, value);
}

ubool strMod(StringBuffer *out, const char *format, ObjList *args) {
  const char *p;
  size_t j;

  for (p = format, j = 0; *p != '\0'; p++) {
    if (*p == '%') {
      Value item;
      p++;
      if (*p == '%') {
        sbputchar(out, '%');
        continue;
      }
      if (j >= args->length) {
        runtimeError("Not enough arguments for format string");
        return UFALSE;
      }
      item = args->buffer[j++];
      switch (*p) {
        case 's':
          if (!valueStr(out, item)) {
            return UFALSE;
          }
          break;
        case 'r':
          if (!valueRepr(out, item)) {
            return UFALSE;
          }
          break;
        case '\0':
          runtimeError("missing format indicator");
          return UFALSE;
        default:
          runtimeError("invalid format indicator '%%%c'", *p);
          return UFALSE;
      }
    } else {
      sbputchar(out, *p);
    }
  }

  return UTRUE;
}
