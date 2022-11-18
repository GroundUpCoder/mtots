#include "mtots_vm.h"
#include "mtots_import.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void repl() {
  char line[1024];
  ObjInstance *module;
  String *mainModuleName;
  mainModuleName = internCString("__main__");
  push(STRING_VAL(mainModuleName));
  module = newModule(mainModuleName, UTRUE);
  pop(); /* mainModuleName */
  push(INSTANCE_VAL(module));
  for (;;) {
    printf("> ");

    if (!fgets(line, sizeof(line), stdin)) {
      printf("\n");
      break;
    }

    if (!interpret(line, module)) {
      fprintf(stderr, "%s", getErrorString());
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
    /* "/home/web_user/samples/webgl2/ch02-02-rendering-modes.mtots",*/
    /* "/home/web_user/samples/webgl2/ch02-07-cone.mtots", */
    MTOTS_WEB_START_SCRIPT
  };
  argc = 2;
  argv = fakeArgv;
#endif

  initVM();

  if (argc == 1) {
    repl();
  } else if (argc == 2) {
    ubool status;
    String *mainModuleName = internCString("__main__");
    push(STRING_VAL(mainModuleName));
    status = importModuleWithPath(mainModuleName, argv[1]);
    pop(); /* mainModuleName */
    if (!status) {
      if (getErrorString()) {
        fprintf(stderr, "%s", getErrorString());
      } else {
        fprintf(stderr, "(runtime-error, but no error message set)\n");
      }
      exit(1);
    }
  } else {
    fprintf(stderr, "Usage: mtots [path]\n");
  }

  freeVM();
  return 0;
}
