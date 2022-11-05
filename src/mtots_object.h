#ifndef mtots_object_h
#define mtots_object_h

#include "mtots_common.h"
#include "mtots_value.h"
#include "mtots_chunk.h"
#include "mtots_table.h"
#include "mtots_file.h"
#include "mtots_dict.h"

#include <stdio.h>

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_CLASS(value) isObjType(value, OBJ_CLASS)
#define IS_CLOSURE(value) isObjType(value, OBJ_CLOSURE)
#define IS_FUNCTION(value) isObjType(value, OBJ_FUNCTION)
#define IS_NATIVE_CLOSURE(value) isObjType(value, OBJ_NATIVE_CLOSURE)
#define IS_INSTANCE(value) isObjType(value, OBJ_INSTANCE)
#define IS_STRING(value) isObjType(value, OBJ_STRING)
#define IS_BYTE_ARRAY(value) isObjType(value, OBJ_BYTE_ARRAY)
#define IS_BYTE_ARRAY_VIEW(value) isObjType(value, OBJ_BYTE_ARRAY_VIEW)
#define IS_LIST(value) isObjType(value, OBJ_LIST)
#define IS_TUPLE(value) isObjType(value, OBJ_TUPLE)
#define IS_DICT(value) isObjType(value, OBJ_DICT)
#define IS_FILE(value) isObjType(value, OBJ_FILE)
#define IS_NATIVE(value) isObjType(value, OBJ_NATIVE)

#define AS_CLASS(value) ((ObjClass*)AS_OBJ(value))
#define AS_CLOSURE(value) ((ObjClosure*)AS_OBJ(value))
#define AS_FUNCTION(value) ((ObjFunction*)AS_OBJ(value))
#define AS_NATIVE_CLOSURE(value) ((ObjNativeClosure*)AS_OBJ(value))
#define AS_INSTANCE(value) ((ObjInstance*)AS_OBJ(value))
#define AS_STRING(value) ((ObjString*)AS_OBJ(value))
#define AS_BYTE_ARRAY(value) ((ObjByteArray*)AS_OBJ(value))
#define AS_BYTE_ARRAY_VIEW(value) ((ObjByteArrayView*)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)
#define AS_LIST(value) ((ObjList*)AS_OBJ(value))
#define AS_TUPLE(value) ((ObjTuple*)AS_OBJ(value))
#define AS_DICT(value) ((ObjDict*)AS_OBJ(value))
#define AS_FILE(value) ((ObjFile*)AS_OBJ(value))
#define AS_NATIVE(value) ((ObjNative*)AS_OBJ(value))

#define NEW_NATIVE_CLOSURE( \
    type, body, blacken, free, name, arity, maxArity) ( \
  (type*)newNativeClosure( \
    sizeof(type), body, blacken, free, name, arity, maxArity))

#define NEW_NATIVE(type, descriptor) \
  ((type*)newNative(descriptor, sizeof(type)))

typedef struct ObjNative ObjNative;
typedef struct ObjClass ObjClass;
typedef struct ObjInstance ObjInstance;

typedef enum {
  OBJ_CLASS,
  OBJ_CLOSURE,
  OBJ_FUNCTION,
  OBJ_NATIVE_CLOSURE,
  OBJ_INSTANCE,
  OBJ_STRING,
  OBJ_BYTE_ARRAY,
  OBJ_BYTE_ARRAY_VIEW,
  OBJ_LIST,
  OBJ_TUPLE,
  OBJ_DICT,

  OBJ_FILE,

  OBJ_NATIVE,

  OBJ_UPVALUE
} ObjType;

struct Obj {
  ObjType type;
  ubool isMarked;
  struct Obj *next;
};

typedef struct ObjFunction {
  Obj obj;
  i16 arity;
  i16 upvalueCount;
  Chunk chunk;
  ObjString *name;
  Value *defaultArgs;
  i16 defaultArgsCount;
  ObjString *moduleName;
} ObjFunction;

struct ObjString {
  Obj obj;
  size_t length;
  char *chars;
  u32 hash;
};

/* NOTE: ByteArray objects can never change size. This is because in many
 * cases, a ByteArray will be used as a backing buffer (e.g. for SDL_Surface)
 * when one is needed. Reallocating could potentially invalidate data structures
 * that point to the underlying buffer */
typedef struct ObjByteArray {
  Obj obj;
  size_t length;
  u8 *buffer;
} ObjByteArray;

/* NOTE: The fields in ObjByteArrayView should always match ObjByteArray
 * In some cases, ObjByteArrayViews will be cast as ObjByteArray */
