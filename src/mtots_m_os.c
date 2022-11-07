#include "mtots_m_os.h"

#include "mtots.h"

#include <string.h>
#include <stdlib.h>

static ubool implDirname(Ref out, RefSet args) {
  const char *chars = getString(refAt(args, 0));
  size_t i = stringSize(refAt(args, 0));
  while (i > 0 && chars[i - 1] == PATH_SEP) {
    i--;
  }
  for (; i > 0; i--) {
    if (chars[i - 1] == PATH_SEP) {
      i--;
      break;
    }
  }
  setString(out, chars, i);
  return UTRUE;
}

static CFunc funcDirname = { implDirname, "dirname", 1 };

static ubool implBasename(Ref out, RefSet args) {
  const char *chars = getString(refAt(args, 0));
  size_t i = stringSize(refAt(args, 0)), end;
  while (i > 0 && chars[i - 1] == PATH_SEP) {
    i--;
  }
  end = i;
  for (; i > 0; i--) {
    if (chars[i - 1] == PATH_SEP) {
      break;
    }
  }
  setString(out, chars + i, end - i);
  return UTRUE;
}

static CFunc funcBasename = {implBasename, "basename", 1};

/* TODO: For now, this is basically just a simple join, but in
 * the future, this function will have to evolve to match the
 * behaviors you would expect from a path join */
static ubool implJoin(Ref out, RefSet args) {
  size_t i, len = args.length - 1;
  char *chars, *p;
  for (i = 0; i < args.length; i++) {
    len += stringSize(refAt(args, i));
  }
  p = chars = malloc(len + 1);
  for (i = 0; i < args.length; i++) {
    const char *schars = getString(refAt(args, i));
    size_t slength = stringSize(refAt(args, i));
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
  setString(out, chars, len);
  free(chars);
  return UTRUE;
}

static CFunc funcJoin = { implJoin, "join", 1, MAX_ARG_COUNT };

static ubool impl(Ref out, RefSet args) {
  Ref module = refAt(args, 0);
  CFunc *cfuncs[] = {
    &funcDirname,
    &funcBasename,
    &funcJoin,
  };
  size_t i;
  StackState stackState = getStackState();
  Ref ref = allocRef();

  for (i = 0; i < sizeof(cfuncs)/sizeof(CFunc*); i++) {
    setCFunc(ref ,cfuncs[i]);
    setInstanceField(module, cfuncs[i]->name, ref);
  }

  setCString(ref, OS_NAME);
  setInstanceField(module, "name", ref);

  setCString(ref, PATH_SEP_STR);
  setInstanceField(module, "sep", ref);

  restoreStackState(stackState);
  return UTRUE;
}

static CFunc func = { impl, "os", 1 };

void addNativeModuleOs() {
  addNativeModuleCFunc(&func);
}
