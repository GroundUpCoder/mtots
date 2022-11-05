#ifndef mtots_ref_h
#define mtots_ref_h

#include "mtots_common.h"

/* Fields for these structs are meant to be private */
typedef struct Ref { i16 i; } Ref;
typedef struct StackState { size_t size; } StackState;

Ref allocRefs(i16 n);
Ref refAt(Ref base, i16 offset);
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
void setString(Ref out, const char *value);

ubool getBool(Ref r);
double getNumber(Ref r);
const char *getString(Ref r);
size_t getStringByteLength(Ref r);

void refGetClass(Ref out, Ref value);

#endif/*mtots_ref_h*/
