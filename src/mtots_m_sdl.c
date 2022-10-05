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

static ubool impl(i16 argCount, Value *args, Value *out) {
  ObjInstance *module = AS_INSTANCE(args[0]);
  size_t i;
  ObjInstance *inst;

  audioTracksetMutex = SDL_CreateMutex();

  /* initialize all strings retained in this module */
  mtots_m_SDL_initStrings(module);

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
      (*method)->receiverType.type = TYPE_PATTERN_NATIVE;
      (*method)->receiverType.nativeTypeDescriptor = descriptor;
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

  inst = newInstance(vm.tableClass);
  tableSetN(&module->fields, "button", OBJ_VAL(inst));
  for (i = 0; i < sizeof(buttonConstants)/sizeof(KeyConstant); i++) {
    KeyConstant c = buttonConstants[i];
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
