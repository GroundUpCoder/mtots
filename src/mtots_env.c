#include "mtots_env.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Instead of PATH being a variable list of paths,
 * to simplify things, we just have 4 paths to try.
 *
 *   1. stdlib root: this is where the standard libary can be found
 *   2. lib root: this is where your project's standard library can be found
 *   3. aux root: this is for any additional auxiliary roots you might
 *        want to add
 *   4. root: this is the root of your main project */

#define MTOTS_STDLIB_ROOT_VAR "MTOTS_STDLIB_ROOT"
#define MTOTS_LIB_ROOT_VAR "MTOTS_LIB_ROOT"
#define MTOTS_AUX_ROOT_VAR "MTOTS_AUX_ROOT"
#define MTOTS_ROOT_VAR "MTOTS_ROOT"

static ubool homeLoaded = UFALSE;
static char homeBuffer[MAX_PATH_LENGTH];

static ubool stdlibRootLoaded = UFALSE;
static char stdlibRootBuffer[MAX_PATH_LENGTH];

static ubool libRootLoaded = UFALSE;
static char libRootBuffer[MAX_PATH_LENGTH];
static const char *libRoot;

static ubool auxRootLoaded = UFALSE;
static char auxRootBuffer[MAX_PATH_LENGTH];
static const char *auxRoot;

static ubool rootLoaded = UFALSE;
static char rootBuffer[MAX_PATH_LENGTH];
static const char *root;

static char modulePath[MAX_PATH_LENGTH];

const char *getHome() {
  if (!homeLoaded) {
    const char *env;
    homeLoaded = UTRUE;
    env = getenv("HOME");
    if (env != NULL) {
      strcpy(homeBuffer, env);
      return homeBuffer;
    }
    env = getenv("USERPROFILE");
    if (env != NULL) {
      strcpy(homeBuffer, env);
      return homeBuffer;
    }
    /* Just make a guess at this point */
    strcpy(homeBuffer, "/home");
  }
  return homeBuffer;
}

static const char *getStdlibRoot() {
  if (!stdlibRootLoaded) {
    const char *env;
    stdlibRootLoaded = UTRUE;
    env = getenv(MTOTS_STDLIB_ROOT_VAR);
    if (env != NULL) {
      strcpy(stdlibRootBuffer, env);
      return stdlibRootBuffer;
    }
    /* Just make a guess */
    snprintf(
      stdlibRootBuffer, MAX_PATH_LENGTH,
      "%s"
      PATH_SEP_STR "git"
      PATH_SEP_STR "mtots"
      PATH_SEP_STR "root",
      getHome());
  }
  return stdlibRootBuffer;
}

static const char *getLibRoot() {
  if (!libRootLoaded) {
    const char *env;
    libRootLoaded = UTRUE;
    env = getenv(MTOTS_LIB_ROOT_VAR);
    if (env != NULL) {
      strcpy(libRootBuffer, env);
      return libRoot = libRootBuffer;
    }
    return libRoot = NULL;
  }
  return libRoot;
}

static const char *getAuxRoot() {
  if (!auxRootLoaded) {
    const char *env;
    auxRootLoaded = UTRUE;
    env = getenv(MTOTS_LIB_ROOT_VAR);
    if (env != NULL) {
      strcpy(auxRootBuffer, env);
      return auxRoot = auxRootBuffer;
    }
    return auxRoot = NULL;
  }
  return auxRoot;
}

static const char *getRoot() {
  if (!rootLoaded) {
    const char *env;
    rootLoaded = UTRUE;
    env = getenv(MTOTS_LIB_ROOT_VAR);
    if (env != NULL) {
      strcpy(rootBuffer, env);
      return root = rootBuffer;
    }
    return root = NULL;
  }
  return root;
}

ubool canOpen(const char *path) {
  FILE *file = fopen(path, "r");
  ubool opened = file != NULL;
  fclose(file);
  return opened;
}

const char *findModulePath(const char *moduleName) {
  typedef const char *RootFn(void);
  RootFn *rootfns[] = {
    getRoot,
    getAuxRoot,
    getLibRoot,
    getStdlibRoot,
  };
  size_t i;

  for (i = 0; i < sizeof(rootfns)/sizeof(const char*); i++) {
    if (rootfns[i]() != NULL) {
      snprintf(
        modulePath,
        MAX_PATH_LENGTH, "%s" PATH_SEP_STR "%s" MTOTS_FILE_EXTENSION,
        rootfns[i](), moduleName);
      if (canOpen(modulePath)) {
        return modulePath;
      }
    }
  }

  return NULL;
}
