#include "mtots_memory.h"
#include "mtots_vm.h"
#include "mtots_object.h"
#include "mtots_compiler.h"

#include <stdlib.h>
#include <stdio.h>

#if DEBUG_LOG_GC
#include <stdio.h>
#include "mtots_debug.h"
#endif

#define GC_HEAP_GROW_FACTOR 2

void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
  void *result;

  vm.bytesAllocated += newSize - oldSize;
  if (newSize > oldSize) {
#if DEBUG_STRESS_GC
  collectGarbage();
#endif
    if (vm.bytesAllocated > vm.nextGC) {
      collectGarbage();
    }
  }

  if (newSize == 0) {
    free(pointer);
    return NULL;
  }

  result = realloc(pointer, newSize);
  if (result == NULL) {
    panic("out of memory");
  }
  return result;
}

void markObject(Obj *object) {
  if (object == NULL || object->isMarked) {
    return;
  }
#if DEBUG_LOG_GC
  printf("%p mark ", (void*) object);
  printValue(OBJ_VAL(object));
  printf("\n");
#endif
  object->isMarked = UTRUE;

  if (vm.grayCapacity < vm.grayCount + 1) {
    vm.grayCapacity = GROW_CAPACITY(vm.grayCapacity);
    vm.grayStack = (Obj**)realloc(
      vm.grayStack, sizeof(Obj*) * vm.grayCapacity);
    if (vm.grayStack == NULL) {
      panic("out of memory (during gc)");
    }
  }
  vm.grayStack[vm.grayCount++] = object;
}

void markValue(Value value) {
  if (IS_OBJ(value)) {
    markObject(AS_OBJ(value));
  }
}

static void markArray(ValueArray *array) {
  size_t i;
  for (i = 0; i < array->count; i++) {
    markValue(array->values[i]);
  }
}

static void blackenObject(Obj *object) {
#if DEBUG_LOG_GC
  printf("%p blacken ", (void*)object);
  printValue(OBJ_VAL(object));
  printf("\n");
#endif

  switch (object->type) {
    case OBJ_CLASS: {
      ObjClass *klass = (ObjClass*)object;
      markObject((Obj*)klass->name);
      markMap(&klass->methods);
      break;
    }
    case OBJ_CLOSURE: {
      i16 i;
      ObjClosure *closure = (ObjClosure*)object;
      markObject((Obj*)closure->module);
      markObject((Obj*)closure->thunk);
      for (i = 0; i < closure->upvalueCount; i++) {
        markObject((Obj*)closure->upvalues[i]);
      }
      break;
    }
    case OBJ_THUNK: {
      ObjThunk *thunk = (ObjThunk*) object;
      size_t i;
      markObject((Obj*)thunk->name);
      markObject((Obj*)thunk->moduleName);
      markArray(&thunk->chunk.constants);
      for (i = 0; i < thunk->defaultArgsCount; i++) {
        markValue(thunk->defaultArgs[i]);
      }
      break;
    }
    case OBJ_NATIVE_CLOSURE: {
      ObjNativeClosure *nc = (ObjNativeClosure*)object;
      if (nc->blacken != NULL) {
        nc->blacken(nc);
      }
      break;
    }
    case OBJ_INSTANCE: {
      ObjInstance *instance = (ObjInstance*)object;
      markObject((Obj*)instance->klass);
      markMap(&instance->fields);
      break;
    }
    case OBJ_UPVALUE:
      markValue(((ObjUpvalue*)object)->closed);
      break;
    case OBJ_STRING:
    case OBJ_BYTE_ARRAY:
      break;
    case OBJ_BYTE_ARRAY_VIEW:
      markObject((Obj*)((ObjByteArrayView*)object)->array);
      break;
    case OBJ_LIST: {
      ObjList *list = (ObjList*)object;
      size_t i;
      for (i = 0; i < list->length; i++) {
        markValue(list->buffer[i]);
      }
      break;
    }
    case OBJ_TUPLE: {
      ObjTuple *tuple = (ObjTuple*)object;
      size_t i;
      for (i = 0; i < tuple->length; i++) {
        markValue(tuple->buffer[i]);
      }
      break;
    }
    case OBJ_DICT: {
      ObjDict *dict = (ObjDict*)object;
      markMap(&dict->dict);
      break;
    }
    case OBJ_FILE: {
      ObjFile *file = (ObjFile*)object;
      markObject((Obj*)(file->name));
      break;
    }
    case OBJ_NATIVE: {
      ObjNative *n = (ObjNative*)object;
      n->descriptor->blacken(n);
      break;
    }
    default:
      abort();
  }
}

