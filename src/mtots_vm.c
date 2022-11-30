#include "mtots_assumptions.h"
#include "mtots_vm.h"
#include "mtots_globals.h"
#include "mtots_class_list.h"
#include "mtots_class_file.h"
#include "mtots_class_str.h"
#include "mtots_class_dict.h"
#include "mtots_class_ba.h"
#include "mtots_class_class.h"
#include "mtots_modules.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <math.h>

VM vm;

static ubool invoke(String *name, i16 argCount);
static void prepPrelude();

static void resetStack() {
  vm.stackTop = vm.stack;
  vm.frameCount = 0;
  vm.openUpvalues = NULL;
  vm.trySnapshotsCount = 0;
}

static void printStackToStringBuffer(StringBuffer *sb) {
  i16 i;
  for (i = vm.frameCount - 1; i >=0; i--) {
    CallFrame *frame = &vm.frames[i];
    ObjThunk *thunk = frame->closure->thunk;
    size_t instruction = frame->ip - thunk->chunk.code - 1;
    sbprintf(
      sb, "[line %d] in ", thunk->chunk.lines[instruction]);
    if (thunk->name == NULL) {
      if (thunk->moduleName == NULL) {
        sbprintf(sb, "[script]\n");
      } else {
        sbprintf(sb, "%s\n", thunk->moduleName->chars);
      }
    } else {
      if (thunk->moduleName == NULL) {
        sbprintf(sb, "%s()\n", thunk->name->chars);
      } else {
        sbprintf(sb, "%s:%s()\n",
          thunk->moduleName->chars, thunk->name->chars);
      }
    }
  }
}