typedef struct ObjByteArrayView {
  ObjByteArray obj;
  ObjByteArray *array;    /* for GC to keep underlying array alive */
} ObjByteArrayView;

typedef struct ObjList {
  Obj obj;
  size_t length;
  size_t capacity;
  Value *buffer;
} ObjList;

/* Unlike in Python, mtots tuples can only hold hashable items */
typedef struct ObjTuple {
  Obj obj;
  size_t length;
  size_t hash;
  Value *buffer;
} ObjTuple;

typedef struct ObjDict {
  Obj obj;
  Dict dict;
} ObjDict;

typedef struct NativeObjectDescriptor {
  void (*blacken)(ObjNative*);
  void (*free)(ObjNative*);
  ubool (*getField)(ObjNative*, ObjString*, Value*);
  ubool (*setField)(ObjNative*, ObjString*, Value);
  CFunction *instantiate;
  size_t objectSize;
  const char *name;

  /* optional, and should not be used as the source of truth
   * about a native object's methods, but sometimes convenient
   * to set here */
  CFunction **methods;

  /* Should be initialized as soon as the relevant native module
   * is loaded */
  ObjClass *klass;
} NativeObjectDescriptor;

struct ObjNative {
  Obj obj;
  NativeObjectDescriptor *descriptor;
};

typedef struct ObjFile {
  Obj obj;
  FILE *file;
  ubool isOpen;
  ObjString *name;
  FileMode mode;
} ObjFile;

typedef struct ObjUpvalue {
  Obj obj;
  Value *location;
  Value closed;
  struct ObjUpvalue *next;
} ObjUpvalue;

typedef struct ObjClosure {
  Obj obj;
  ObjInstance *module;
  ObjFunction *function;
  ObjUpvalue **upvalues;
  i16 upvalueCount;
} ObjClosure;

/* NOTE: ObjNativeClosure is a bit different from the other object
 * types, in that ObjNativeClosure is meant to be embedded,
 * with other data for the closure trailing the struct.
 * This means that we cannot actually know the allocation size
 * without actually looking at the 'size' field.
 */
typedef struct ObjNativeClosure {
  Obj obj;
  size_t size; /* Size of the allocated struct including ObjNativeClosure */
  ubool (*body)(void *it, i16 argCount, Value *args, Value *out);
  void (*blacken)(void *it);
  void (*free)(void *it);
  const char *name;
  i16 arity;
  i16 maxArity;
} ObjNativeClosure;

struct ObjClass {
  Obj obj;
  ObjString *name;
  Table methods;
  ubool isModuleClass;
  ubool isBuiltinClass;
  NativeObjectDescriptor *descriptor; /* NULL if not native */
};

struct ObjInstance {
  Obj obj;
  ObjClass *klass;
  Table fields;
};

ubool IS_MODULE(Value value);

ObjInstance *newModule(ObjString *name, ubool includeGlobals);
ObjInstance *newModuleFromCString(const char *name, ubool includeGlobals);
ObjClass *newClass(ObjString *name);
ObjClass *newClassFromCString(const char *name);
ObjClosure *newClosure(ObjFunction *function, ObjInstance *module);
ObjFunction *newFunction();
ObjNativeClosure *newNativeClosure(
  size_t structSize,
  ubool (*body)(void *it, i16 argCount, Value *args, Value *out),
  void (*blacken)(void *it),
  void (*free)(void *it),
  const char *name,
  i16 arity,
  i16 maxArity);
ObjInstance *newInstance(ObjClass *klass);
ObjString *takeString(char *chars, size_t length);
ObjString *copyString(const char *chars, size_t length);
ObjString *copyCString(const char *chars);
ObjByteArray *newByteArray(size_t size);
ObjByteArray *takeByteArray(u8 *buffer, size_t size);
ObjByteArray *copyByteArray(const u8 *buffer, size_t size);
ObjByteArrayView *newByteArrayView(
    size_t length, u8 *buffer, ObjByteArray *array);
ObjList *newList(size_t size);
ObjTuple *copyTuple(Value *buffer, size_t length);
ObjDict *newDict();
ObjFile *newFile(FILE *file, ubool isOpen, ObjString *name, FileMode mode);
ObjFile *openFile(const char *filename, FileMode mode);
ObjNative *newNative(NativeObjectDescriptor *descriptor, size_t objectSize);
ObjUpvalue *newUpvalue(Value *slot);
ObjClass *getClass(Value value);
void printObject(Value value);
const char *getObjectTypeName(ObjType type);

/* should-be-inline */ ubool isNative(
  Value value, NativeObjectDescriptor *descriptor);
/* should-be-inline */ ubool isObjType(Value value, ObjType type);

#endif/*mtots_object_h*/