static void freeObject(Obj *object) {
#if DEBUG_LOG_GC
  printf("%p free type %s\n", (void*) object, getObjectTypeName(object->type));
#endif

  switch (object->type) {
    case OBJ_CLASS: {
      ObjClass *klass = (ObjClass*)object;
      freeMap(&klass->methods);
      FREE(ObjClass, object);
      break;
    }
    case OBJ_CLOSURE: {
      ObjClosure *closure = (ObjClosure*) object;
      FREE_ARRAY(ObjUpvalue*, closure->upvalues, closure->upvalueCount);
      FREE(ObjClosure, object);
      break;
    }
    case OBJ_THUNK: {
      ObjThunk *thunk = (ObjThunk*) object;
      freeChunk(&thunk->chunk);
      FREE_ARRAY(Value, thunk->defaultArgs, thunk->defaultArgsCount);
      FREE(ObjThunk, object);
      break;
    }
    case OBJ_NATIVE_CLOSURE: {
      ObjNativeClosure *nc = (ObjNativeClosure*)object;

      if (nc->free != NULL) {
        nc->free(nc);
      }

      /* We cannot use the FREE macro with ObjNativeClosure
       * because the actual allocation size is likely larger with
       * some trailing data.
       * So we have to call reallocate directly
       */
      reallocate(object, nc->size, 0);
      break;
    }
    case OBJ_INSTANCE: {
      ObjInstance *instance = (ObjInstance*)object;
      freeMap(&instance->fields);
      FREE(ObjInstance, object);
      break;
    }
    case OBJ_STRING: {
      ObjString *string = (ObjString*)object;
      FREE_ARRAY(char, string->chars, string->length + 1);
      FREE(ObjString, object);
      break;
    }
    case OBJ_BYTE_ARRAY: {
      ObjByteArray *byteArray = (ObjByteArray*)object;
      FREE_ARRAY(u8, byteArray->buffer, byteArray->length);
      FREE(ObjByteArray, object);
      break;
    }
    case OBJ_BYTE_ARRAY_VIEW: {
      FREE(ObjByteArrayView, object);
      break;
    }
    case OBJ_LIST: {
      ObjList *list = (ObjList*)object;
      FREE_ARRAY(Value, list->buffer, list->capacity);
      FREE(ObjList, object);
      break;
    }
    case OBJ_TUPLE: {
      ObjTuple *tuple = (ObjTuple*)object;
      FREE_ARRAY(Value, tuple->buffer, tuple->length);
      FREE(ObjTuple, object);
      break;
    }
    case OBJ_DICT: {
      ObjDict *dict = (ObjDict*)object;
      freeMap(&dict->dict);
      FREE(ObjDict, object);
      break;
    }
    case OBJ_FILE: {
      FREE(ObjFile, object);
      break;
    }
    case OBJ_NATIVE: {
      ObjNative *n = (ObjNative*)object;
      n->descriptor->free(n);
      reallocate(object, n->descriptor->objectSize, 0);
      break;
    }
    case OBJ_UPVALUE:
      FREE(ObjUpvalue, object);
      break;
    default:
      abort();
  }
}

static void markRoots() {
  Value *slot;
  i16 i;
  ObjUpvalue *upvalue;
  for (slot = vm.stack; slot < vm.stackTop; slot++) {
    markValue(*slot);
  }

  for (i = 0; i < vm.frameCount; i++) {
    markObject((Obj*)vm.frames[i].closure);
  }

  for (upvalue = vm.openUpvalues;
      upvalue != NULL;
      upvalue = upvalue->next) {
    markObject((Obj*)upvalue);
  }

  markMap(&vm.globals);
  markMap(&vm.modules);
  markMap(&vm.nativeModuleThunks);
  markCompilerRoots();
  markObject((Obj*)vm.preludeString);
  markObject((Obj*)vm.initString);
  markObject((Obj*)vm.iterString);
  markObject((Obj*)vm.lenString);
  markObject((Obj*)vm.mulString);
  markObject((Obj*)vm.modString);
  markObject((Obj*)vm.containsString);
  markObject((Obj*)vm.nilString);
  markObject((Obj*)vm.trueString);
  markObject((Obj*)vm.falseString);
  markObject((Obj*)vm.sentinelClass);
  markObject((Obj*)vm.nilClass);
  markObject((Obj*)vm.boolClass);
  markObject((Obj*)vm.numberClass);
  markObject((Obj*)vm.stringClass);
  markObject((Obj*)vm.byteArrayClass);
  markObject((Obj*)vm.byteArrayViewClass);
  markObject((Obj*)vm.listClass);
  markObject((Obj*)vm.tupleClass);
  markObject((Obj*)vm.mapClass);
  markObject((Obj*)vm.functionClass);
  markObject((Obj*)vm.operatorClass);
  markObject((Obj*)vm.classClass);
  markObject((Obj*)vm.fileClass);
  markObject((Obj*)vm.stdinFile);
  markObject((Obj*)vm.stdoutFile);
  markObject((Obj*)vm.stderrFile);
}

static void traceReferences() {
  while (vm.grayCount > 0) {
    Obj *object = vm.grayStack[--vm.grayCount];
    blackenObject(object);
  }
}

static void sweep() {
  Obj *previous = NULL;
  Obj *object = vm.objects;
  while (object != NULL) {
    if (object->isMarked) {
      object->isMarked = UFALSE;
      previous = object;
      object = object->next;
    } else {
      Obj *unreached = object;
      object = object->next;
      if (previous != NULL) {
        previous->next = object;
      } else {
        vm.objects = object;
      }
      freeObject(unreached);
    }
  }
}

void freeObjects() {
  Obj *object = vm.objects;
  while (object != NULL) {
    Obj *next = object->next;
    freeObject(object);
    object = next;
  }

  free(vm.grayStack);
}

void collectGarbage() {
#if DEBUG_LOG_GC
  size_t before = vm.bytesAllocated;
  printf("-- gc begin\n");
#endif

  markRoots();
  traceReferences();
  mapRemoveWhite(&vm.strings);
  mapRemoveWhite(&vm.tuples);
  sweep();

  vm.nextGC = vm.bytesAllocated * GC_HEAP_GROW_FACTOR;

#if DEBUG_LOG_GC
  printf("-- gc end \n");
  printf(
    "   collected %zu bytes (from %zu to %zu) next at %zu\n",
    before - vm.bytesAllocated, before, vm.bytesAllocated,
    vm.nextGC);
#endif
}
