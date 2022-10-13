#define SDL_MAIN_HANDLED
#include <SDL.h>

#define GL_GLES_PROTOTYPES 0
#include <GLES3/gl3.h>

#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <dlfcn.h>

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

static const char kVS[] = "\n"
"attribute vec4 vPosition;\n"
"void main()\n"
"{\n"
"  gl_Position = vPosition;\n"
"}\n";

static const char kFS[] = "\n"
"precision mediump float;\n"
"void main()\n"
"{\n"
"  gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
"}\n";

static GLuint mainProgram;
static GLuint mainBuffer;

static PFNGLATTACHSHADERPROC glAttachShader;
static PFNGLBINDBUFFERPROC glBindBuffer;
static PFNGLBUFFERDATAPROC glBufferData;
static PFNGLCLEARPROC glClear;
static PFNGLCLEARCOLORPROC glClearColor;
static PFNGLCOMPILESHADERPROC glCompileShader;
static PFNGLCREATEPROGRAMPROC glCreateProgram;
static PFNGLCREATESHADERPROC glCreateShader;
static PFNGLDELETEPROGRAMPROC glDeleteProgram;
static PFNGLDELETESHADERPROC glDeleteShader;
static PFNGLDRAWARRAYSPROC glDrawArrays;
static PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
static PFNGLGENBUFFERSPROC glGenBuffers;
static PFNGLGETERRORPROC glGetError;
static PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
static PFNGLGETPROGRAMIVPROC glGetProgramiv;
static PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
static PFNGLGETSHADERIVPROC glGetShaderiv;
static PFNGLGETSTRINGPROC glGetString;
static PFNGLLINKPROGRAMPROC glLinkProgram;
static PFNGLSHADERSOURCEPROC glShaderSource;
static PFNGLUSEPROGRAMPROC glUseProgram;
static PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
static PFNGLVIEWPORTPROC glViewport;

void loadOpenGL() {
  void *handle = dlopen("libGLESv2.dylib", RTLD_NOW);
  if (!handle) { printf("FAILED TO LOAD GLESv2\n"); return; }
  glAttachShader = (PFNGLATTACHSHADERPROC) dlsym(handle, "glAttachShader");
  glBindBuffer = (PFNGLBINDBUFFERPROC) dlsym(handle, "glBindBuffer");
  glBufferData = (PFNGLBUFFERDATAPROC) dlsym(handle, "glBufferData");
  glClear = (PFNGLCLEARPROC) dlsym(handle, "glClear");
  glClearColor = (PFNGLCLEARCOLORPROC) dlsym(handle, "glClearColor");
  glCompileShader = (PFNGLCOMPILESHADERPROC) dlsym(handle, "glCompileShader");
  glCreateProgram = (PFNGLCREATEPROGRAMPROC) dlsym(handle, "glCreateProgram");
  glCreateShader = (PFNGLCREATESHADERPROC) dlsym(handle, "glCreateShader");
  glDeleteProgram = (PFNGLDELETEPROGRAMPROC) dlsym(handle, "glDeleteProgram");
  glDeleteShader = (PFNGLDELETESHADERPROC) dlsym(handle, "glDeleteShader");
  glDrawArrays = (PFNGLDRAWARRAYSPROC) dlsym(handle, "glDrawArrays");
  glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC) dlsym(handle, "glEnableVertexAttribArray");
  glGenBuffers = (PFNGLGENBUFFERSPROC) dlsym(handle, "glGenBuffers");
  glGetError = (PFNGLGETERRORPROC) dlsym(handle, "glGetError");
  glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC) dlsym(handle, "glGetProgramInfoLog");
  glGetProgramiv = (PFNGLGETPROGRAMIVPROC) dlsym(handle, "glGetProgramiv");
  glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC) dlsym(handle, "glGetShaderInfoLog");
  glGetShaderiv = (PFNGLGETSHADERIVPROC) dlsym(handle, "glGetShaderiv");
  glGetString = (PFNGLGETSTRINGPROC) dlsym(handle, "glGetString");
  glLinkProgram = (PFNGLLINKPROGRAMPROC) dlsym(handle, "glLinkProgram");
  glShaderSource = (PFNGLSHADERSOURCEPROC) dlsym(handle, "glShaderSource");
  glUseProgram = (PFNGLUSEPROGRAMPROC) dlsym(handle, "glUseProgram");
  glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC) dlsym(handle, "glVertexAttribPointer");
  glViewport = (PFNGLVIEWPORTPROC) dlsym(handle, "glViewport");

  if (!glAttachShader) { printf("FAILED TO LOAD glAttachShader\n"); }
  if (!glBindBuffer) { printf("FAILED TO LOAD glBindBuffer\n"); }
  if (!glBufferData) { printf("FAILED TO LOAD glBufferData\n"); }
  if (!glClear) { printf("FAILED TO LOAD glClear\n"); }
  if (!glClearColor) { printf("FAILED TO LOAD glClearColor\n"); }
  if (!glCompileShader) { printf("FAILED TO LOAD glCompileShader\n"); }
  if (!glCreateProgram) { printf("FAILED TO LOAD glCreateProgram\n"); }
  if (!glCreateShader) { printf("FAILED TO LOAD glCreateShader\n"); }
  if (!glDeleteProgram) { printf("FAILED TO LOAD glDeleteProgram\n"); }
  if (!glDeleteShader) { printf("FAILED TO LOAD glDeleteShader\n"); }
  if (!glDrawArrays) { printf("FAILED TO LOAD glDrawArrays\n"); }
  if (!glEnableVertexAttribArray) { printf("FAILED TO LOAD glEnableVertexAttribArray\n"); }
  if (!glGenBuffers) { printf("FAILED TO LOAD glGenBuffers\n"); }
  if (!glGetError) { printf("FAILED TO LOAD glGetError\n"); }
  if (!glGetProgramInfoLog) { printf("FAILED TO LOAD glGetProgramInfoLog\n"); }
  if (!glGetProgramiv) { printf("FAILED TO LOAD glGetProgramiv\n"); }
  if (!glGetShaderInfoLog) { printf("FAILED TO LOAD glGetShaderInfoLog\n"); }
  if (!glGetShaderiv) { printf("FAILED TO LOAD glGetShaderiv\n"); }
  if (!glGetString) { printf("FAILED TO LOAD glGetString\n"); }
  if (!glLinkProgram) { printf("FAILED TO LOAD glLinkProgram\n"); }
  if (!glShaderSource) { printf("FAILED TO LOAD glShaderSource\n"); }
  if (!glUseProgram) { printf("FAILED TO LOAD glUseProgram\n"); }
  if (!glVertexAttribPointer) { printf("FAILED TO LOAD glVertexAttribPointer\n"); }
  if (!glViewport) { printf("FAILED TO LOAD glViewport\n"); }
}

