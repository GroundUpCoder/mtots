#include "mtots_object.h"
#include "mtots_memory.h"
#include "mtots_value.h"
#include "mtots_vm.h"
#include "mtots_table.h"

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

ObjInstance *newModule(ObjString *name, ubool includeGlobals) {
  ObjClass *klass;
  ObjInstance *instance;

  klass = newClass(name);
  klass->isModuleClass = UTRUE;

  push(OBJ_VAL(klass));
  instance = newInstance(klass);
  pop(); /* klass */

  if (includeGlobals) {
    push(OBJ_VAL(instance));
    tableAddAll(&vm.globals, &instance->fields);
    pop(); /* instance */
  }

  return instance;
}

ObjInstance *newModuleFromCString(const char *name, ubool includeGlobals) {
  ObjString *nameStr;
  ObjInstance *instance;

  nameStr = copyCString(name);
  push(OBJ_VAL(nameStr));
  instance = newModule(nameStr, includeGlobals);
  pop(); /* nameStr */

  return instance;
}

ObjClass *newClass(ObjString *name) {
  ObjClass *klass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
  klass->name = name;
  initTable(&klass->methods);
  klass->isModuleClass = UFALSE;
  klass->isBuiltinClass = UFALSE;
  klass->descriptor = NULL;
  return klass;
}

ObjClass *newClassFromCString(const char *name) {
  ObjClass *klass;
  ObjString *nameObj = copyCString(name);
  push(OBJ_VAL(nameObj));
  klass = newClass(nameObj);
  pop(); /* nameObj */
  return klass;
}

ObjClosure *newClosure(ObjFunction *function, ObjInstance *module) {
  ObjUpvalue **upvalues = ALLOCATE(ObjUpvalue*, function->upvalueCount);
  ObjClosure *closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
  i16 i;
  for (i = 0; i < function->upvalueCount; i++) {
    upvalues[i] = NULL;
  }
  closure->module = module;
  closure->function = function;
  closure->upvalues = upvalues;
  closure->upvalueCount = function->upvalueCount;
  return closure;
}

ObjFunction *newFunction() {
  ObjFunction *function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
  function->arity = 0;
  function->upvalueCount = 0;
  function->name = NULL;
  function->defaultArgs = NULL;
  function->defaultArgsCount = 0;
  function->moduleName = NULL;
  initChunk(&function->chunk);
  return function;
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
  initTable(&instance->fields);
  return instance;
}

static ObjString *allocateString(char *chars, int length, u32 hash) {
  ObjString *string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
  string->length = length;
  string->chars = chars;
  string->hash = hash;

  push(OBJ_VAL(string));
  tableSet(&vm.strings, string, NIL_VAL());
  pop();

  return string;
}

