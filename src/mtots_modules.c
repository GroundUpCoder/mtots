#include "mtots_modules.h"
#include "mtots_m_os.h"
#include "mtots_m_sdl.h"
#include "mtots_m_opengles3.h"

void addNativeModules() {
  addNativeModuleOs();

  addNativeModuleSDL();
  addNativeModuleOpenGLES3();
}
