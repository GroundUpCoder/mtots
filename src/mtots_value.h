#ifndef mtots_value_h
#define mtots_value_h

#include "mtots_common.h"

typedef struct CFunction CFunction;
typedef struct Obj Obj;
typedef struct ObjString ObjString;

typedef enum {
  OperatorLen
} Operator;

typedef enum {
  SentinelStopIteration,
  SentinelEmptyKey        /* Used internally in Dict */
} Sentinel;

typedef enum {
  VAL_BOOL,
  VAL_NIL,
  VAL_NUMBER,
  VAL_CFUNCTION,
  VAL_OPERATOR,
  VAL_SENTINEL,
  VAL_OBJ
} ValueType;

typedef struct {
  ValueType type;
  union {
    ubool boolean;
    double number;
    CFunction *cfunction;
    Operator operator;
    Sentinel sentinel;
    Obj *obj;
  } as;
} Value;

struct CFunction {
  ubool (*body)(i16 argCount, Value *args, Value *out);
  const char *name;
  i16 arity;
  i16 maxArity;
};

typedef struct {
  size_t capacity;
  size_t count;
  Value *values;
} ValueArray;

#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)
#define IS_CFUNCTION(value) ((value).type == VAL_CFUNCTION)
#define IS_OPERATOR(value) ((value).type == VAL_OPERATOR)
#define IS_SENTINEL(value) ((value).type == VAL_SENTINEL)
#define IS_OBJ(value) ((value).type == VAL_OBJ)
#define AS_OBJ(value) ((value).as.obj)
#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)
#define AS_CFUNCTION(value) ((value).as.cfunction)
#define AS_OPERATOR(value) ((value).as.operator)
#define AS_SENTINEL(value) ((value).as.sentinel)

#define OBJ_VAL(object) (OBJ_VAL_EXPLICIT((Obj*)(object)))

STATIC_INLINE Value BOOL_VAL(ubool value);
STATIC_INLINE Value NIL_VAL();
STATIC_INLINE Value NUMBER_VAL(double value);
STATIC_INLINE Value CFUNCTION_VAL(CFunction *func);
STATIC_INLINE Value OPERATOR_VAL(Operator operator);
STATIC_INLINE Value SENTINEL_VAL(Sentinel sentinel);
STATIC_INLINE Value OBJ_VAL_EXPLICIT(Obj *object);

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

#endif/*mtots_value_h*/
