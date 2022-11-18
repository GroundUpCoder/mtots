#ifndef mtots_vm_h
#define mtots_vm_h

#include "mtots_chunk.h"
#include "mtots_object.h"
#include "mtots_ops.h"

/* headers with prototypes whose function definitions are in mtots_vm.c */
#include "mtots_stack.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * U8_COUNT)
#define TRY_SNAPSHOTS_MAX 64
#define MAX_ERROR_STRING_LENGTH 2048

typedef struct CallFrame {
  ObjClosure *closure;
  u8 *ip;
  Value *slots;
} CallFrame;

typedef struct TrySnapshot {
  u8 *ip;
  Value *stackTop;
  i16 frameCount;
} TrySnapshot;

typedef struct VM {
  CallFrame frames[FRAMES_MAX];
  i16 frameCount;
  Value stack[STACK_MAX];
  Value *stackTop;
  TrySnapshot trySnapshots[TRY_SNAPSHOTS_MAX];
  i16 trySnapshotsCount;
  Map globals;
  Map modules;            /* all preloaded modules */
  Map nativeModuleThunks; /* Map of CFunctions */
  Map tuples;              /* table of all interned tuples */

  String *preludeString;
  String *initString;
  String *iterString;
  String *lenString;
  String *mulString;
  String *modString;
  String *containsString;
  String *nilString;
  String *trueString;
  String *falseString;

  ObjClass *sentinelClass;
  ObjClass *nilClass;
  ObjClass *boolClass;
  ObjClass *numberClass;
  ObjClass *stringClass;
  ObjClass *byteArrayClass;
  ObjClass *byteArrayViewClass;
  ObjClass *listClass;
  ObjClass *tupleClass;
  ObjClass *mapClass;
  ObjClass *functionClass;
  ObjClass *operatorClass;
  ObjClass *classClass;
  ObjClass *fileClass;

  ObjFile *stdinFile;
  ObjFile *stdoutFile;
  ObjFile *stderrFile;

  ObjUpvalue *openUpvalues;

  size_t bytesAllocated;
  size_t nextGC;
  Obj *objects;
  size_t grayCount;
  size_t grayCapacity;
  Obj **grayStack;

  char *errorString;
} VM;

extern VM vm;

void initVM();
void freeVM();
ubool interpret(const char *source, ObjInstance *module);
void defineGlobal(const char *name, Value value);

/* NOTE: Deprecated. Use addNativeModuleCFunc instead.
 *
 * Native module bodies should be a CFunction that accepts
 * Exactly one argument, the module
 */
void addNativeModule(CFunction *func);

/* semi-private functions */
ubool run();
ubool call(ObjClosure *closure, i16 argCount);

#endif/*mtots_vm_h*/
