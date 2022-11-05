#ifndef mtots_class_list_impl_h
#define mtots_class_list_impl_h
#include "mtots_class_list.h"
#include "mtots_memory.h"
#include "mtots_value.h"
#include "mtots_vm.h"

static void listAppend(ObjList *list, Value value) {
  if (list->capacity < list->length + 1) {
    size_t oldCapacity = list->capacity;
    list->capacity = GROW_CAPACITY(list->capacity);
    list->buffer = GROW_ARRAY(
      Value, list->buffer, oldCapacity, list->capacity);
  }
  list->buffer[list->length++] = value;
}

static ubool implListAppend(i16 argCount, Value *args, Value *out) {
  Value receiver = args[-1];
  ObjList *list;
  if (!IS_LIST(receiver)) {
    runtimeError("Expected list as receiver to List.append()");
    return UFALSE;
  }
  list = AS_LIST(receiver);
  listAppend(list, args[0]);
  return UTRUE;
}

static CFunction funcListAppend = { implListAppend, "append", 1 };

static ubool implListPop(i16 argCount, Value *args, Value *out) {
  Value receiver = args[-1];
  ObjList *list;
  if (!IS_LIST(receiver)) {
    runtimeError("Expected list as receiver to List.pop()");
    return UFALSE;
  }
  list = AS_LIST(receiver);
  if (list->length == 0) {
    runtimeError("Pop from an empty List");
    return UFALSE;
  }
  *out = list->buffer[--list->length];
  return UTRUE;
}

static CFunction funcListPop = { implListPop, "pop", 0 };

static ubool implListMul(i16 argCount, Value *args, Value *out) {
  Value receiver = args[-1];
  ObjList *list, *result;
  size_t r, rep = AS_U32(args[0]);
  if (!IS_LIST(receiver)) {
    runtimeError("Expected list as receiver to List.append()");
    return UFALSE;
  }
  list = AS_LIST(receiver);
  result = newList(list->length * rep);
  for (r = 0; r < rep; r++) {
    size_t i;
    for (i = 0; i < list->length; i++) {
      result->buffer[r * list->length + i] = list->buffer[i];
    }
  }
  *out = OBJ_VAL(result);
  return UTRUE;
}

static TypePattern argsListMul[] = {
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcListMul = { implListMul, "__mul__",
  sizeof(argsListMul)/sizeof(TypePattern), 0, argsListMul };

static ubool implListGetItem(i16 argCount, Value *args, Value *out) {
  Value receiver = args[-1];
  ObjList *list;
  i32 index;
  if (!IS_LIST(receiver)) {
    runtimeError("Expected list as receiver to List.__getitem__()");
    return UFALSE;
  }
  list = AS_LIST(receiver);
  if (!IS_NUMBER(args[0])) {
    runtimeError("Expcted List index to be a number");
    return UFALSE;
  }
  index = (i32) AS_NUMBER(args[0]);
  if (index < 0) {
    index += list->length;
  }
  if (index < 0 || index >= list->length) {
    runtimeError("List index out of bounds");
    return UFALSE;
  }
  *out = list->buffer[index];
  return UTRUE;
}

static CFunction funcListGetItem = { implListGetItem, "__getitem__", 1 };

static ubool implListSetItem(i16 argCount, Value *args, Value *out) {
  Value receiver = args[-1];
  ObjList *list;
  i32 index;
  if (!IS_LIST(receiver)) {
    runtimeError("Expected list as receiver to List.__setitem__()");
    return UFALSE;
  }
  list = AS_LIST(receiver);
  if (!IS_NUMBER(args[0])) {
    runtimeError("Expected List index to be a number");
    return UFALSE;
  }
  index = (i32) AS_NUMBER(args[0]);
  if (index < 0) {
    index += list->length;
  }
  if (index < 0 || index >= list->length) {
    runtimeError("List index out of bounds");
    return UFALSE;
  }
  list->buffer[index] = args[1];
  return UTRUE;
}

static CFunction funcListSetItem = { implListSetItem, "__setitem__", 2 };

typedef struct ObjListIterator {
  ObjNativeClosure obj;
  ObjList *list;
  size_t index;
} ObjListIterator;

static ubool implListIterator(
    void *it, i16 argCount, Value *args, Value *out) {
  ObjListIterator *iter = (ObjListIterator*)it;
  if (iter->index < iter->list->length) {
    *out = iter->list->buffer[iter->index++];
  } else {
    *out = STOP_ITERATION_VAL();
  }
  return UTRUE;
}

static void blackenListIterator(void *it) {
  ObjListIterator *li = (ObjListIterator*)it;
  markObject((Obj*)(li->list));
}

static ubool implListIter(i16 argCount, Value *args, Value *out) {
  Value receiver = args[-1];
  ObjList *list;
  ObjListIterator *iter;
  if (!IS_LIST(receiver)) {
    runtimeError("Expected list as receiver to List.__setitem__()");
    return UFALSE;
  }
  list = AS_LIST(receiver);
  iter = NEW_NATIVE_CLOSURE(
    ObjListIterator,
    implListIterator,
    blackenListIterator,
    NULL,
    "ListIterator", 0, 0);
  iter->list = list;
  iter->index = 0;
  *out = OBJ_VAL(iter);
  return UTRUE;
}

static CFunction funcListIter = { implListIter, "__iter__", 0 };

void initListClass() {
  ObjString *tmpstr;
  CFunction *methods[] = {
    &funcListAppend,
    &funcListPop,
    &funcListMul,
    &funcListGetItem,
    &funcListSetItem,
    &funcListIter,
  };
  size_t i;
  ObjClass *cls;

  tmpstr = copyCString("List");
  push(OBJ_VAL(tmpstr));
  cls = vm.listClass = newClass(tmpstr);
  cls->isBuiltinClass = UTRUE;
  pop();

  for (i = 0; i < sizeof(methods) / sizeof(CFunction*); i++) {
    tmpstr = copyCString(methods[i]->name);
    push(OBJ_VAL(tmpstr));
    tableSet(
      &cls->methods, tmpstr, CFUNCTION_VAL(methods[i]));
    pop();
  }
}
#endif/*mtots_class_list_impl_h*/
