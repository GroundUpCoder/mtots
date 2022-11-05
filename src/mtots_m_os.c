#include "mtots_m_os.h"

#include "mtots.h"

#include <string.h>
#include <stdlib.h>

static ubool implDirname(i16 argc, Ref argv, Ref out) {
  const char *chars = getString(argv);
  size_t i = getStringByteLength(argv);
  while (i > 0 && chars[i - 1] == PATH_SEP) {
    i--;
  }
  for (; i > 0; i--) {
    if (chars[i - 1] == PATH_SEP) {
      i--;
      break;
    }
  }
  setStringWithLength(out, chars, i);
  return UTRUE;
}

static CFunc funcDirname = { implDirname, "dirname", 1 };

static ubool implBasename(i16 argc, Ref argv, Ref out) {
  const char *chars = getString(argv);
  size_t i = getStringByteLength(argv), end;
  while (i > 0 && chars[i - 1] == PATH_SEP) {
    i--;
  }
  end = i;
  for (; i > 0; i--) {
    if (chars[i - 1] == PATH_SEP) {
      break;
    }
  }
  setStringWithLength(out, chars + i, end - i);
  return UTRUE;
}

static CFunc funcBasename = {implBasename, "basename", 1};

/* TODO: For now, this is basically just a simple join, but in
 * the future, this function will have to evolve to match the
 * behaviors you would expect from a path join */
static ubool implJoin(i16 argc, Ref argv, Ref out) {
  size_t i, len = argc - 1;
  char *chars, *p;
  for (i = 0; i < argc; i++) {
    len += getStringByteLength(refAt(argv, i));
  }
  p = chars = malloc(len + 1);
  for (i = 0; i < argc; i++) {
    const char *schars = getString(refAt(argv, i));
    size_t slength = getStringByteLength(refAt(argv, i));
    if (i > 0) {
      *p++ = PATH_SEP;
    }
    memcpy(p, schars, slength);
    p += slength;
  }
  *p = '\0';
  if (p - chars != len) {
    panic("internal consistency error in os.join()");
  }
  setStringWithLength(out, chars, len);
  free(chars);
  return UTRUE;
}

static CFunc funcJoin = { implJoin, "join", 1, MAX_ARG_COUNT };

static ubool impl(i16 argc, Ref argv, Ref out) {
  Ref module = argv;
  CFunc *cfuncs[] = {
    &funcDirname,
    &funcBasename,
    &funcJoin,
  };
  size_t i;
  StackState stackState = getStackState();
  Ref ref = allocRefs(1);

  for (i = 0; i < sizeof(cfuncs)/sizeof(CFunc*); i++) {
    setCFunc(ref ,cfuncs[i]);
    setInstanceField(module, cfuncs[i]->name, ref);
  }

  setString(ref, OS_NAME);
  setInstanceField(module, "name", ref);

  setString(ref, PATH_SEP_STR);
  setInstanceField(module, "sep", ref);

  restoreStackState(stackState);
  return UTRUE;
}

static CFunc func = { impl, "os", 1 };

void addNativeModuleOs() {
  addNativeModuleCFunc(&func);
}
