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
  module = newModule(mainModuleName);
  pop(); /* mainModuleName */
  push(OBJ_VAL(module));
  for (;;) {
    printf("> ");

    if (!fgets(line, sizeof(line), stdin)) {
      printf("\n");
      break;
    }

    interpret(line, module);
  }
  pop(); /* module */
}

int main(int argc, const char *argv[]) {
#ifdef __EMSCRIPTEN__
  const char *fakeArgv[2] = {
    "",
    "/home/web_user/apps/music-keyboard.mtots",
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
      exit(MTOTS_EXIT_CODE_RUNTIME_ERROR);
    }
  } else {
    fprintf(stderr, "Usage: mtots [path]\n");
  }

  freeVM();
  return 0;
}