void defineGlobal(const char *name, Value value) {
  push(STRING_VAL(internString(name, strlen(name))));
  push(value);
  mapSetStr(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
  pop();
  pop();
}

void addNativeModule(CFunction *func) {
  String *name;
  if (func->arity != 1) {
    panic("Native modules must accept 1 argument but got %d", func->arity);
  }
  name = internCString(func->name);
  push(STRING_VAL(name));
  if (!mapSetStr(&vm.nativeModuleThunks, name, CFUNCTION_VAL(func))) {
    panic("Native module %s is already defined", name->chars);
  }

  pop(); /* name */
}

static void initNoMethodClass(ObjClass **clsptr, const char *name) {
  String *tmpstr;

  tmpstr = internCString(name);
  push(STRING_VAL(tmpstr));
  *clsptr = newClass(tmpstr);
  (*clsptr)->isBuiltinClass = UTRUE;
  pop(); /* tmpstr */
}

void initVM() {
  setErrorContextProvider(printStackToStringBuffer);
  checkAssumptions();
  initParseRules();
  resetStack();
  vm.objects = NULL;
  vm.bytesAllocated = 0;
  vm.nextGC = 1024 * 1024;

  vm.grayCount = 0;
  vm.grayCapacity = 0;
  vm.grayStack = NULL;

  vm.preludeString = NULL;
  vm.initString = NULL;
  vm.iterString = NULL;
  vm.lenString = NULL;
  vm.mulString = NULL;
  vm.modString = NULL;
  vm.containsString = NULL;
  vm.nilString = NULL;
  vm.trueString = NULL;
  vm.falseString = NULL;
  vm.sentinelClass = NULL;
  vm.nilClass = NULL;
  vm.boolClass = NULL;
  vm.numberClass = NULL;
  vm.stringClass = NULL;
  vm.byteArrayClass = NULL;
  vm.byteArrayViewClass = NULL;
  vm.listClass = NULL;
  vm.tupleClass = NULL;
  vm.mapClass = NULL;
  vm.functionClass = NULL;
  vm.operatorClass = NULL;
  vm.classClass = NULL;
  vm.fileClass = NULL;
  vm.stdinFile = NULL;
  vm.stdoutFile = NULL;
  vm.stderrFile = NULL;

  initMap(&vm.globals);
  initMap(&vm.modules);
  initMap(&vm.nativeModuleThunks);
  initMap(&vm.tuples);

  vm.preludeString = internCString("__prelude__");
  vm.initString = internCString("__init__");
  vm.iterString = internCString("__iter__");
  vm.lenString = internCString("__len__");
  vm.mulString = internCString("__mul__");
  vm.modString = internCString("__mod__");
  vm.containsString = internCString("__contains__");
  vm.nilString = internCString("nil");
  vm.trueString = internCString("true");
  vm.falseString = internCString("false");

  initNoMethodClass(&vm.sentinelClass, "Sentinel");
  initNoMethodClass(&vm.nilClass, "Nil");
  initNoMethodClass(&vm.boolClass, "Bool");
  initNoMethodClass(&vm.numberClass, "Number");
  initStringClass();
  initByteArrayClass();
  initByteArrayViewClass();
  initListClass();
  initNoMethodClass(&vm.tupleClass, "Tuple");
  initDictClass();
  initNoMethodClass(&vm.functionClass, "Function");
  initNoMethodClass(&vm.operatorClass, "Operator");
  initClassClass();
  initFileClass();

  defineDefaultGlobals();
  addNativeModules();

  prepPrelude();
}

void freeVM() {
  freeMap(&vm.globals);
  freeMap(&vm.modules);
  freeMap(&vm.nativeModuleThunks);
  freeMap(&vm.tuples);
  vm.preludeString = NULL;
  vm.initString = NULL;
  vm.iterString = NULL;
  vm.lenString = NULL;
  vm.mulString = NULL;
  vm.modString = NULL;
  vm.containsString = NULL;
  vm.nilString = NULL;
  vm.trueString = NULL;
  vm.falseString = NULL;
  freeObjects();
}

void push(Value value) {
  if (vm.stackTop + 1 > vm.stack + STACK_MAX) {
    panic("stack overflow");
  }
  *vm.stackTop = value;
  vm.stackTop++;
}

Value pop() {
  if (vm.stackTop <= vm.stack) {
    panic("stack underflow");
  }
  vm.stackTop--;
  return *vm.stackTop;
}

static Value peek(int distance) {
  return vm.stackTop[-1 - distance];
}

static ubool isIterator(Value value) {
  if (IS_OBJ(value)) {
    switch (AS_OBJ(value)->type) {
      case OBJ_NATIVE_CLOSURE: return AS_NATIVE_CLOSURE(value)->arity == 0;
      case OBJ_CLOSURE: return AS_CLOSURE(value)->thunk->arity == 0;
      default: break;
    }
  }
  return UFALSE;
}

static ubool callCFunction(CFunction *cfunc, i16 argCount) {
  Value result = NIL_VAL(), *argsStart;
  ubool status;
  if (cfunc->arity != argCount) {
    /* not an exact match for the arity
     * We need further checks */
    if (cfunc->maxArity) {
      /* we have optional args */
      if (argCount < cfunc->arity) {
        runtimeError(
          "Function %s expects at least %d arguments but got %d",
          cfunc->name, cfunc->arity, argCount);
        return UFALSE;
      } else if (argCount > cfunc->maxArity) {
        runtimeError(
          "Function %s expects at most %d arguments but got %d",
          cfunc->name, cfunc->maxArity, argCount);
        return UFALSE;
      }
      /* At this point we have argCount between
       * cfunc->arity and cfunc->maxArity */
    } else {
      runtimeError(
        "Function %s expects %d arguments but got %d",
        cfunc->name, cfunc->arity, argCount);
      return UFALSE;
    }
  }
  argsStart = vm.stackTop - argCount;
  /* NOTE: Every call always has a value in the receiver slot -
   * In particular, normal function calls will have the function
   * itself in the receiver slot */
  if (!typePatternMatch(cfunc->receiverType, argsStart[-1])) {
    runtimeError(
      "Invalid receiver passed to method %s()",
      cfunc->name);
    return UFALSE;
  }
  if (cfunc->argTypes != NULL) {
    size_t i;
    for (i = 0; i < argCount; i++) {
      if (!typePatternMatch(cfunc->argTypes[i], argsStart[i])) {
        runtimeError(
          "%s() expects %s for argument %d, but got %s",
          cfunc->name, getTypePatternName(cfunc->argTypes[i]),
          i, getKindName(argsStart[i]));
        return UFALSE;
      }
    }
  }
  status = cfunc->body(argCount, argsStart, &result);
  if (!status) {
    return UFALSE;
  }
  vm.stackTop -= argCount + 1;
  push(result);
  return UTRUE;
}

static ubool callNativeClosure(ObjNativeClosure *nc, i16 argCount) {
  Value result = NIL_VAL();
  ubool status;
  if (nc->arity != argCount) {
    /* not an exact match for the arity
     * We need further checks */
    if (nc->maxArity) {
      /* we have optional args */
      if (argCount < nc->arity) {
        runtimeError(
          "Function %s expects at least %d arguments but got %d",
          nc->name, nc->arity, argCount);
        return UFALSE;
      } else if (argCount > nc->maxArity) {
        runtimeError(
          "Function %s expects at most %d arguments but got %d",
          nc->name, nc->maxArity, argCount);
        return UFALSE;
      }
      /* At this point we have argCount between
       * nc->arity and nc->maxArity */
    } else {
      runtimeError(
        "Function %s expects %d arguments but got %d",
        nc->name, nc->arity, argCount);
      return UFALSE;
    }
  }
  status = nc->body(nc, argCount, vm.stackTop - argCount, &result);
  if (!status) {
    return UFALSE;
  }
  vm.stackTop -= argCount + 1;
  push(result);
  return UTRUE;
}

ubool call(ObjClosure *closure, i16 argCount) {
  CallFrame *frame;

  if (argCount < closure->thunk->arity &&
      argCount + closure->thunk->defaultArgsCount >=
        closure->thunk->arity) {
    size_t i = 0;
    while (argCount < closure->thunk->arity) {
      push(closure->thunk->defaultArgs[i]);
      i++;
      argCount++;
    }
  }

  if (argCount != closure->thunk->arity) {
    runtimeError(
      "Expected %d arguments but got %d",
      closure->thunk->arity, argCount);
    return UFALSE;
  }

  if (vm.frameCount == FRAMES_MAX) {
    runtimeError("Stack overflow");
    return UFALSE;
  }

  frame = &vm.frames[vm.frameCount++];
  frame->closure = closure;
  frame->ip = closure->thunk->chunk.code;
  frame->slots = vm.stackTop - argCount - 1;
  return UTRUE;
}

static ubool callByteArrayClass(i16 argCount) {
  Value arg;
  if (argCount != 1) {
    runtimeError("ByteArray() requires exactly one argument");
    return UFALSE;
  }
  arg = peek(0);
  if (IS_NUMBER(arg)) {
    size_t size = AS_NUMBER(arg);
    ObjByteArray *ba = newByteArray(size);
    pop(); /* arg */
    pop(); /* ByeArray class */
    push(BYTE_ARRAY_VAL(ba));
    return UTRUE;
  }
  if (IS_BYTE_ARRAY(arg)) {
    ObjByteArray *otherBA = AS_BYTE_ARRAY(arg);
    ObjByteArray *ba = copyByteArray(otherBA->buffer, otherBA->length);
    pop(); /* arg */
    pop(); /* ByeArray class */
    push(BYTE_ARRAY_VAL(ba));
    return UTRUE;
  }
  if (IS_STRING(arg)) {
    String *str = AS_STRING(arg);
    ObjByteArray *ba = copyByteArray(
      (const u8*)str->chars, str->length);
    pop(); /* arg */
    pop(); /* ByeArray class */
    push(BYTE_ARRAY_VAL(ba));
    return UTRUE;
  }
  if (IS_LIST(arg)) {
    ObjList *list = AS_LIST(arg);
    size_t len = list->length, i;
    u8 *buffer = ALLOCATE(u8, len);
    for (i = 0; i < len; i++) {
      Value item = list->buffer[i];
      u8 b;
      if (!IS_NUMBER(item)) {
        runtimeError(
          "ByteArray() requires a list of numbers, "
          "but got list item %s", getKindName(item));
        return UFALSE;
      }
      b = AS_NUMBER(item);
      buffer[i] = b;
    }
    pop(); /* arg */
    pop(); /* ByeArray class */
    push(BYTE_ARRAY_VAL(takeByteArray(buffer, len)));
    return UTRUE;
  }
  runtimeError(
    "ByteArray() expects a number, string or list argument "
    "but got %s", getKindName(arg));
  return UFALSE;
}

static ubool callClass(ObjClass *klass, i16 argCount) {
  Value initializer;

  if (klass->descriptor) {
    /* native class */
    if (klass->descriptor->instantiate) {
      return callCFunction(klass->descriptor->instantiate, argCount);
    } else {
      runtimeError(
        "Native class %s does not allow instantiation",
        klass->descriptor->name);
      return UFALSE;
    }
  } else if (klass->isBuiltinClass) {
    /* builtin class */
    if (klass == vm.byteArrayClass) {
      return callByteArrayClass(argCount);
    }
    runtimeError("Builtin class %s does not allow instantiation",
      klass->name->chars);
    return UFALSE;
  } else if (klass->isModuleClass) {
    /* module */
    runtimeError("Instantiating module classes is not allowed");
    return UFALSE;
  } else {
    /* normal classes */
    vm.stackTop[-argCount - 1] = INSTANCE_VAL(newInstance(klass));
    if (mapGetStr(&klass->methods, vm.initString, &initializer)) {
      return call(AS_CLOSURE(initializer), argCount);
    } else if (argCount != 0) {
      runtimeError("Expected 0 arguments but got %d", argCount);
      return UFALSE;
    }
    return UTRUE;
  }
}

static ubool callOperator(Operator op, i16 argCount) {
  switch (op) {
    case OperatorLen: {
      Value receiver = vm.stackTop[-1];
      if (argCount != 1) {
        runtimeError("len() requires 1 argument but got %d", (int) argCount);
        return UFALSE;
      }
      vm.stackTop--;
      vm.stackTop[-1] = receiver;

      if (IS_STRING(receiver)) {
        vm.stackTop[-1] = NUMBER_VAL(AS_STRING(receiver)->length);
        return UTRUE;
      } else if (IS_OBJ(receiver)) {
        switch (AS_OBJ(receiver)->type) {
          case OBJ_BYTE_ARRAY:
            vm.stackTop[-1] = NUMBER_VAL(AS_BYTE_ARRAY(receiver)->length);
          case OBJ_LIST:
            vm.stackTop[-1] = NUMBER_VAL(AS_LIST(receiver)->length);
            return UTRUE;
          case OBJ_TUPLE:
            vm.stackTop[-1] = NUMBER_VAL(AS_TUPLE(receiver)->length);
            return UTRUE;
          case OBJ_DICT:
            vm.stackTop[-1] = NUMBER_VAL(AS_DICT(receiver)->dict.size);
            return UTRUE;
          default:
            return invoke(vm.lenString, 0);
        }
      }

      runtimeError(
        "object of kind '%s' has no len()",
        getKindName(receiver));
      return UFALSE;
    }
  }
  runtimeError("Unrecognized operator %d", op);
  return UFALSE;
}

static ubool callValue(Value callee, i16 argCount) {
  if (IS_CFUNCTION(callee)) {
    CFunction *cfunc = AS_CFUNCTION(callee);
    return callCFunction(cfunc, argCount);
  } else if (IS_OBJ(callee)) {
    switch (OBJ_TYPE(callee)) {
      case OBJ_CLASS:
        return callClass(AS_CLASS(callee), argCount);
      case OBJ_CLOSURE:
        return call(AS_CLOSURE(callee), argCount);
      case OBJ_NATIVE_CLOSURE:
        return callNativeClosure(AS_NATIVE_CLOSURE(callee), argCount);
      default:
        break; /* Non-callable object type */
    }
  } else if (IS_OPERATOR(callee)) {
    return callOperator(AS_OPERATOR(callee), argCount);
  }
  runtimeError(
    "Can only call functions and classes but got %s", getKindName(callee));
  return UFALSE;
}

static ubool invokeFromClass(
    ObjClass *klass, String *name, i16 argCount) {
  Value method;
  if (!mapGetStr(&klass->methods, name, &method)) {
    runtimeError(
      "Method '%s' not found in '%s'",
      name->chars,
      klass->name->chars);
    return UFALSE;
  }
  return callValue(method, argCount);
}

static ubool invoke(String *name, i16 argCount) {
  ObjClass *klass;
  Value receiver = peek(argCount);

  klass = getClassOfValue(receiver);
  if (klass == NULL) {
    runtimeError(
      "%s kind does not yet support method calls", getKindName(receiver));
    return UFALSE;
  }

  return invokeFromClass(klass, name, argCount);
}

static ObjUpvalue *captureUpvalue(Value *local) {
  ObjUpvalue *prevUpvalue = NULL;
  ObjUpvalue *upvalue = vm.openUpvalues;
  ObjUpvalue *createdUpvalue;

  while (upvalue != NULL && upvalue->location > local) {
    prevUpvalue = upvalue;
    upvalue = upvalue->next;
  }

  if (upvalue != NULL && upvalue->location == local) {
    return upvalue;
  }

  createdUpvalue = newUpvalue(local);
  createdUpvalue->next = upvalue;
  if (prevUpvalue == NULL) {
    vm.openUpvalues = createdUpvalue;
  } else {
    prevUpvalue->next = createdUpvalue;
  }
  return createdUpvalue;
}

static void closeUpvalues(Value *last) {
  while (vm.openUpvalues != NULL &&
      vm.openUpvalues->location >= last) {
    ObjUpvalue *upvalue = vm.openUpvalues;
    upvalue->closed = *upvalue->location;
    upvalue->location = &upvalue->closed;
    vm.openUpvalues = upvalue->next;
  }
}

static void defineMethod(String *name) {
  Value method = peek(0);
  ObjClass *klass = AS_CLASS(peek(1));
  mapSetStr(&klass->methods, name, method);
  pop();
}

static ubool isFalsey(Value value) {
  return IS_NIL(value) ||
    (IS_BOOL(value) && !AS_BOOL(value)) ||
    (IS_NUMBER(value) && AS_NUMBER(value) == 0);
}

static void concatenate() {
  String *result;
  String *b = AS_STRING(peek(0));
  String *a = AS_STRING(peek(1));
  size_t length = a->length + b->length;
  char *chars = malloc(sizeof(char) * (length + 1));
  memcpy(chars, a->chars, a->length);
  memcpy(chars + a->length, b->chars, b->length);
  chars[length] = '\0';
  result = internOwnedString(chars, length);
  pop();
  pop();
  push(STRING_VAL(result));
}

ubool run() {
  i16 returnFrameCount = vm.frameCount - 1;
  CallFrame *frame = &vm.frames[vm.frameCount - 1];

#define READ_BYTE() (*frame->ip++)
#define READ_SHORT() \
  (frame->ip += 2, (u16)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_CONSTANT() \
  (frame->closure->thunk->chunk.constants.values[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define RETURN_RUNTIME_ERROR() \
  do { \
    TrySnapshot *snap; \
    if (vm.trySnapshotsCount == 0) return UFALSE; \
    snap = &vm.trySnapshots[--vm.trySnapshotsCount]; \
    vm.stackTop = snap->stackTop; \
    vm.frameCount = snap->frameCount; \
    frame = &vm.frames[vm.frameCount - 1]; \
    frame->ip = snap->ip; \
    clearErrorString(); \
    goto loop; \
  } while(0)
#define BINARY_OP(valueType, op) \
  do { \
    if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
      runtimeError("Operands must be numbers"); \
      RETURN_RUNTIME_ERROR(); \
    } \
    { \
      double b = AS_NUMBER(pop()); \
      double a = AS_NUMBER(pop()); \
      push(valueType(a op b)); \
    } \
  } while (0)
#define BINARY_BITWISE_OP(op) \
  do { \
    if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
      runtimeError("Operands must be numbers"); \
      RETURN_RUNTIME_ERROR(); \
    } \
    { \
      u32 b = AS_U32(pop()); \
      u32 a = AS_U32(pop()); \
      push(NUMBER_VAL(a op b)); \
    } \
  } while (0)

  for(;;) {
    u8 instruction;

#if DEBUG_TRACE_EXECUTION
    Value *slot;
loop:
    printf("          ");
    for (slot = vm.stack; slot < vm.stackTop; slot++) {
      printf("[ ");
      printValue(*slot);
      printf(" ]");
    }
    printf("\n");
    disassembleInstruction(
      &frame->closure->thunk->chunk,
      (int)(frame->ip - frame->closure->thunk->chunk.code));
#else
loop:
#endif

    switch (instruction = READ_BYTE()) {
      case OP_CONSTANT: {
        Value constant = READ_CONSTANT();
        push(constant);
        break;
      }
      case OP_NIL: push(NIL_VAL()); break;
      case OP_TRUE: push(BOOL_VAL(1)); break;
      case OP_FALSE: push(BOOL_VAL(0)); break;
      case OP_POP: pop(); break;
      case OP_GET_LOCAL: {
        u8 slot = READ_BYTE();
        push(frame->slots[slot]);
        break;
      }
      case OP_SET_LOCAL: {
        u8 slot = READ_BYTE();
        frame->slots[slot] = peek(0);
        break;
      }
      case OP_GET_GLOBAL: {
        String *name = READ_STRING();
        Value value;
        if (!mapGetStr(&frame->closure->module->fields, name, &value)) {
          runtimeError("Undefined variable '%s'", name->chars);
          RETURN_RUNTIME_ERROR();
        }
        push(value);
        break;
      }
      case OP_DEFINE_GLOBAL: {
        String *name = READ_STRING();
        mapSetStr(&frame->closure->module->fields, name, peek(0));
        pop();
        break;
      }
      case OP_SET_GLOBAL: {
        String *name = READ_STRING();
        if (mapSetStr(&frame->closure->module->fields, name, peek(0))) {
          mapDeleteStr(&frame->closure->module->fields, name);
          runtimeError("Undefined variable '%s'", name->chars);
          RETURN_RUNTIME_ERROR();
        }
        break;
      }
      case OP_GET_UPVALUE: {
        u8 slot = READ_BYTE();
        push(*frame->closure->upvalues[slot]->location);
        break;
      }
      case OP_SET_UPVALUE: {
        u8 slot = READ_BYTE();
        *frame->closure->upvalues[slot]->location = peek(0);
        break;
      }
      case OP_GET_FIELD: {
        String *name;
        Value value = NIL_VAL();

        if (IS_INSTANCE(peek(0))) {
          ObjInstance *instance;
          instance = AS_INSTANCE(peek(0));
          name = READ_STRING();
          if (mapGetStr(&instance->fields, name, &value)) {
            pop(); /* Instance */
            push(value);
            break;
          }
          runtimeError(
            "Field '%s' not found in %s",
            name->chars, instance->klass->name->chars);
          RETURN_RUNTIME_ERROR();
        }

        if (IS_DICT(peek(0))) {
          ObjDict *d = AS_DICT(peek(0));
          name = READ_STRING();
          if (mapGet(&d->dict, STRING_VAL(name), &value)) {
            pop(); /* Instance */
            push(value);
            break;
          }
          runtimeError("Field '%s' not found in Map", name->chars);
          RETURN_RUNTIME_ERROR();
        }

        if (IS_NATIVE(peek(0))) {
          ObjNative *n = AS_NATIVE(peek(0));
          if (n->descriptor->getField) {
            name = READ_STRING();
            if (n->descriptor->getField(n, name, &value)) {
              pop(); /* Instance */
              push(value);
              break;
            } else {
              runtimeError(
                "Field '%s' not found in native type %s",
                name->chars,
                getKindName(peek(0)));
              RETURN_RUNTIME_ERROR();
            }
          }
        }

        runtimeError(
          "%s values do not have have fields", getKindName(peek(0)));
        RETURN_RUNTIME_ERROR();
      }
      case OP_SET_FIELD: {
        Value value;

        if (IS_INSTANCE(peek(1))) {
          ObjInstance *instance;
          instance = AS_INSTANCE(peek(1));
          mapSetStr(&instance->fields, READ_STRING(), peek(0));
          value = pop();
          pop();
          push(value);
          break;
        }

        if (IS_DICT(peek(1))) {
          ObjDict *d = AS_DICT(peek(1));
          mapSet(&d->dict, STRING_VAL(READ_STRING()), peek(0));
          value = pop();
          pop();
          push(value);
          break;
        }

        if (IS_NATIVE(peek(1))) {
          ObjNative *n = AS_NATIVE(peek(1));
          if (n->descriptor->setField) {
            String *name = READ_STRING();
            if (n->descriptor->setField(n, name, peek(0))) {
              value = pop();
              pop();
              push(value);
              break;
            } else {
              runtimeError(
                "Field %s not found on %s",
                name->chars,
                getKindName(peek(1)));
              RETURN_RUNTIME_ERROR();
            }
          }
        }

        runtimeError(
          "%s values do not have have fields", getKindName(peek(1)));
        RETURN_RUNTIME_ERROR();
        break;
      }
      case OP_IS: {
        Value b = pop();
        Value a = pop();
        push(BOOL_VAL(valuesIs(a, b)));
        break;
      }
      case OP_EQUAL: {
        Value b = pop();
        Value a = pop();
        push(BOOL_VAL(valuesEqual(a, b)));
        break;
      }
      case OP_GREATER: {
        ubool result = valueLessThan(peek(0), peek(1));
        pop();
        pop();
        push(BOOL_VAL(result));
        break;
      }
      case OP_LESS: {
        ubool result = valueLessThan(peek(1), peek(0));
        pop();
        pop();
        push(BOOL_VAL(result));
        break;
      }
      case OP_ADD: {
        if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
          concatenate();
        } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
          double b = AS_NUMBER(pop());
          double a = AS_NUMBER(pop());
          push(NUMBER_VAL(a + b));
        } else {
          runtimeError("Operands must be two numbers or two strings");
          RETURN_RUNTIME_ERROR();
        }
        break;
      }
      case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -); break;
      case OP_MULTIPLY: {
        if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
          double b = AS_NUMBER(pop());
          double a = AS_NUMBER(pop());
          push(NUMBER_VAL(a * b));
        } else {
          if (!invoke(vm.mulString, 1)) {
            RETURN_RUNTIME_ERROR();
          }
          frame = &vm.frames[vm.frameCount - 1];
        }
        break;
      }
      case OP_DIVIDE: BINARY_OP(NUMBER_VAL, /); break;
      case OP_FLOOR_DIVIDE: {
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
          runtimeError("Operands must be numbers");
          RETURN_RUNTIME_ERROR();
        }
        {
          double b = AS_NUMBER(pop());
          double a = AS_NUMBER(pop());
          push(NUMBER_VAL(floor(a / b)));
        }
        break;
      }
      case OP_MODULO:
        if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
          double b = AS_NUMBER(pop());
          double a = AS_NUMBER(pop());
          push(NUMBER_VAL(fmod(a, b)));
        } else {
          if (!invoke(vm.modString, 1)) {
            RETURN_RUNTIME_ERROR();
          }
          frame = &vm.frames[vm.frameCount - 1];
        }
        break;
      case OP_SHIFT_LEFT: BINARY_BITWISE_OP(<<); break;
      case OP_SHIFT_RIGHT: BINARY_BITWISE_OP(>>); break;
      case OP_BITWISE_OR: BINARY_BITWISE_OP(|); break;
      case OP_BITWISE_AND: BINARY_BITWISE_OP(&); break;
      case OP_BITWISE_XOR: BINARY_BITWISE_OP(^); break;
      case OP_BITWISE_NOT: {
        u32 x;
        if (!IS_NUMBER(peek(0))) {
          runtimeError("Operand must be a number");
          RETURN_RUNTIME_ERROR();
        }
        x = AS_U32(pop());
        push(NUMBER_VAL(~x));
        break;
      }
      case OP_IN: {
        if (IS_CLASS(peek(0))) {
          ObjClass *cls = AS_CLASS(pop());
          push(BOOL_VAL(cls == getClassOfValue(pop())));
        } else {
          Value b = pop();
          Value a = pop();
          push(b);
          push(a);
          if (!invoke(vm.containsString, 1)) {
            RETURN_RUNTIME_ERROR();
          }
          frame = &vm.frames[vm.frameCount - 1];
        }
        break;
      }
      case OP_NOT:
        push(BOOL_VAL(isFalsey(pop())));
        break;
      case OP_NEGATE:
        if (!IS_NUMBER(peek(0))) {
          runtimeError("Operand must be an number");
          RETURN_RUNTIME_ERROR();
        }
        push(NUMBER_VAL(-AS_NUMBER(pop())));
        break;
      case OP_JUMP: {
        u16 offset = READ_SHORT();
        frame->ip += offset;
        break;
      }
      case OP_JUMP_IF_FALSE: {
        u16 offset = READ_SHORT();
        if (isFalsey(peek(0))) {
          frame->ip += offset;
        }
        break;
      }
      case OP_JUMP_IF_STOP_ITERATION: {
        u16 offset = READ_SHORT();
        if (IS_STOP_ITERATION(peek(0))) {
          frame->ip += offset;
        }
        break;
      }
      case OP_TRY_START: {
        u16 offset = READ_SHORT();
        TrySnapshot *snapshot;
        if (vm.trySnapshotsCount >= TRY_SNAPSHOTS_MAX) {
          panic("try snapshot overflow");
        }
        snapshot = &vm.trySnapshots[vm.trySnapshotsCount++];
        snapshot->frameCount = vm.frameCount;
        snapshot->ip = frame->ip + offset;
        snapshot->stackTop = vm.stackTop;
        if (frame != &vm.frames[vm.frameCount - 1]) {
          panic("internal vm frame error");
        }
        break;
      }
      case OP_TRY_END: {
        u16 offset = READ_SHORT();
        if (vm.trySnapshotsCount == 0) {
          panic("try snapshot underflow");
        }
        frame->ip += offset;
        vm.trySnapshotsCount--;
        break;
      }
      case OP_RAISE: {
        if (!IS_STRING(peek(0))) {
          panic("Only strings can be raised right now");
        }
        runtimeError("%s", AS_STRING(peek(0))->chars);
        RETURN_RUNTIME_ERROR();
      }
      case OP_GET_ITER: {
        Value iterable = peek(0);
        if (isIterator(iterable)) {
          /* nothing to do */
        } else {
          if (!invoke(vm.iterString, 0)) {
            RETURN_RUNTIME_ERROR();
          }
        }
        break;
      }
      case OP_GET_NEXT: {
        push(peek(0));
        if (!callValue(peek(0), 0)) {
          RETURN_RUNTIME_ERROR();
        }
        frame = &vm.frames[vm.frameCount - 1];
        break;
      }
      case OP_LOOP: {
        u16 offset = READ_SHORT();
        frame->ip -= offset;
        break;
      }
      case OP_CALL: {
        i16 argCount = READ_BYTE();
        if (!callValue(peek(argCount), argCount)) {
          RETURN_RUNTIME_ERROR();
        }
        frame = &vm.frames[vm.frameCount - 1];
        break;
      }
      case OP_INVOKE: {
        String *method = READ_STRING();
        i16 argCount = READ_BYTE();
        if (!invoke(method, argCount)) {
          RETURN_RUNTIME_ERROR();
        }
        frame = &vm.frames[vm.frameCount - 1];
        break;
      }
      case OP_SUPER_INVOKE: {
        String *method = READ_STRING();
        i16 argCount = READ_BYTE();
        ObjClass *superclass = AS_CLASS(pop());
        if (!invokeFromClass(superclass, method, argCount)) {
          RETURN_RUNTIME_ERROR();
        }
        frame = &vm.frames[vm.frameCount - 1];
        break;
      }
      case OP_CLOSURE: {
        ObjThunk *thunk = AS_THUNK(READ_CONSTANT());
        ObjClosure *closure = newClosure(thunk, frame->closure->module);
        i16 i;
        push(CLOSURE_VAL(closure));
        for (i = 0; i < closure->upvalueCount; i++) {
          u8 isLocal = READ_BYTE();
          u8 index = READ_BYTE();
          if (isLocal) {
            closure->upvalues[i] =
              captureUpvalue(frame->slots + index);
          } else {
            closure->upvalues[i] = frame->closure->upvalues[index];
          }
        }
        break;
      }
      case OP_CLOSE_UPVALUE:
        closeUpvalues(vm.stackTop - 1);
        pop();
        break;
      case OP_RETURN: {
        Value result = pop();
        closeUpvalues(frame->slots);
        vm.frameCount--;
        if (vm.frameCount == returnFrameCount) {
          pop(); /* script function object that started the call */

          vm.stackTop = frame->slots;
          push(result);
          if (vm.frameCount > 0) {
            frame = &vm.frames[vm.frameCount - 1];
          }

          return UTRUE;
        }

        vm.stackTop = frame->slots;
        push(result);
        frame = &vm.frames[vm.frameCount - 1];
        break;
      }
      case OP_IMPORT: {
        String *name = READ_STRING();
        if (!importModule(name)) {
          RETURN_RUNTIME_ERROR();
        }
        break;
      }
      case OP_NEW_LIST: {
        size_t i, length = READ_BYTE();
        ObjList *list = newList(length);
        Value *start = vm.stackTop - length;
        for (i = 0; i < length; i++) {
          list->buffer[i] = start[i];
        }
        *start = LIST_VAL(list);
        vm.stackTop = start + 1;
        break;
      }
      case OP_NEW_DICT: {
        size_t i, length = READ_BYTE();
        ObjDict *dict = newDict();
        Value *start = vm.stackTop - 2 * length;
        push(DICT_VAL(dict)); /* preserve for GC */
        for (i = 0; i < 2 * length; i += 2) {
          mapSet(&dict->dict, start[i], start[i + 1]);
        }
        vm.stackTop = start;
        push(DICT_VAL(dict));
        break;
      }
      case OP_CLASS:
        push(CLASS_VAL(newClass(READ_STRING())));
        break;
      case OP_INHERIT: {
        Value superclass;
        ObjClass *subclass;
        superclass = peek(1);
        if (!IS_CLASS(superclass)) {
          runtimeError("Superclass must be a class");
          RETURN_RUNTIME_ERROR();
        }

        subclass = AS_CLASS(peek(0));
        mapAddAll(&AS_CLASS(superclass)->methods, &subclass->methods);
        pop(); /* subclass */
        break;
      }
      case OP_METHOD:
        defineMethod(READ_STRING());
        break;
    }
  }
