#include "mtots_vm.h"

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
    if (vm.bytesAllocated + getInternedStringsAllocationSize() > vm.nextGC) {
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
  printValue(OBJ_VAL_EXPLICIT(object));
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

void markString(String *string) {
  if (string) {
    string->isMarked = UTRUE;
  }
}

void markValue(Value value) {
  switch (value.type) {
    case VAL_STRING:
      markString(AS_STRING(value));
      break;
    case VAL_OBJ:
      markObject(AS_OBJ(value));
      break;
    default:
      break;
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
  printValue(OBJ_VAL_EXPLICIT(object));
  printf("\n");
#endif

  switch (object->type) {
    case OBJ_CLASS: {
      ObjClass *klass = (ObjClass*)object;
      markString(klass->name);
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
      markString(thunk->name);
      markString(thunk->moduleName);
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
    case OBJ_BUFFER:
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
      markString(file->name);
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
    case OBJ_BUFFER: {
      ObjBuffer *buffer = (ObjBuffer*)object;
      freeBuffer(&buffer->buffer);
      FREE(ObjBuffer, object);
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
  markString(vm.preludeString);
  markString(vm.initString);
  markString(vm.iterString);
  markString(vm.lenString);
  markString(vm.mulString);
  markString(vm.modString);
  markString(vm.containsString);
  markString(vm.nilString);
  markString(vm.trueString);
  markString(vm.falseString);
  markObject((Obj*)vm.sentinelClass);
  markObject((Obj*)vm.nilClass);
  markObject((Obj*)vm.boolClass);
  markObject((Obj*)vm.numberClass);
  markObject((Obj*)vm.stringClass);
  markObject((Obj*)vm.bufferClass);
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
  size_t before = vm.bytesAllocated + getInternedStringsAllocationSize();
  printf("-- gc begin\n");
#endif

  markRoots();
  traceReferences();
  freeUnmarkedStrings();
  mapRemoveWhite(&vm.tuples);
  sweep();

  vm.nextGC = (vm.bytesAllocated + getInternedStringsAllocationSize()) * GC_HEAP_GROW_FACTOR;

#if DEBUG_LOG_GC
  printf("-- gc end \n");
  printf(
    "   collected %zu bytes (from %zu to %zu) next at %zu\n",
    before - (vm.bytesAllocated + getInternedStringsAllocationSize()),
    before,
    vm.bytesAllocated + getInternedStringsAllocationSize(),
    vm.nextGC);
#endif
}