static u32 hashString(const char *key, size_t length) {
  /* FNV-1a as presented in the Crafting Interpreters book */
  size_t i;
  u32 hash = 2166136261u;
  for (i = 0; i < length; i++) {
    hash ^= (u8) key[i];
    hash *= 16777619;
  }
  return hash;
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

/* NOTE: this function should be used with care.
 * This function computes a hash of the string the moment when it is called
 * to see if there is an interned version of this string.
 * Modifying the underlying chars array after making a call to this function
 * may lead to very strange bugs.
 */
ObjString *takeString(char *chars, size_t length) {
  u32 hash = hashString(chars, length);
  ObjString *interned = tableFindString(&vm.strings, chars, length, hash);
  if (interned != NULL) {
    FREE_ARRAY(char, chars, length + 1);
    return interned;
  }
  return allocateString(chars, length, hash);
}

ObjString *copyString(const char *chars, size_t length) {
  u32 hash = hashString(chars, length);
  ObjString *interned = tableFindString(&vm.strings, chars, length, hash);
  char *heapChars;
  if (interned != NULL) {
    return interned;
  }
  heapChars = ALLOCATE(char, length + 1);
  memcpy(heapChars, chars, length);
  heapChars[length] = '\0';
  return allocateString(heapChars, length, hash);
}

ObjString *copyCString(const char *chars) {
  return copyString(chars, strlen(chars));
}

ObjByteArray *newByteArray(size_t size) {
  unsigned char *newBuffer = ALLOCATE(unsigned char, size);
  ObjByteArray *byteArray = ALLOCATE_OBJ(ObjByteArray, OBJ_BYTE_ARRAY);
  memset(newBuffer, 0, size);
  byteArray->buffer = newBuffer;
  byteArray->size = size;
  return byteArray;
}

ObjByteArray *takeByteArray(unsigned char *buffer, size_t size) {
  ObjByteArray *byteArray = ALLOCATE_OBJ(ObjByteArray, OBJ_BYTE_ARRAY);
  byteArray->buffer = buffer;
  byteArray->size = size;
  return byteArray;
}

ObjByteArray *copyByteArray(const unsigned char *buffer, size_t size) {
  unsigned char *newBuffer = ALLOCATE(unsigned char, size);
  memcpy(newBuffer, buffer, size);
  return takeByteArray(newBuffer, size);
}

ObjList *newList(size_t size) {
  ObjList *list = ALLOCATE_OBJ(ObjList, OBJ_LIST);
  list->capacity = 0;
  list->length = 0;
  list->buffer = NULL;

  /* Save onto stack, since we need to allocate more
   * before we return
   */
  push(OBJ_VAL(list));

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

  push(OBJ_VAL(tuple));
  dictSet(&vm.tuples, OBJ_VAL(tuple), NIL_VAL());
  pop();

  return tuple;
}

ObjTuple *copyTuple(Value *buffer, size_t length) {
  u32 hash = hashTuple(buffer, length);
  ObjTuple *interned = dictFindTuple(&vm.tuples, buffer, length, hash);
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
  initDict(&dict->dict);
  return dict;
}

ObjFile *newFile(FILE *file, ubool isOpen, ObjString *name, FileMode mode) {
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
  ObjString *name = copyCString(filename);
  push(OBJ_VAL(name));               /* GC: keep 'name' alive */
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

ObjClass *getClass(Value value) {
  switch (value.type) {
    case VAL_BOOL: return vm.boolClass;
    case VAL_NIL: return vm.nilClass;
    case VAL_NUMBER: return vm.numberClass;
    case VAL_CFUNCTION: return vm.functionClass;
    case VAL_OPERATOR: return vm.operatorClass;
    case VAL_SENTINEL: return vm.sentinelClass;
    case VAL_OBJ: {
      switch (AS_OBJ(value)->type) {
        case OBJ_CLASS: return vm.classClass;
        case OBJ_CLOSURE: return vm.functionClass;
        case OBJ_FUNCTION: panic("function kinds do not have classes");
        case OBJ_NATIVE_CLOSURE: return vm.functionClass;
        case OBJ_INSTANCE: return AS_INSTANCE(value)->klass;
        case OBJ_STRING: return vm.stringClass;
        case OBJ_BYTE_ARRAY: return vm.byteArrayClass;
        case OBJ_LIST: return vm.listClass;
        case OBJ_TUPLE: return vm.tupleClass;
        case OBJ_DICT: return vm.dictClass;
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

static void printFunction(ObjFunction *function) {
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
      printFunction(AS_CLOSURE(value)->function);
      break;
    case OBJ_FUNCTION:
      printFunction(AS_FUNCTION(value));
      break;
    case OBJ_NATIVE_CLOSURE:
      printf("<function %s>", AS_NATIVE_CLOSURE(value)->name);
      break;
    case OBJ_INSTANCE:
      printf("<%s instance>", AS_INSTANCE(value)->klass->name->chars);
      break;
    case OBJ_STRING:
      printf("%s", AS_CSTRING(value));
      break;
    case OBJ_BYTE_ARRAY:
      printf("<byteArray %lu>", (unsigned long)AS_BYTE_ARRAY(value)->size);
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
  case OBJ_FUNCTION: return "OBJ_FUNCTION";
  case OBJ_NATIVE_CLOSURE: return "OBJ_NATIVE_CLOSURE";
  case OBJ_INSTANCE: return "OBJ_INSTANCE";
  case OBJ_STRING: return "OBJ_STRING";
  case OBJ_BYTE_ARRAY: return "OBJ_BYTE_ARRAY";
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
