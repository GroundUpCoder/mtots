#ifndef mtots_util_env_h
#define mtots_util_env_h

#include "mtots_common.h"

const char *getHome();
ubool canOpen(const char *path);
const char *findModulePath(const char *moduleName);

#endif/*mtots_util_env_h*/
