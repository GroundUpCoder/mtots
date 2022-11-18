#ifndef mtots_ref_h
#define mtots_ref_h

#include "mtots_util.h"

/* Fields for these structs are meant to be private */
typedef struct RefSet { i16 start, length; } RefSet;
typedef struct Ref { i16 i; } Ref;
typedef struct StackState { size_t size; } StackState;

typedef struct CFunc {
  ubool (*body)(Ref out, RefSet args);
  const char *name;
  i16 arity;
  i16 maxArity;
} CFunc;

RefSet allocRefs(i16 n);
Ref refAt(RefSet rs, i16 offset);
Ref allocRef();
StackState getStackState();
void restoreStackState(StackState state);

ubool isNil(Ref r);
ubool isBool(Ref r);
ubool isNumber(Ref r);
ubool isCFunc(Ref r);
ubool isObj(Ref r);
ubool isClass(Ref r);
ubool isClosure(Ref r);
ubool isString(Ref r);
ubool isList(Ref r);
ubool isTuple(Ref r);
ubool isFile(Ref r);

void setNil(Ref out);
void setBool(Ref out, ubool value);
void setNumber(Ref out, double value);
void setCFunc(Ref out, CFunc *value);
void setString(Ref out, const char *string, size_t length);
void setCString(Ref out, const char *string);
void setEmptyList(Ref out);
void setList(Ref out, RefSet items);
void setDict(Ref out);
void setValue(Ref out, Ref src);

void setInstanceField(Ref recv, const char *fieldName, Ref value);

Ref allocNil();
Ref allocBool(ubool boolean);
Ref allocNumber(double number);
Ref allocString(const char *string, size_t length);
Ref allocCString(const char *string);
Ref allocEmptyList();
Ref allocList(RefSet items);
Ref allocDict();
Ref allocValue(Ref src);



void getClass(Ref out, Ref value);
ubool getBool(Ref r);
double getNumber(Ref r);
const char *getString(Ref r);
size_t stringSize(Ref r);


/* Add new native modules */
void addNativeModuleCFunc(CFunc *cfunc);

#endif/*mtots_ref_h*/
