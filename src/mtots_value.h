#ifndef mtots_value_h
#define mtots_value_h

#include "mtots_common.h"
#include "mtots_cfunc.h"

typedef struct CFunction CFunction;
typedef struct Obj Obj;
typedef struct ObjString ObjString;

typedef enum Operator {
  OperatorLen
} Operator;

typedef enum Sentinel {
  SentinelStopIteration,
  SentinelEmptyKey        /* Used internally in Dict */
} Sentinel;

typedef enum ValueType {
  VAL_BOOL,
  VAL_NIL,
  VAL_NUMBER,
  VAL_CFUNC,
  VAL_CFUNCTION,
  VAL_OPERATOR,
  VAL_SENTINEL,
  VAL_OBJ
} ValueType;

typedef struct Value {
  ValueType type;
  union {
    ubool boolean;
    double number;
    CFunc *cfunc;
    CFunction *cfunction; /* cfunction is deprecated, prefer cfunc */
    Operator op;
    Sentinel sentinel;
    Obj *obj;
  } as;
} Value;

/* NOTE: CFunction is deprecated. Use CFunc instead */
struct CFunction {
  ubool (*body)(i16 argCount, Value *args, Value *out);
  const char *name;
  i16 arity;
  i16 maxArity;
  TypePattern *argTypes;
  TypePattern receiverType;
};

typedef struct ValueArray {
  size_t capacity;
  size_t count;
  Value *values;
} ValueArray;

#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)
#define IS_CFUNCTION(value) ((value).type == VAL_CFUNCTION)
#define IS_CFUNC(value) ((value).type == VAL_CFUNC)
#define IS_OPERATOR(value) ((value).type == VAL_OPERATOR)
#define IS_SENTINEL(value) ((value).type == VAL_SENTINEL)
#define IS_OBJ(value) ((value).type == VAL_OBJ)
#define AS_OBJ(value) ((value).as.obj)
#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)
#define AS_CFUNC(value) ((value).as.cfunc)
#define AS_CFUNCTION(value) ((value).as.cfunction)
#define AS_OPERATOR(value) ((value).as.op)
#define AS_SENTINEL(value) ((value).as.sentinel)

/* should-be-inline */ u32 AS_U32(Value value);
/* should-be-inline */ i32 AS_I32(Value value);

#define OBJ_VAL(object) (OBJ_VAL_EXPLICIT((Obj*)(object)))

Value BOOL_VAL(ubool value);
Value NIL_VAL();
Value NUMBER_VAL(double value);
Value CFUNC_VAL(CFunc *func);
Value CFUNCTION_VAL(CFunction *func);
Value OPERATOR_VAL(Operator op);
Value SENTINEL_VAL(Sentinel sentinel);
Value OBJ_VAL_EXPLICIT(Obj *object);

#define IS_STOP_ITERATION(value) ( \
  IS_SENTINEL(value) && \
  ((value).as.sentinel == SentinelStopIteration))
#define IS_EMPTY_KEY(value) ( \
  IS_SENTINEL(value) && \
  ((value).as.sentinel == SentinelEmptyKey))

#define STOP_ITERATION_VAL() (SENTINEL_VAL(SentinelStopIteration))
#define EMPTY_KEY_VAL() (SENTINEL_VAL(SentinelEmptyKey))

void initValueArray(ValueArray *array);
void writeValueArray(ValueArray *array, Value value);
void freeValueArray(ValueArray *array);
void printValue(Value value);
const char *getValueTypeName(ValueType type);
const char *getKindName(Value value);

ubool typePatternMatch(TypePattern pattern, Value value);
const char *getTypePatternName(TypePattern pattern);

/* Just a convenience function to check that a Value is
 * equal to the given C-string */
ubool valueIsCString(Value value, const char *string);

#endif/*mtots_value_h*/
