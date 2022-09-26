#include "mtots_modules.h"
#include "mtots_m_os.h"
#include "mtots_m_sdl.h"

void addNativeModules() {
  addNativeModuleOs();

  addNativeModuleSDL();
}
