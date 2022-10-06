#include "mtots_vm.h"
#include "mtots_debug.h"
#include "mtots_compiler.h"
#include "mtots_memory.h"
#include "mtots_object.h"
#include "mtots_globals.h"
#include "mtots_class_list.h"
#include "mtots_class_file.h"
#include "mtots_class_str.h"
#include "mtots_class_dict.h"
#include "mtots_class_table.h"
#include "mtots_class_ba.h"
#include "mtots_import.h"
#include "mtots_modules.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <math.h>

VM vm;

static ubool invoke(ObjString *name, i16 argCount);
static void prepPrelude();

static void resetStack() {
  vm.stackTop = vm.stack;
  vm.frameCount = 0;
  vm.openUpvalues = NULL;
}

static void printStack() {
  i16 i;
  for (i = vm.frameCount - 1; i >= 0; i--) {
    CallFrame *frame = &vm.frames[i];
    ObjFunction *function = frame->closure->function;
    size_t instruction = frame->ip - function->chunk.code - 1;
    fprintf(
      stderr, "[line %d] in ", function->chunk.lines[instruction]);
    if (function->name == NULL) {
      fprintf(stderr, "script\n");
    } else {
      fprintf(stderr, "%s()\n", function->name->chars);
    }
  }
}

NORETURN void panic(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);
  printStack();
  exit(MTOTS_EXIT_CODE_PANIC);
}

void runtimeError(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);
  printStack();
  resetStack();
}

void defineGlobal(const char *name, Value value) {
  push(OBJ_VAL(copyString(name, strlen(name))));
  push(value);
  tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
  pop();
  pop();
}

void addNativeModule(CFunction *func) {
  ObjString *name;
  if (func->arity != 1) {
    panic("Native modules must accept 1 argument but got %d", func->arity);
  }
  name = copyCString(func->name);
  push(OBJ_VAL(name));
  if (!tableSet(&vm.nativeModuleThunks, name, CFUNCTION_VAL(func))) {
    panic("Native module %s is already defined", name->chars);
  }

  pop(); /* name */
}

static void initNoMethodClass(ObjClass **clsptr, const char *name) {
  ObjString *tmpstr;

  tmpstr = copyCString(name);
  push(OBJ_VAL(tmpstr));
  *clsptr = newClass(tmpstr);
  (*clsptr)->isBuiltinClass = UTRUE;
  pop(); /* tmpstr */
}

void initVM() {
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
  vm.nilString = NULL;
  vm.trueString = NULL;
  vm.falseString = NULL;
  vm.sentinelClass = NULL;
  vm.nilClass = NULL;
  vm.boolClass = NULL;
  vm.numberClass = NULL;
  vm.stringClass = NULL;
  vm.byteArrayClass = NULL;
  vm.listClass = NULL;
  vm.dictClass = NULL;
  vm.functionClass = NULL;
  vm.operatorClass = NULL;
  vm.classClass = NULL;
  vm.fileClass = NULL;
  vm.tableClass = NULL;
  vm.stdinFile = NULL;
  vm.stdoutFile = NULL;
  vm.stderrFile = NULL;

  initTable(&vm.globals);
  initTable(&vm.strings);
  initTable(&vm.modules);
  initTable(&vm.nativeModuleThunks);

  vm.preludeString = copyCString("__prelude__");
  vm.initString = copyCString("__init__");
  vm.iterString = copyCString("__iter__");
  vm.lenString = copyCString("__len__");
  vm.mulString = copyCString("__mul__");
  vm.modString = copyCString("__mod__");
  vm.nilString = copyCString("nil");
  vm.trueString = copyCString("true");
  vm.falseString = copyCString("false");

  initNoMethodClass(&vm.sentinelClass, "Sentinel");
  initNoMethodClass(&vm.nilClass, "Nil");
  initNoMethodClass(&vm.boolClass, "Bool");
  initNoMethodClass(&vm.numberClass, "Number");
  initStringClass();
  initByteArrayClass();
  initListClass();
  initDictClass();
  initNoMethodClass(&vm.functionClass, "Function");
  initNoMethodClass(&vm.operatorClass, "Operator");
  initNoMethodClass(&vm.classClass, "Class");
  initFileClass();
  initTableClass();

  defineDefaultGlobals();
  addNativeModules();

  prepPrelude();
}

