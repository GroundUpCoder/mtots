#include "mtots_m_sdl.h"
#include "mtots_vm.h"

#if MTOTS_ENABLE_SDL

#include "mtots_m_sdl_common.h"
#include "mtots_m_sdl_dat.h"
#include "mtots_m_sdl_key.h"
#include "mtots_m_sdl_render.h"
#include "mtots_m_sdl_kstate.h"
#include "mtots_m_sdl_win.h"
#include "mtots_m_sdl_event.h"
#include "mtots_m_sdl_rect.h"
#include "mtots_m_sdl_point.h"
#include "mtots_m_sdl_surf.h"
#include "mtots_m_sdl_texture.h"
#include "mtots_m_sdl_adev.h"
#include "mtots_m_sdl_funcs.h"
#include "mtots_m_sdl_acb.h"
#include "mtots_m_sdl_gl.h"

static ubool impl(i16 argCount, Value *args, Value *out) {
  ObjInstance *module = AS_INSTANCE(args[0]);
  size_t i;
  ObjInstance *sdlglModule;
  ObjDict *dict;

  audioTracksetMutex = SDL_CreateMutex();

  /* initialize all strings retained in this module */
  mtots_m_SDL_initStrings(module);

  /* initialize all classes in this module */
  for (i = 0; i < sizeof(descriptors)/sizeof(NativeObjectDescriptor*); i++) {
    CFunction **method;
    NativeObjectDescriptor *descriptor = descriptors[i];
    ObjClass *klass = newClassFromCString(descriptor->name);
    mapSetN(&module->fields, descriptor->name, CLASS_VAL(klass));
    descriptor->klass = klass;
    klass->descriptor = descriptor;
    for (method = descriptor->methods; method && *method; method++) {
      mapSetN(&klass->methods, (*method)->name, CFUNCTION_VAL(*method));
      (*method)->receiverType.type = TYPE_PATTERN_NATIVE;
      (*method)->receiverType.nativeTypeDescriptor = descriptor;
    }
  }

  for (i = 0; i < sizeof(functions)/sizeof(CFunction*); i++) {
    mapSetN(&module->fields, functions[i]->name, CFUNCTION_VAL(functions[i]));
  }

  sdlglModule = createSDLGLModule();
  push(INSTANCE_VAL(sdlglModule));
  mapSetN(&module->fields, "gl", INSTANCE_VAL(sdlglModule));
  pop(); /* sdlglModule */

  dict = newDict();
  mapSetN(&module->fields, "key", DICT_VAL(dict));
  for (i = 0; i < sizeof(keyConstants)/sizeof(KeyConstant); i++) {
    KeyConstant c = keyConstants[i];
    mapSetN(&dict->map, c.name, NUMBER_VAL(c.value));
  }

  dict = newDict();
  mapSetN(&module->fields, "scancode", DICT_VAL(dict));
  for (i = 0; i < sizeof(scanConstants)/sizeof(KeyConstant); i++) {
    KeyConstant c = scanConstants[i];
    mapSetN(&dict->map, c.name, NUMBER_VAL(c.value));
  }

  dict = newDict();
  mapSetN(&module->fields, "button", DICT_VAL(dict));
  for (i = 0; i < sizeof(buttonConstants)/sizeof(KeyConstant); i++) {
    KeyConstant c = buttonConstants[i];
    mapSetN(&dict->map, c.name, NUMBER_VAL(c.value));
  }

  for (i = 0; i < sizeof(numericConstants)/sizeof(NumericConstant); i++) {
    NumericConstant c = numericConstants[i];
    mapSetN(&module->fields, c.name, NUMBER_VAL(c.value));
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
