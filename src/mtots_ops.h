#ifndef mtots_ops_h
#define mtots_ops_h

#include "mtots_object.h"

ubool valuesIs(Value a, Value b);
ubool valuesEqual(Value a, Value b);
ubool valueLessThan(Value a, Value b);
void sortList(ObjList *list, ObjList *keys);
ubool valueRepr(StringBuffer *out, Value value);
ubool valueStr(StringBuffer *out, Value value);

#endif/*mtots_ops_h*/
