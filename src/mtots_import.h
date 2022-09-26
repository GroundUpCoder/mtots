#ifndef mtots_import_h
#define mtots_import_h

#include "mtots_common.h"
#include "mtots_value.h"
#include "mtots_object.h"

ubool importModuleWithPath(ObjString *moduleName, const char *path);
ubool importModule(ObjString *moduleName);

#endif/*mtots_import_h*/
