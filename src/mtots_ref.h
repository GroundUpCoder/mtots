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
void setNil(Ref out);
void setBool(Ref out, ubool value);
void setNumber(Ref out, double value);
void setString(Ref out, const char *value);

void refGetClass(Ref out, Ref value);

#endif/*mtots_ref_h*/
