#ifndef mtots_vm_h
#define mtots_vm_h

#include "mtots_chunk.h"
#include "mtots_value.h"
#include "mtots_table.h"
#include "mtots_object.h"
#include "mtots_ops.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * U8_COUNT)

typedef struct {
  ObjClosure *closure;
  u8 *ip;
  Value *slots;
} CallFrame;

typedef struct {
  CallFrame frames[FRAMES_MAX];
  i16 frameCount;
  Value stack[STACK_MAX];
  Value *stackTop;
  Table globals;
  Table strings;            /* table of all interned strings */
  Table modules;            /* all preloaded modules */
  Table nativeModuleThunks; /* Table of CFunctions */

  ObjString *initString;
  ObjString *iterString;
  ObjString *lenString;
  ObjString *mulString;
  ObjString *modString;
  ObjString *nilString;
  ObjString *trueString;
  ObjString *falseString;

  ObjClass *sentinelClass;
  ObjClass *nilClass;
  ObjClass *boolClass;
  ObjClass *numberClass;
  ObjClass *stringClass;
  ObjClass *byteArrayClass;
  ObjClass *listClass;
  ObjClass *dictClass;
  ObjClass *functionClass;
  ObjClass *operatorClass;
  ObjClass *classClass;
  ObjClass *fileClass;

  /* Class to use when we want to an instance as
   * a bag of values */
  ObjClass *tableClass;

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
} VM;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

extern VM vm;

void initVM();
void freeVM();
InterpretResult interpret(const char *source, ObjInstance *module);
void push(Value value);
Value pop();
NORETURN void panic(const char *format, ...);
void runtimeError(const char *format, ...);
void defineGlobal(const char *name, Value value);

/* Native module bodies should be a CFunction that accepts
 * Exactly one argument, the module
 */
void addNativeModule(CFunction *func);

/* semi-private functions */
InterpretResult run(i16 returnFrameCount);
ubool call(ObjClosure *closure, i16 argCount);

#endif/*mtots_vm_h*/