#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_SHORT
#undef READ_STRING
#undef BINARY_OP
}

/* Runs true on success, false otherwise */
ubool interpret(const char *source, ObjInstance *module) {
  ObjClosure *closure;
  ObjThunk *thunk = compile(source, module->klass->name);
  if (thunk == NULL) {
    return UFALSE;
  }

  push(THUNK_VAL(thunk));
  closure = newClosure(thunk, module);
  pop();
  push(CLOSURE_VAL(closure));
  call(closure, 0);

  return run();
}

static void prepPrelude() {
  /* Import __prelude__ */
  if (!importModule(vm.preludeString)) {
    panic("Faield to load prelude");
  }
  if (!IS_MODULE(peek(0)) ||
      strcmp(AS_INSTANCE(peek(0))->klass->name->chars, "__prelude__") != 0) {
    panic("Unexpected stack state after loading prelude");
  }

  /* Copy over values from __prelude__ into global */
  {
    ObjInstance *prelude = AS_INSTANCE(peek(0));
    MapIterator ti, tj;
    MapEntry *entry, *e;

    initMapIterator(&ti, &prelude->fields);
    while (mapIteratorNext(&ti, &entry)) {
      if (valueIsCString(entry->key, "sorted") ||
          valueIsCString(entry->key, "list") ||
          valueIsCString(entry->key, "tuple") ||
          valueIsCString(entry->key, "dict") ||
          valueIsCString(entry->key, "set")) {
        mapSet(&vm.globals, entry->key, entry->value);
      } else if (valueIsCString(entry->key, "__List__")) {
        ObjClass *mixinListClass;
        if (!IS_CLASS(entry->value)) {
          panic("__prelude__.__List__ is not a class");
        }
        mixinListClass = AS_CLASS(entry->value);
        initMapIterator(&tj, &mixinListClass->methods);
        while (mapIteratorNext(&tj, &e)) {
          if (valueIsCString(e->key, "sort")) {
            mapSet(&vm.listClass->methods, e->key, e->value);
          }
        }
      }
    }
  }
}

ubool valueIsCString(Value value, const char *string) {
  return IS_STRING(value) && strcmp(AS_STRING(value)->chars, string) == 0;
}
