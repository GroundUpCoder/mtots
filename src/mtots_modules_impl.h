#ifndef mtots_modules_impl_h
#define mtots_modules_impl_h
#include "mtots_modules.h"
#include "mtots_m_os.h"
#include "mtots_m_json.h"
#include "mtots_m_sdl.h"

void addNativeModules() {
  addNativeModuleOs();
  addNativeModuleJson();

  addNativeModuleSDL();
}
#endif/*mtots_modules_impl_h*/
