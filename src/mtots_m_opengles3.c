#include "mtots_m_opengles3.h"

#if MTOTS_ENABLE_OPENGLES3

#include "mtots_common.h"
#include "mtots_value.h"
#include "mtots_object.h"
#include "mtots_vm.h"
#include "mtots_memory.h"
#include "mtots_m_opengles3_gen.h"
#include "mtots_m_opengles3_dat.h"

#include <GLES3/gl3.h>

static ubool implBufferData(i16 argCount, Value *args, Value *out) {
  GLenum target = AS_U32(args[0]);
  ObjByteArray *ba = AS_BYTE_ARRAY(args[1]);
  GLenum usage = AS_U32(args[2]);
  glBufferData(target, ba->size, ba->buffer, usage);
  return UTRUE;
}

static TypePattern argsBufferData[] = {
  { TYPE_PATTERN_NUMBER },
  { TYPE_PATTERN_BYTE_ARRAY },
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcBufferData = {
  implBufferData, "bufferData",
  sizeof(argsBufferData)/sizeof(TypePattern), 0, argsBufferData,
};

static ubool implShaderSource(i16 argCount, Value *args, Value *out) {
  u32 shader = AS_U32(args[0]);
  const char *source = AS_STRING(args[1])->chars;
  glShaderSource(shader, 1, &source, NULL);
  return UTRUE;
}

static TypePattern argsShaderSource[] = {
  { TYPE_PATTERN_NUMBER },
  { TYPE_PATTERN_STRING },
};

static CFunction funcShaderSource = {
  implShaderSource, "shaderSource",
  sizeof(argsShaderSource)/sizeof(TypePattern), 0, argsShaderSource,
};

static ubool implGetShaderInfoLog(i16 argCount, Value *args, Value *out) {
  GLuint shader = (GLuint)AS_NUMBER(args[0]);
  GLint infoLogLength;
  char *buffer;
  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
  if (infoLogLength < 1) { /* Log is empty */
    return UTRUE;
  }
  buffer = ALLOCATE(char, infoLogLength);
  glGetShaderInfoLog(shader, infoLogLength, NULL, buffer);
  *out = OBJ_VAL(takeString(buffer, infoLogLength - 1));
  return UTRUE;
}

static TypePattern argsGetShaderInfoLog[] = {
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcGetShaderInfoLog = {
  implGetShaderInfoLog, "getShaderInfoLog",
  sizeof(argsGetShaderInfoLog)/sizeof(TypePattern), 0,
  argsGetShaderInfoLog,
};

static ubool implGetProgramInfoLog(i16 argCount, Value *args, Value *out) {
  GLuint program = (GLuint)AS_NUMBER(args[0]);
  GLint infoLogLength;
  char *buffer;
  glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
  if (infoLogLength < 1) { /* Log is empty */
    return UTRUE;
  }
  buffer = ALLOCATE(char, infoLogLength);
  glGetProgramInfoLog(program, infoLogLength, NULL, buffer);
  *out = OBJ_VAL(takeString(buffer, infoLogLength - 1));
  return UTRUE;
}

static TypePattern argsGetProgramInfoLog[] = {
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcGetProgramInfoLog = {
  implGetProgramInfoLog, "getProgramInfoLog",
  sizeof(argsGetProgramInfoLog)/sizeof(TypePattern), 0,
  argsGetProgramInfoLog,
};

static ubool implGetString(i16 argCount, Value *args, Value *out) {
  *out = OBJ_VAL(copyCString((const char*)glGetString((GLenum)AS_NUMBER(args[0]))));
  return UTRUE;
}

static TypePattern argsGetString[] = {
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcGetString = {
  implGetString, "getString",
  sizeof(argsGetString)/sizeof(TypePattern), 0,
  argsGetString,
};

static CFunction *functions[] = {
  &funcBufferData,
  &funcShaderSource,
  &funcGetShaderInfoLog,
  &funcGetProgramInfoLog,
  &funcGetString,
};

static ubool impl(i16 argCount, Value *args, Value *out) {
  ObjInstance *module = AS_INSTANCE(args[0]);
  size_t i;

  for (i = 0; i < sizeof(functions)/sizeof(CFunction*); i++) {
    tableSetN(&module->fields, functions[i]->name, CFUNCTION_VAL(functions[i]));
  }

  for (i = 0; i < sizeof(genFunctions)/sizeof(CFunction*); i++) {
    tableSetN(&module->fields, genFunctions[i]->name, CFUNCTION_VAL(genFunctions[i]));
  }

  for (i = 0; i < sizeof(constants)/sizeof(OpenGLConstant); i++) {
    OpenGLConstant c = constants[i];
    tableSetN(&module->fields, c.name, NUMBER_VAL(c.value));
  }

  tableSetN(&module->fields, "TRUE", BOOL_VAL(GL_TRUE));
  tableSetN(&module->fields, "FALSE", BOOL_VAL(GL_FALSE));

  return UTRUE;
}

static CFunction func = { impl, "opengles3", 1 };

void addNativeModuleOpenGLES3() {
  addNativeModule(&func);
}

#else
void addNativeModuleOpenGLES3() {
}
#endif
