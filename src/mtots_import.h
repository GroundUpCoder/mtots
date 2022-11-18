#ifndef mtots_import_h
#define mtots_import_h

#include "mtots_common.h"
#include "mtots_value.h"
#include "mtots_object.h"

ubool importModuleWithPath(String *moduleName, const char *path);
ubool importModule(String *moduleName);

#endif/*mtots_import_h*/
