#include "mtots_modules.h"
#include "mtots_m_os.h"
#include "mtots_m_json.h"
#include "mtots_m_sdl.h"

void addNativeModules() {
  addNativeModuleOs();
  addNativeModuleJson();

  addNativeModuleSDL();
}
