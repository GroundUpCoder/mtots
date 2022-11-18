#include "mtots_memory.h"
#include "mtots_vm.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define ALLOCATE_OBJ(type, objectType) \
  (type*)allocateObject(sizeof(type), objectType)

/* should-be-inline */ ubool isObjType(Value value, ObjType type) {
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

static Obj *allocateObject(size_t size, ObjType type) {
  Obj *object = (Obj*)reallocate(NULL, 0, size);
  object->type = type;
  object->isMarked = UFALSE;
  object->next = vm.objects;
  vm.objects = object;

#if DEBUG_LOG_GC
  printf("%p allocate %zu for %d\n", (void*) object, size, type);
#endif

  return object;
}

ubool IS_MODULE(Value value) {
  return IS_INSTANCE(value) && AS_INSTANCE(value)->klass->isModuleClass;
}

ObjInstance *newModule(String *name, ubool includeGlobals) {
  ObjClass *klass;
  ObjInstance *instance;

  klass = newClass(name);
  klass->isModuleClass = UTRUE;

  push(CLASS_VAL(klass));
  instance = newInstance(klass);
  pop(); /* klass */

  if (includeGlobals) {
    push(INSTANCE_VAL(instance));
    mapAddAll(&vm.globals, &instance->fields);
    pop(); /* instance */
  }

  return instance;
}

ObjInstance *newModuleFromCString(const char *name, ubool includeGlobals) {
  String *nameStr;
  ObjInstance *instance;

  nameStr = internCString(name);
  push(STRING_VAL(nameStr));
  instance = newModule(nameStr, includeGlobals);
  pop(); /* nameStr */

  return instance;
}

ObjClass *newClass(String *name) {
  ObjClass *klass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
  klass->name = name;
  initMap(&klass->methods);
  klass->isModuleClass = UFALSE;
  klass->isBuiltinClass = UFALSE;
  klass->descriptor = NULL;
  return klass;
}

ObjClass *newClassFromCString(const char *name) {
  ObjClass *klass;
  String *nameObj = internCString(name);
  push(STRING_VAL(nameObj));
  klass = newClass(nameObj);
  pop(); /* nameObj */
  return klass;
}

ObjClosure *newClosure(ObjThunk *thunk, ObjInstance *module) {
  ObjUpvalue **upvalues = ALLOCATE(ObjUpvalue*, thunk->upvalueCount);
  ObjClosure *closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
  i16 i;
  for (i = 0; i < thunk->upvalueCount; i++) {
    upvalues[i] = NULL;
  }
  closure->module = module;
  closure->thunk = thunk;
  closure->upvalues = upvalues;
  closure->upvalueCount = thunk->upvalueCount;
  return closure;
}

ObjThunk *newFunction() {
  ObjThunk *thunk = ALLOCATE_OBJ(ObjThunk, OBJ_THUNK);
  thunk->arity = 0;
  thunk->upvalueCount = 0;
  thunk->name = NULL;
  thunk->defaultArgs = NULL;
  thunk->defaultArgsCount = 0;
  thunk->moduleName = NULL;
  initChunk(&thunk->chunk);
  return thunk;
}

ObjNativeClosure *newNativeClosure(
    size_t structSize,
    ubool (*body)(void *it, i16 argCount, Value *args, Value *out),
    void (*blacken)(void *it),
    void (*free)(void *it),
    const char *name,
    i16 arity,
    i16 maxArity) {
  ObjNativeClosure *nc;
  /* Struct size must be strictly greater.
   * If no extra data is needed, use CFunction val instead.
   */
  if (sizeof(ObjNativeClosure) >= structSize) {
    panic(
      "Native Closure struct size is smaller than or "
      "equal to ObjNativeClosure");
  }
  nc = (ObjNativeClosure*)allocateObject(structSize, OBJ_NATIVE_CLOSURE);
  nc->size = structSize;
  nc->body = body;
  nc->blacken = blacken;
  nc->free = free;
  nc->name = name;
  nc->arity = arity;
  nc->maxArity = maxArity;
  memset(
    ((char*)nc) + sizeof(ObjNativeClosure),
    0,
    structSize - sizeof(ObjNativeClosure));
  return nc;
}

ObjInstance *newInstance(ObjClass *klass) {
  ObjInstance *instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
  instance->klass = klass;
  initMap(&instance->fields);
  return instance;
}

static u32 hashTuple(Value *buffer, size_t length) {
  /* FNV-1a as presented in the Crafting Interpreters book */
  size_t i;
  u32 hash = 2166136261u;
  for (i = 0; i < length; i++) {
    u32 itemhash = hashval(buffer[i]);
    hash ^= (u8) (itemhash);
    hash *= 16777619;
    hash ^= (u8) (itemhash >> 8);
    hash *= 16777619;
    hash ^= (u8) (itemhash >> 16);
    hash *= 16777619;
    hash ^= (u8) (itemhash >> 24);
    hash *= 16777619;
  }
  return hash;
}

ObjByteArray *newByteArray(size_t length) {
  u8 *newBuffer = ALLOCATE(u8, length);
  ObjByteArray *byteArray = ALLOCATE_OBJ(ObjByteArray, OBJ_BYTE_ARRAY);
  memset(newBuffer, 0, length);
  byteArray->buffer = newBuffer;
  byteArray->length = length;
  return byteArray;
}

ObjByteArray *takeByteArray(u8 *buffer, size_t length) {
  ObjByteArray *byteArray = ALLOCATE_OBJ(ObjByteArray, OBJ_BYTE_ARRAY);
  byteArray->buffer = buffer;
  byteArray->length = length;
  return byteArray;
}

ObjByteArray *copyByteArray(const u8 *buffer, size_t length) {
  u8 *newBuffer = ALLOCATE(u8, length);
  memcpy(newBuffer, buffer, length);
  return takeByteArray(newBuffer, length);
}

static ObjByteArray *getUnderlyingByteArray(ObjByteArray *arrayOrView) {
  while (arrayOrView->obj.type == OBJ_BYTE_ARRAY_VIEW) {
    arrayOrView = ((ObjByteArrayView*)arrayOrView)->array;
  }
  if (arrayOrView->obj.type != OBJ_BYTE_ARRAY) {
    abort(); /* some other type of object? needs to be handled */
  }
  return arrayOrView;
}

ObjByteArrayView *newByteArrayView(
    size_t length, u8 *buffer, ObjByteArray *array) {
  ObjByteArrayView *view = ALLOCATE_OBJ(ObjByteArrayView, OBJ_BYTE_ARRAY_VIEW);
  view->obj.length = length;
  view->obj.buffer = buffer;
  view->array = getUnderlyingByteArray(array);
  if (view->obj.buffer < array->buffer ||
      view->obj.buffer >= array->buffer + array->length ||
      view->obj.buffer + view->obj.length > array->buffer + array->length) {
    /* Invalid view */
    abort();
  }
  return view;
}

ObjList *newList(size_t size) {
  ObjList *list = ALLOCATE_OBJ(ObjList, OBJ_LIST);
  list->capacity = 0;
  list->length = 0;
  list->buffer = NULL;

  /* Save onto stack, since we need to allocate more
   * before we return
   */
  push(LIST_VAL(list));

  if (size > 0) {
    size_t i;
    list->buffer = ALLOCATE(Value, size);
    list->capacity = list->length = size;
    for (i = 0; i < size; i++) {
      list->buffer[i] = NIL_VAL();
    }
  }

  pop(); /* list */

  return list;
}

static ObjTuple *allocateTuple(Value *buffer, int length, u32 hash) {
  ObjTuple *tuple = ALLOCATE_OBJ(ObjTuple, OBJ_TUPLE);
  tuple->length = length;
  tuple->buffer = buffer;
  tuple->hash = hash;

  push(TUPLE_VAL(tuple));
  mapSet(&vm.tuples, TUPLE_VAL(tuple), NIL_VAL());
  pop();

  return tuple;
}

ObjTuple *copyTuple(Value *buffer, size_t length) {
  u32 hash = hashTuple(buffer, length);
  ObjTuple *interned = mapFindTuple(&vm.tuples, buffer, length, hash);
  Value *newBuffer;
  if (interned != NULL) {
    return interned;
  }
  newBuffer = ALLOCATE(Value, length);
  memcpy(newBuffer, buffer, sizeof(Value) * length);
  return allocateTuple(newBuffer, length, hash);
}

ObjDict *newDict() {
  ObjDict *dict = ALLOCATE_OBJ(ObjDict, OBJ_DICT);
  initMap(&dict->dict);
  return dict;
}

ObjFile *newFile(FILE *file, ubool isOpen, String *name, FileMode mode) {
  ObjFile *f = ALLOCATE_OBJ(ObjFile, OBJ_FILE);
  f->file = file;
  f->isOpen = isOpen;
  f->name = name;
  f->mode = mode;
  if (name->length > MAX_PATH_LENGTH) {
    panic("File name too long: %s", name->chars);
  }
  return f;
}

ObjFile *openFile(const char *filename, FileMode mode) {
  char modestr[] = "\0\0\0";
  FILE *f;
  ObjFile *file;
  String *name = internCString(filename);
  push(STRING_VAL(name));               /* GC: keep 'name' alive */
  fileModeToString(mode, modestr);
  f = fopen(filename, modestr);
  file = newFile(f, UTRUE, name, mode);
  pop();                                    /* GC: pop 'name' */
  return file;
}

ObjNative *newNative(NativeObjectDescriptor *descriptor, size_t objectSize) {
  ObjNative *n;
  if (descriptor->objectSize != objectSize) {
    panic("Mismatched native object size");
  }
  n = (ObjNative*)allocateObject(objectSize, OBJ_NATIVE);
  n->descriptor = descriptor;
  return n;
}

ObjUpvalue *newUpvalue(Value *slot) {
  ObjUpvalue *upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
  upvalue->closed = NIL_VAL();
  upvalue->location = slot;
  upvalue->next = NULL;
  return upvalue;
}

ObjClass *getClassOfValue(Value value) {
  switch (value.type) {
    case VAL_BOOL: return vm.boolClass;
    case VAL_NIL: return vm.nilClass;
    case VAL_NUMBER: return vm.numberClass;
    case VAL_STRING: return vm.stringClass;
    case VAL_CFUNC: return vm.functionClass;
    case VAL_CFUNCTION: return vm.functionClass;
    case VAL_OPERATOR: return vm.operatorClass;
    case VAL_SENTINEL: return vm.sentinelClass;
    case VAL_OBJ: {
      switch (AS_OBJ(value)->type) {
        case OBJ_CLASS: return vm.classClass;
        case OBJ_CLOSURE: return vm.functionClass;
        case OBJ_THUNK: panic("function kinds do not have classes");
        case OBJ_NATIVE_CLOSURE: return vm.functionClass;
        case OBJ_INSTANCE: return AS_INSTANCE(value)->klass;
        case OBJ_BYTE_ARRAY: return vm.byteArrayClass;
        case OBJ_BYTE_ARRAY_VIEW: return vm.byteArrayViewClass;
        case OBJ_LIST: return vm.listClass;
        case OBJ_TUPLE: return vm.tupleClass;
        case OBJ_DICT: return vm.mapClass;
        case OBJ_FILE: return vm.fileClass;
        case OBJ_NATIVE: return AS_NATIVE(value)->descriptor->klass;
        case OBJ_UPVALUE: panic("upvalue kinds do not have classes");
      }
      break;
    }
  }
  /* TODO: Get the class for other kinds of values */
  panic("Class not found for %s kinds", getKindName(value));
  return NULL;
}

static void printFunction(ObjThunk *function) {
  if (function->name == NULL) {
    printf("<script>");
    return;
  }
  printf("<fn %s>", function->name->chars);
}

void printObject(Value value) {
  switch (OBJ_TYPE(value)) {
    case OBJ_CLASS:
      if (AS_CLASS(value)->isModuleClass) {
        printf("<module %s>", AS_CLASS(value)->name->chars);
      } else {
        printf("<class %s>", AS_CLASS(value)->name->chars);
      }
      break;
    case OBJ_CLOSURE:
      printFunction(AS_CLOSURE(value)->thunk);
      break;
    case OBJ_THUNK:
      printFunction(AS_THUNK(value));
      break;
    case OBJ_NATIVE_CLOSURE:
      printf("<function %s>", AS_NATIVE_CLOSURE(value)->name);
      break;
    case OBJ_INSTANCE:
      printf("<%s instance>", AS_INSTANCE(value)->klass->name->chars);
      break;
    case OBJ_BYTE_ARRAY:
      printf("<byteArray %lu>", (unsigned long)AS_BYTE_ARRAY(value)->length);
      break;
    case OBJ_LIST:
      printf("<list %lu items>", (unsigned long) AS_LIST(value)->length);
      break;
    case OBJ_TUPLE:
      printf("<tuple %lu items>", (unsigned long) AS_TUPLE(value)->length);
      break;
    case OBJ_DICT:
      printf("<dict>");
      break;
    case OBJ_FILE:
      printf("<file %s>", AS_FILE(value)->name->chars);
      break;
    case OBJ_NATIVE:
      printf(
        "<native-object %s>",
        AS_NATIVE(value)->descriptor->klass->name->chars);
    case OBJ_UPVALUE:
      printf("<upvalue>");
      break;
    default:
      abort();
  }
}

const char *getObjectTypeName(ObjType type) {
  switch (type) {
  case OBJ_CLASS: return "OBJ_CLASS";
  case OBJ_CLOSURE: return "OBJ_CLOSURE";
  case OBJ_THUNK: return "OBJ_THUNK";
  case OBJ_NATIVE_CLOSURE: return "OBJ_NATIVE_CLOSURE";
  case OBJ_INSTANCE: return "OBJ_INSTANCE";
  case OBJ_BYTE_ARRAY: return "OBJ_BYTE_ARRAY";
  case OBJ_BYTE_ARRAY_VIEW: return "OBJ_BYTE_ARRAY_VIEW";
  case OBJ_LIST: return "OBJ_LIST";
  case OBJ_TUPLE: return "OBJ_TUPLE";
  case OBJ_DICT: return "OBJ_DICT";
  case OBJ_FILE: return "OBJ_FILE";
  case OBJ_NATIVE: return "OBJ_NATIVE";
  case OBJ_UPVALUE: return "OBJ_UPVALUE";
  }
  return "OBJ_<unrecognized>";
}

ubool isNative(Value value, NativeObjectDescriptor *descriptor) {
  return IS_NATIVE(value) && AS_NATIVE(value)->descriptor == descriptor;
}

Value LIST_VAL(ObjList *list) {
  return OBJ_VAL_EXPLICIT((Obj*)list);
}

Value DICT_VAL(ObjDict *dict) {
  return OBJ_VAL_EXPLICIT((Obj*)dict);
}

Value INSTANCE_VAL(ObjInstance *instance) {
  return OBJ_VAL_EXPLICIT((Obj*)instance);
}

Value BYTE_ARRAY_VAL(ObjByteArray *byteArray) {
  return OBJ_VAL_EXPLICIT((Obj*)byteArray);
}

Value BYTE_ARRAY_VIEW_VAL(ObjByteArrayView *byteArrayView) {
  return OBJ_VAL_EXPLICIT((Obj*)byteArrayView);
}

Value THUNK_VAL(ObjThunk *thunk) {
  return OBJ_VAL_EXPLICIT((Obj*)thunk);
}

Value CLOSURE_VAL(ObjClosure *closure) {
  return OBJ_VAL_EXPLICIT((Obj*)closure);
}

Value FILE_VAL(ObjFile *file) {
  return OBJ_VAL_EXPLICIT((Obj*)file);
}

Value TUPLE_VAL(ObjTuple *tuple) {
  return OBJ_VAL_EXPLICIT((Obj*)tuple);
}

Value CLASS_VAL(ObjClass *klass) {
  return OBJ_VAL_EXPLICIT((Obj*)klass);
}
