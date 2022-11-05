#ifndef mtots_import_impl_h
#define mtots_import_impl_h
#include "mtots_import.h"
#include "mtots_object.h"
#include "mtots_vm.h"
#include "mtots_compiler.h"
#include "mtots_env.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *readFile(const char *path) {
  char *buffer;
  size_t fileSize, bytesRead;
  FILE *file = fopen(path, "rb");

  if (file == NULL) {
    panic("Could not open file \"%s\"\n", path);
  }

  /* NOTE: This might not actually be standards
   * compliant - i.e. this may fail on some platforms.
   */
  fseek(file, 0L, SEEK_END);
  fileSize = ftell(file);
  rewind(file);

  buffer = (char*)malloc(fileSize + 1);
  if (buffer == NULL) {
    panic("Not enough memory to read \"%s\"\n", path);
  }
  bytesRead = fread(buffer, sizeof(char), fileSize, file);
  if (bytesRead < fileSize) {
    panic("Could not read file \"%s\"\n", path);
  }
  buffer[bytesRead] = '\0';

  fclose(file);
  return buffer;
}

/* Runs the module specified by the given path with the given moduleName
 * Puts the result of running the module on the top of the stack
 * NOTE: Never cached (unlike importModule())
 */
ubool importModuleWithPath(ObjString *moduleName, const char *path) {
  char *source = readFile(path);
  ObjClosure *closure;
  ObjFunction *function;
  i16 returnFrameCount = vm.frameCount;
  ObjInstance *module;
  ObjString *pathStr;

  module = newModule(moduleName, UTRUE);
  push(OBJ_VAL(module));

  pathStr = copyCString(path);
  push(OBJ_VAL(pathStr));
  tableSetN(&module->fields, "__path__", OBJ_VAL(pathStr));
  pop(); /* pathStr */

  function = compile(source, moduleName);
  if (function == NULL) {
    runtimeError("Failed to compile %s", path);
    return UFALSE;
  }

  push(OBJ_VAL(function));
  closure = newClosure(function, module);
  pop(); /* function */

  push(OBJ_VAL(closure));

  call(closure, 0);

  if (run(returnFrameCount)) {
    pop(); /* return value from run */

    push(OBJ_VAL(module));

    /* We need to copy all fields of the instance to the class so
     * that method calls will properly call the functions in the module */
    tableAddAll(&module->fields, &module->klass->methods);

    pop(); /* module */

    /* the module we pushed in the beginning is still on the stack */
    return UTRUE;
  }
  return UFALSE;
}

static ubool importModuleNoCache(ObjString *moduleName) {
  Value nativeModuleThunkValue;

  /* Check for a native module with the given name */
  if (tableGet(&vm.nativeModuleThunks, moduleName, &nativeModuleThunkValue)) {
    ObjInstance *module;
    CFunction *nativeModuleThunk;
    Value moduleValue, result = NIL_VAL(), *stackStart;
    if (!IS_CFUNCTION(nativeModuleThunkValue)) {
      abort();
    }
    nativeModuleThunk = AS_CFUNCTION(nativeModuleThunkValue);
    module = newModule(moduleName, UFALSE);
    moduleValue = OBJ_VAL(module);
    push(OBJ_VAL(module));
    stackStart = vm.stackTop;
    if (!nativeModuleThunk->body(1, &moduleValue, &result)) {
      return UFALSE;
    }
    /* At this point, module should be at the top of the stack */
    if (vm.stackTop != stackStart) {
      panic(
        "Native module started with %d items on the stack, but "
        "ended with %d",
        stackStart - vm.stack,
        vm.stackTop - vm.stack);
    }

    /* We need to copy all fields of the instance to the class so
     * that method calls will properly call the functions in the module */
    tableAddAll(&module->fields, &module->klass->methods);

    return UTRUE;
  } else {
    /* Otherwise, we're working with a script */
    const char *path = findModulePath(moduleName->chars);
    if (path == NULL) {
      runtimeError("Could not find module %s", moduleName->chars);
      return UFALSE;
    }
    return importModuleWithPath(moduleName, path);
  }
}

/* Loads the module specified by the given moduleName
 * Puts the result of running the module on the top of the stack
 *
 * importModule will search the modules cache first, and
 * if not found, will call importModuleWithPath and add the
 * new entry into the cache
 */
ubool importModule(ObjString *moduleName) {
  Value module = NIL_VAL();
  if (tableGet(&vm.modules, moduleName, &module)) {
    if (!IS_MODULE(module)) {
      abort(); /* vm.modules table should only contain modules */
    }
    push(module);
    return UTRUE;
  }
  if (!importModuleNoCache(moduleName)) {
    return UFALSE;
  }

  /* if importModuleNoCache is successful, we should have
   * a module at TOS */
  if (!IS_MODULE(vm.stackTop[-1])) {
    abort();
  }
  tableSet(&vm.modules, moduleName, vm.stackTop[-1]);
  return UTRUE;
}
#endif/*mtots_import_impl_h*/
