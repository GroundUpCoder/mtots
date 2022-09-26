#include "mtots_m_sdl.h"
#include "mtots_vm.h"

#if MTOTS_ENABLE_SDL

#include "mtots_m_sdl_dat.h"
#include "mtots_m_sdl_key.h"


static void nopBlacken(ObjNative *n) {}
static void nopFree(ObjNative *n) {}

typedef struct {
  ObjString **location;
  const char *value;
} RetainedString;

static ObjString *string_type;
static ObjString *string_key;
static ObjString *string_repeat;

/**********************************************************
 * ObjWindow
 *********************************************************/
typedef struct {
  ObjNative obj;
  SDL_Window *handle;
} ObjWindow;

static NativeObjectDescriptor descriptorWindow = {
  nopBlacken, nopFree, NULL, NULL, sizeof(ObjWindow), "Window"};

/**********************************************************
 * ObjEvent
 *********************************************************/

typedef struct {
  ObjNative obj;
  SDL_Event data;
} ObjEvent;

static ubool implEvent(i16 argCount, Value *args, Value *out);

static CFunction funcEvent = { implEvent, "Event" , 0 };

static ubool eventGetField(ObjNative *n, ObjString *key, Value *out) {
  ObjEvent *event = (ObjEvent*)n;
  if (key == string_type) {
    *out = NUMBER_VAL(event->data.type);
    return UTRUE;
  } else if (key == string_key) {
    *out = NUMBER_VAL(event->data.key.keysym.sym);
    return UTRUE;
  } else if (key == string_repeat) {
    *out = BOOL_VAL(event->data.key.repeat != 0);
    return UTRUE;
  }
  return UFALSE;
}

static NativeObjectDescriptor descriptorEvent = {
  nopBlacken, nopFree, eventGetField, &funcEvent,
  sizeof(ObjEvent), "Event"};

static ubool implEvent(i16 argCount, Value *args, Value *out) {
  *out = OBJ_VAL(NEW_NATIVE(ObjEvent, &descriptorEvent));
  return UTRUE;
}

/**********************************************************
 * KeyboardState
 *********************************************************/

typedef struct {
  ObjNative obj;
  const Uint8 *state;
} ObjKeyboardState;

static ubool implKeyboardStateGetItem(i16 argCount, Value *args, Value *out);

static CFunction funcKeyboardStateGetItem = {
  implKeyboardStateGetItem, "__getitem__", 1 };

static CFunction *keyboardStateMethods[] = {
  &funcKeyboardStateGetItem,
  NULL,
};

static NativeObjectDescriptor descriptorKeyboardState = {
  nopBlacken, nopFree, NULL, NULL,
  sizeof(ObjKeyboardState), "KeyboardState",
  keyboardStateMethods };

static ubool implKeyboardStateGetItem(i16 argCount, Value *args, Value *out) {
  ObjKeyboardState *kstate;
  size_t index;
  if (!IS_NATIVE(args[-1]) ||
      AS_NATIVE(args[-1])->descriptor != &descriptorKeyboardState) {
    runtimeError("Bad receiver type in KeyboardState.__getitem__(): %s",
      getKindName(args[-1]));
    return UFALSE;
  }
  kstate = (ObjKeyboardState*)AS_NATIVE(args[-1]);
  if (!IS_NUMBER(args[0])) {
    runtimeError("Expected number but got %s", getKindName(args[0]));
    return UFALSE;
  }
  index = (size_t) AS_NUMBER(args[0]);
  *out = BOOL_VAL(kstate->state[index] != 0);
  return UTRUE;
}

/**********************************************************
 * functions: Initialization and Startup
 *********************************************************/

static ubool implInit(i16 argCount, Value *args, Value *out) {
  SDL_SetMainReady();
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    runtimeError("Failed to init SDL video: %s", SDL_GetError());
    return UFALSE;
  }
  return UTRUE;
}

static CFunction funcInit = { implInit, "init", 0 };

static ubool implQuit(i16 argCount, Value *args, Value *out) {
  SDL_Quit();
  return UTRUE;
}

static CFunction funcQuit = { implQuit, "quit", 0 };

static ubool implCreateWindow(i16 argCount, Value *args, Value *out) {
  size_t i;
  ObjWindow *window;
  if (!IS_STRING(args[0])) {
    runtimeError(
      "createWindow() title must be a string but got %s",
      getKindName(args[0]));
    return UFALSE;
  }
  for (i = 1; i < argCount; i++) {
    if (!IS_NUMBER(args[i])) {
      runtimeError(
        "createWindow() coordinates must be a number, but got %s",
        getKindName(args[i]));
      return UFALSE;
    }
  }
  window = NEW_NATIVE(ObjWindow, &descriptorWindow);
  window->handle = SDL_CreateWindow(
    AS_STRING(args[0])->chars,
    (int) (u32) AS_NUMBER(args[1]),
    (int) (u32) AS_NUMBER(args[2]),
    (int) (u32) AS_NUMBER(args[3]),
    (int) (u32) AS_NUMBER(args[4]),
    (u32) AS_NUMBER(args[5]));
  *out = OBJ_VAL(window);
  return UTRUE;
}