void freeVM() {
  freeTable(&vm.globals);
  freeTable(&vm.strings);
  vm.preludeString = NULL;
  vm.initString = NULL;
  vm.iterString = NULL;
  vm.lenString = NULL;
  vm.mulString = NULL;
  vm.modString = NULL;
  vm.nilString = NULL;
  vm.trueString = NULL;
  vm.falseString = NULL;
  freeObjects();
}

void push(Value value) {
  if (vm.stackTop + 1 > vm.stack + STACK_MAX) {
    exit(MTOTS_EXIT_CODE_STACK_OVERFLOW);
  }
  *vm.stackTop = value;
  vm.stackTop++;
}

Value pop() {
  if (vm.stackTop <= vm.stack) {
    exit(MTOTS_EXIT_CODE_STACK_UNDERFLOW);
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
      case OBJ_CLOSURE: return AS_CLOSURE(value)->function->arity == 0;
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

  if (argCount != closure->function->arity) {
    runtimeError(
      "Expected %d arguments but got %d",
      closure->function->arity, argCount);
    return UFALSE;
  }

  if (vm.frameCount == FRAMES_MAX) {
    runtimeError("Stack overflow");
    return UFALSE;
  }

  frame = &vm.frames[vm.frameCount++];
  frame->closure = closure;
  frame->ip = closure->function->chunk.code;
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
    push(OBJ_VAL(ba));
    return UTRUE;
  }
  if (IS_STRING(arg)) {
    ObjString *str = AS_STRING(arg);
    ObjByteArray *ba = copyByteArray(
      (const unsigned char*)str->chars, str->length);
    pop(); /* arg */
    pop(); /* ByeArray class */
    push(OBJ_VAL(ba));
    return UTRUE;
  }
  if (IS_LIST(arg)) {
    ObjList *list = AS_LIST(arg);
    size_t len = list->length, i;
    unsigned char *buffer = ALLOCATE(unsigned char, len);
    for (i = 0; i < len; i++) {
      Value item = list->buffer[i];
      unsigned char b;
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
    push(OBJ_VAL(takeByteArray(buffer, len)));
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
    vm.stackTop[-argCount - 1] = OBJ_VAL(newInstance(klass));
    if (tableGet(&klass->methods, vm.initString, &initializer)) {
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

      if (IS_OBJ(receiver)) {
        switch (AS_OBJ(receiver)->type) {
          case OBJ_STRING:
            vm.stackTop[-1] = NUMBER_VAL(AS_STRING(receiver)->length);
            return UTRUE;
          case OBJ_BYTE_ARRAY:
            vm.stackTop[-1] = NUMBER_VAL(AS_BYTE_ARRAY(receiver)->size);
          case OBJ_LIST:
            vm.stackTop[-1] = NUMBER_VAL(AS_LIST(receiver)->length);
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
    ObjClass *klass, ObjString *name, i16 argCount) {
  Value method;
  if (!tableGet(&klass->methods, name, &method)) {
    runtimeError(
      "Method '%s' not found in '%s'",
      name->chars,
      klass->name->chars);
    return UFALSE;
  }
  return callValue(method, argCount);
}

static ubool invoke(ObjString *name, i16 argCount) {
  ObjClass *klass;
  Value receiver = peek(argCount);

  klass = getClass(receiver);
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

static void defineMethod(ObjString *name) {
  Value method = peek(0);
  ObjClass *klass = AS_CLASS(peek(1));
  tableSet(&klass->methods, name, method);
  pop();
}

static ubool isFalsey(Value value) {
  return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate() {
  ObjString *result;
  ObjString *b = AS_STRING(peek(0));
  ObjString *a = AS_STRING(peek(1));
  size_t length = a->length + b->length;
  char *chars = ALLOCATE(char, length + 1);
  memcpy(chars, a->chars, a->length);
  memcpy(chars + a->length, b->chars, b->length);
  chars[length] = '\0';
  result = takeString(chars, length);
  pop();
  pop();
  push(OBJ_VAL(result));
}

InterpretResult run(i16 returnFrameCount) {
  CallFrame *frame = &vm.frames[vm.frameCount - 1];

#define READ_BYTE() (*frame->ip++)
#define READ_SHORT() \
  (frame->ip += 2, (u16)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_CONSTANT() \
  (frame->closure->function->chunk.constants.values[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define BINARY_OP(valueType, op) \
  do { \
    if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
      runtimeError("Operands must be numbers"); \
      return INTERPRET_RUNTIME_ERROR; \
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
      return INTERPRET_RUNTIME_ERROR; \
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
    printf("          ");
    for (slot = vm.stack; slot < vm.stackTop; slot++) {
      printf("[ ");
      printValue(*slot);
      printf(" ]");
    }
    printf("\n");
    disassembleInstruction(
      &frame->closure->function->chunk,
      (int)(frame->ip - frame->closure->function->chunk.code));
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
      case OP_ROT_TWO: {
        Value b = pop();
        Value a = pop();
        push(b);
        push(a);
        break;
      }
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
        ObjString *name = READ_STRING();
        Value value;
        if (!tableGet(&frame->closure->module->fields, name, &value)) {
          runtimeError("Undefined variable '%s'", name->chars);
          return INTERPRET_RUNTIME_ERROR;
        }
        push(value);
        break;
      }
      case OP_DEFINE_GLOBAL: {
        ObjString *name = READ_STRING();
        tableSet(&frame->closure->module->fields, name, peek(0));
        pop();
        break;
      }
      case OP_SET_GLOBAL: {
        ObjString *name = READ_STRING();
        if (tableSet(&frame->closure->module->fields, name, peek(0))) {
          tableDelete(&frame->closure->module->fields, name);
          runtimeError("Undefined variable '%s'", name->chars);
          return INTERPRET_RUNTIME_ERROR;
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
        ObjInstance *instance;
        ObjString *name;
        Value value = NIL_VAL();

        if (IS_INSTANCE(peek(0))) {
          instance = AS_INSTANCE(peek(0));
          name = READ_STRING();
          if (tableGet(&instance->fields, name, &value)) {
            pop(); /* Instance */
            push(value);
            break;
          }

          runtimeError("Undefined field '%s'", name->chars);
          return INTERPRET_RUNTIME_ERROR;
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
                "Field %s not found on %s",
                name->chars,
                getKindName(peek(0)));
              return INTERPRET_RUNTIME_ERROR;
            }
          }
        }

        runtimeError(
          "%s values do not have have fields", getKindName(peek(0)));
        return INTERPRET_RUNTIME_ERROR;
      }
      case OP_SET_FIELD: {
        Value value;
        ObjInstance *instance;

        if (IS_INSTANCE(peek(1))) {
          instance = AS_INSTANCE(peek(1));
          tableSet(&instance->fields, READ_STRING(), peek(0));
          value = pop();
          pop();
          push(value);
          break;
        }

        if (IS_NATIVE(peek(1))) {
          ObjNative *n = AS_NATIVE(peek(1));
          if (n->descriptor->setField) {
            ObjString *name = READ_STRING();
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
              return INTERPRET_RUNTIME_ERROR;
            }
          }
        }

        runtimeError(
          "%s values do not have have fields", getKindName(peek(0)));
        return INTERPRET_RUNTIME_ERROR;
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
          return INTERPRET_RUNTIME_ERROR;
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
            return INTERPRET_RUNTIME_ERROR;
          }
          frame = &vm.frames[vm.frameCount - 1];
        }
        break;
      }
      case OP_DIVIDE: BINARY_OP(NUMBER_VAL, /); break;
      case OP_FLOOR_DIVIDE: {
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
          runtimeError("Operands must be numbers");
          return INTERPRET_RUNTIME_ERROR;
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
            return INTERPRET_RUNTIME_ERROR;
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
          return INTERPRET_RUNTIME_ERROR;
        }
        x = AS_U32(pop());
        push(NUMBER_VAL(~x));
        break;
      }
      case OP_NOT:
        push(BOOL_VAL(isFalsey(pop())));
        break;
      case OP_NEGATE:
        if (!IS_NUMBER(peek(0))) {
          runtimeError("Operand must be an number");
          return INTERPRET_RUNTIME_ERROR;
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
      case OP_GET_ITER: {
        Value iterable = peek(0);
        if (isIterator(iterable)) {
          /* nothing to do */
        } else {
          if (!invoke(vm.iterString, 0)) {
            return INTERPRET_RUNTIME_ERROR;
          }
        }
        break;
      }
      case OP_GET_NEXT: {
        push(peek(0));
        if (!callValue(peek(0), 0)) {
          return INTERPRET_RUNTIME_ERROR;
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
          return INTERPRET_RUNTIME_ERROR;
        }
        frame = &vm.frames[vm.frameCount - 1];
        break;
      }
      case OP_INVOKE: {
        ObjString *method = READ_STRING();
        i16 argCount = READ_BYTE();
        if (!invoke(method, argCount)) {
          return INTERPRET_RUNTIME_ERROR;
        }
        frame = &vm.frames[vm.frameCount - 1];
        break;
      }
      case OP_SUPER_INVOKE: {
        ObjString *method = READ_STRING();
        i16 argCount = READ_BYTE();
        ObjClass *superclass = AS_CLASS(pop());
        if (!invokeFromClass(superclass, method, argCount)) {
          return INTERPRET_RUNTIME_ERROR;
        }
        frame = &vm.frames[vm.frameCount - 1];
        break;
      }
      case OP_CLOSURE: {
        ObjFunction *function = AS_FUNCTION(READ_CONSTANT());
        ObjClosure *closure = newClosure(function, frame->closure->module);
        i16 i;
        push(OBJ_VAL(closure));
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

          return INTERPRET_OK;
        }

        vm.stackTop = frame->slots;
        push(result);
        frame = &vm.frames[vm.frameCount - 1];
        break;
      }
      case OP_IMPORT: {
        ObjString *name = READ_STRING();
        if (!importModule(name)) {
          return INTERPRET_RUNTIME_ERROR;
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
        *start = OBJ_VAL(list);
        vm.stackTop = start + 1;
        break;
      }
      case OP_NEW_DICT: {
        size_t i, length = READ_BYTE();
        ObjDict *dict = newDict();
        Value *start = vm.stackTop - 2 * length;
        push(OBJ_VAL(dict)); /* preserve for GC */
        for (i = 0; i < 2 * length; i += 2) {
          dictSet(&dict->dict, start[i], start[i + 1]);
        }
        vm.stackTop = start;
        push(OBJ_VAL(dict));
        break;
      }
      case OP_CLASS:
        push(OBJ_VAL(newClass(READ_STRING())));
        break;
      case OP_INHERIT: {
        Value superclass;
        ObjClass *subclass;
        superclass = peek(1);
        if (!IS_CLASS(superclass)) {
          runtimeError("Superclass must be a class");
          return INTERPRET_RUNTIME_ERROR;
        }

        subclass = AS_CLASS(peek(0));
        tableAddAll(&AS_CLASS(superclass)->methods, &subclass->methods);
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

InterpretResult interpret(const char *source, ObjInstance *module) {
  ObjClosure *closure;
  ObjFunction *function = compile(source);
  if (function == NULL) {
    return INTERPRET_COMPILE_ERROR;
  }

  push(OBJ_VAL(function));
  closure = newClosure(function, module);
  pop();
  push(OBJ_VAL(closure));
  call(closure, 0);

  return run(0);
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
    size_t i, j;

    for (i = 0; i < prelude->fields.capacity; i++) {
      Entry *entry = prelude->fields.entries + i;
      if (entry->key != NULL) {
        if (strcmp(entry->key->chars, "sorted") == 0) {
          tableSet(&vm.globals, entry->key, entry->value);
        } else if (strcmp(entry->key->chars, "__List__") == 0) {
          ObjClass *mixinListClass;
          if (!IS_CLASS(entry->value)) {
            panic("__prelude__.__List__ is not a class");
          }
          mixinListClass = AS_CLASS(entry->value);
          for (j = 0; j < mixinListClass->methods.capacity; j++) {
            Entry *e = mixinListClass->methods.entries + j;
            if (e->key != NULL) {
              if (strcmp(e->key->chars, "sort") == 0) {
                tableSet(&vm.listClass->methods, e->key, e->value);
              }
            }
          }
        }
      }
    }
  }
}
