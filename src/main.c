#include "mtots_common.h"
#include "mtots_chunk.h"
#include "mtots_debug.h"
#include "mtots_vm.h"
#include "mtots_compiler.h"
#include "mtots_import.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void repl() {
  char line[1024];
  ObjInstance *module;
  ObjString *mainModuleName;
  mainModuleName = copyCString("__main__");
  push(OBJ_VAL(mainModuleName));
  module = newModule(mainModuleName, UTRUE);
  pop(); /* mainModuleName */
  push(OBJ_VAL(module));
  for (;;) {
    printf("> ");

    if (!fgets(line, sizeof(line), stdin)) {
      printf("\n");
      break;
    }

    if (interpret(line, module) == INTERPRET_RUNTIME_ERROR) {
      fprintf(stderr, "%s", vm.errorString);
    }
    pop(); /* return value */
  }
  pop(); /* module */
}

int main(int argc, const char *argv[]) {
#ifdef __EMSCRIPTEN__
  const char *fakeArgv[2] = {
    "",
    /* "/home/web_user/apps/music-keyboard.mtots", */
    /* "/home/web_user/samples/angle-00.mtots", */
    "/home/web_user/samples/webgl2/ch02-02-rendering-modes.mtots",
  };
  argc = 2;
  argv = fakeArgv;
#endif

  initRules();
  initVM();

  if (argc == 1) {
    repl();
  } else if (argc == 2) {
    ubool status;
    ObjString *mainModuleName = copyCString("__main__");
    push(OBJ_VAL(mainModuleName));
    status = importModuleWithPath(mainModuleName, argv[1]);
    pop(); /* mainModuleName */
    if (!status) {
      if (vm.errorString) {
        fprintf(stderr, "%s", vm.errorString);
      }
      exit(MTOTS_EXIT_CODE_RUNTIME_ERROR);
    }
  } else {
    fprintf(stderr, "Usage: mtots [path]\n");
  }

  freeVM();
  return 0;
}