static CFunction funcCreateWindow = { implCreateWindow, "createWindow", 6 };

/**********************************************************
 * functions: Input
 *********************************************************/

static ubool implPollEvent(i16 argCount, Value *args, Value *out) {
  ObjEvent *event;
  if (!IS_NATIVE(args[0]) ||
      AS_NATIVE(args[0])->descriptor != &descriptorEvent) {
    runtimeError(
      "sdl.pollEvent() requires an sdl.Event argument but got %s",
      getKindName(args[0]));
    return UFALSE;
  }
  event = (ObjEvent*)AS_OBJ(args[0]);
  *out = BOOL_VAL(SDL_PollEvent(&event->data));
  return UTRUE;
}

static CFunction funcPollEvent = { implPollEvent, "pollEvent", 1 };

static ubool implGetKeyboardState(i16 argCount, Value *args, Value *out) {
  ObjKeyboardState *kstate =
    NEW_NATIVE(ObjKeyboardState, &descriptorKeyboardState);
  kstate->state = SDL_GetKeyboardState(NULL);
  *out = OBJ_VAL(kstate);
  return UTRUE;
}

static CFunction funcGetKeyboardState = {
  implGetKeyboardState, "getKeyboardState", 0 };

static NativeObjectDescriptor *descriptors[] = {
  &descriptorWindow,
  &descriptorEvent,
  &descriptorKeyboardState,
};

static CFunction *functions[] = {
  &funcInit,
  &funcQuit,
  &funcCreateWindow,
  &funcPollEvent,
  &funcGetKeyboardState,
};

static ubool impl(i16 argCount, Value *args, Value *out) {
  ObjInstance *module = AS_INSTANCE(args[0]);
  RetainedString rstrs[] = {
    {&string_type, "type"},
    {&string_key, "key"},
    {&string_repeat, "repeat"},
  };
  size_t i;
  ObjList *list;
  ObjInstance *inst;

  /* initialize all strings retained in this module */
  list = newList(sizeof(rstrs)/sizeof(RetainedString));
  tableSetN(&module->fields, "__retain__", OBJ_VAL(list));
  for (i = 0; i < sizeof(rstrs)/sizeof(RetainedString); i++) {
    *rstrs[i].location = copyCString(rstrs[i].value);
    list->buffer[i] = OBJ_VAL(*rstrs[i].location);
  }

  /* initialize all classes in this module */
  for (i = 0; i < sizeof(descriptors)/sizeof(NativeObjectDescriptor*); i++) {
    CFunction **method;
    NativeObjectDescriptor *descriptor = descriptors[i];
    ObjClass *klass = newClassFromCString(descriptor->name);
    tableSetN(&module->fields, descriptor->name, OBJ_VAL(klass));
    descriptor->klass = klass;
    klass->descriptor = descriptor;
    for (method = descriptor->methods; method && *method; method++) {
      tableSetN(&klass->methods, (*method)->name, CFUNCTION_VAL(*method));
    }
  }

  for (i = 0; i < sizeof(functions)/sizeof(CFunction*); i++) {
    tableSetN(&module->fields, functions[i]->name, CFUNCTION_VAL(functions[i]));
  }

  inst = newInstance(vm.tableClass);
  tableSetN(&module->fields, "key", OBJ_VAL(inst));
  for (i = 0; i < sizeof(keyConstants)/sizeof(KeyConstant); i++) {
    KeyConstant c = keyConstants[i];
    tableSetN(&inst->fields, c.name, NUMBER_VAL(c.value));
  }

  inst = newInstance(vm.tableClass);
  tableSetN(&module->fields, "scancode", OBJ_VAL(inst));
  for (i = 0; i < sizeof(scanConstants)/sizeof(KeyConstant); i++) {
    KeyConstant c = scanConstants[i];
    tableSetN(&inst->fields, c.name, NUMBER_VAL(c.value));
  }

  for (i = 0; i < sizeof(numericConstants)/sizeof(NumericConstant); i++) {
    NumericConstant c = numericConstants[i];
    tableSetN(&module->fields, c.name, NUMBER_VAL(c.value));
  }

  return UTRUE;
}

static CFunction func = { impl, "sdl", 1 };

void addNativeModuleSDL() {
  addNativeModule(&func);
}

#else
void addNativeModuleSDL() {}
#endif