GLuint compileShader(GLenum type, const char *source) {
  const char *sourceArray[1];
  GLint compileResult;
  GLuint shader = glCreateShader(type);

  sourceArray[0] = source;

  glShaderSource(shader, 1, sourceArray, NULL);
  glCompileShader(shader);
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compileResult);

  if (compileResult == 0) {
    GLint infoLogLength;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

    /* Info log length includes null terminator
     * so 1 means that info log is an empty string */
    if (infoLogLength > 1) {
      char infoLog[1024];
      glGetShaderInfoLog(shader, 1024, NULL, infoLog);
      printf("shader compilation failed: %s\n", infoLog);
    } else {
      printf("shader compilation failed. <Empty log message>\n");
    }
  }
  return shader;
}

GLuint checkLinkStatusAndReturnProgram(GLuint program) {
  GLint linkStatus;

  if (glGetError() != GL_NO_ERROR) {
    return 0;
  }

  glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
  if (linkStatus == 0) {
    GLint infoLogLength;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

    if (infoLogLength > 1) {
      char infoLog[1024];
      glGetProgramInfoLog(program, 1024, NULL, infoLog);
      printf("program link failed: %s\n", infoLog);
    } else {
      printf("program link failed. <Empty log message>\n");
    }

    glDeleteProgram(program);
    return 0;
  }

  return program;
}

GLuint compileProgram(const char *vsSource, const char *fsSource) {
  GLuint vs = compileShader(GL_VERTEX_SHADER, vsSource);
  GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsSource);
  GLuint program;

  if (vs == 0 || fs == 0) {
    glDeleteShader(fs);
    glDeleteShader(vs);
    return 0;
  }

  program = glCreateProgram();

  glAttachShader(program, vs);
  glDeleteShader(vs);

  glAttachShader(program, fs);
  glDeleteShader(fs);

  glLinkProgram(program);

  return checkLinkStatusAndReturnProgram(program);
}

void initialize() {
  GLfloat vertices[] = {
    0.0f, 0.5f, 0.0f,
    -0.5f, -0.5f, 0.0f,
    0.5f, -0.5f, 0.0f,
  };

  mainProgram = compileProgram(kVS, kFS);
  if (!mainProgram) {
    return;
  }

  glClearColor(0.0f, 0.3f, 0.0f, 0.0f);

  glGenBuffers(1, &mainBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, mainBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
}

void draw(SDL_Window *window) {
  int width, height;

  /* We need to get width and height this way which may differ from
   * from SCREEN_WIDTH, SCREEN_HEIGHT if we're working in high DPI
   * environments (e.g. macos retina displays)
   *
   * If we don't do this, on a mac it looks like the drawing will
   * only happen on the lower left corner of the screen. */
  SDL_GL_GetDrawableSize(window, &width, &height);
  printf("width = %d, height = %d\n", width, height);
  glViewport(0, 0, width, height);

  glClear(GL_COLOR_BUFFER_BIT);

  glUseProgram(mainProgram);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(0);

  glDrawArrays(GL_TRIANGLES, 0, 3);
}

int main(int argc, char* argv[]) {
  int quit = 0;
  SDL_Event e;
  SDL_Window* window;
  SDL_GLContext gl;
  const GLubyte *version;

  printf("STARTING\n");
  loadOpenGL();

  /* Initialize SDL. */
  SDL_SetMainReady();
  if (SDL_Init(SDL_INIT_VIDEO) < 0) return 1;

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

  /* Create the window where we will draw. */
  printf("BEFORE WINDOW CREATION\n");
  window = SDL_CreateWindow("SDL_RenderClear",
                            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                            SCREEN_WIDTH, SCREEN_HEIGHT,
                            SDL_WINDOW_OPENGL|SDL_WINDOW_ALLOW_HIGHDPI);
  printf("AFTER WINDOW CREATION\n");
  gl = SDL_GL_CreateContext(window);
  if (!gl) {
    printf("SDL_GL_CreateContext failed: %s\n", SDL_GetError());
    return 1;
  }
  printf("gl/context = %p\n", (void*)gl);

  version = glGetString(GL_VERSION);
  printf("version = %s\n", version);

  initialize();
  draw(window);
  SDL_GL_SwapWindow(window);

  /* Hack to get window to stay up */
  while (quit == 0){
    while(SDL_PollEvent(&e)){
      if(e.type == SDL_QUIT) quit = 1;
    }

#ifdef __EMSCRIPTEN__
    emscripten_sleep(16);
#else
    SDL_Delay(16);
#endif
  }

  SDL_DestroyWindow(window);

  /* Always be sure to clean up */
  SDL_Quit();
  return 0;
}
