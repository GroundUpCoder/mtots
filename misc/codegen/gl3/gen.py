import os
import sys

scriptDir = os.path.dirname(os.path.realpath(__file__))
# headerPath = os.path.join(scriptDir, 'header.h')
headerPath = os.path.join(scriptDir, 'filtered.h')

baseNames = []

unsupported = {
  'const void*',
  'const GLchar*const*',
  'void*',
  'void**',
  'GLsizei*',
  'GLboolean*',
  'GLint*',
  'GLuint*',
  'GLfloat*',
  'GLsync',
  'GLint64*',
  'const GLfloat*',
  'const GLint*',
  'const GLuint*',
  'const GLenum*',
  'const GLint64*',
}

unsupportedFunctions = {
  # Not supported in webgl
  'glFlushMappedBufferRange',
  'glUnmapBuffer',
}

# These are functions that accept 'n' and a pointer to 'GLuint's.
# While translating these functions exactly as is would be a bit
# cumbersome to use, their singular versions (i.e. when n = 1)
# are often very useful.
singularizableFunctions = {
  # GLES 2
  'glDeleteBuffers',
  'glDeleteFramebuffers',
  'glDeleteRenderbuffers',
  'glDeleteTextures',
  'glDeleteBuffers',
  'glDeleteFramebuffers',
  'glDeleteRenderbuffers',
  'glDeleteTextures',
  'glGenBuffers',
  'glGenFramebuffers',
  'glGenRenderbuffers',
  'glGenTextures',

  # GLES 3.0
  'glDeleteQueries',
  'glDeleteVertexArrays',
  'glGenQueries',
  'glGenVertexArrays',
}

# These are functions that end with 'iv' or 'fv'
# but the final parameter is always a singleton
ivfvFunctions = {
  'glGetBufferParameteriv',
  'glGetFramebufferAttachmentParameteriv',
  'glGetProgramiv',
  'glGetRenderbufferParameteriv',
  'glGetShaderiv',
  'glGetTexParameteriv',
  'glGetUniformiv',
  'glGetVertexAttribiv',
  'glTexParameteriv',
  'glGetTexParameterfv',
  'glGetUniformfv',
  'glGetVertexAttribfv',
  'glTexParameterfv',
}

print(r"""#ifndef mtots_m_opengles3_gen_h
#define mtots_m_opengles3_gen_h
/* Autogenerated and should only be included exactly
 * once by mtots_m_opengles3.c
 *
 * Not all functions are included, but all functions that have
 * non-pointer scalar argument and return values are included.
 *
 * See `misc/codegen/gl3/gen.py` for more info on how this file
 * was generated */
#include "mtots_m_opengles3.h"
#include "mtots_vm.h"
#include <GLES3/gl3.h>
""")

def handleivfv():
  baseNames.append(baseName)
  assert args[-1][0] in ('GLint*', 'const GLint*', 'GLfloat*', 'const GLfloat*'), args
  assert all('*' not in arg[0] for arg in args[:-1]), args
  assert returnType == 'void', returnType

  # The function returns iff the last parameter is not a 'const'
  returns = not args[-1][0].startswith('const ')

  fmtArgs = ', '.join(arg[1] for arg in args[:-1]) + f', &{args[-1][1]}'
  call = f'{glFuncName}({fmtArgs})'

  ## impl
  print(f'static ubool impl{baseName}(i16 argCount, Value *args, Value *out) {{')
  for i, (argType, argName) in enumerate(args[:-1]):
    emitInitArg(i, argType, argName)
  if returns:
    if args[-1][0].endswith('GLfloat*'):
      print(f'  GLfloat {args[-1][1]};')
    elif args[-1][0].endswith('GLint*'):
      print(f'  GLint {args[-1][1]};')
    else:
      raise Exception(f'Unrecognized out type in ivfv {args[-1][0]}')
  else:
    if args[-1][0].endswith('GLfloat*'):
      print(f'  GLfloat {args[-1][1]} = (GLfloat)AS_NUMBER(args[{len(args) - 1}]);')
    elif args[-1][0].endswith('GLint*'):
      print(f'  GLint {args[-1][1]} = (GLint)AS_NUMBER(args[{len(args) - 1}]);')
    else:
      raise Exception(f'Unrecognized out type in ivfv {args[-1][0]}')
  print(f'  {call};')
  if returns:
    print(f'  *out = NUMBER_VAL({args[-1][1]});')
  print('  return UTRUE;')
  print('}')

  ## args
  print(f'static TypePattern args{baseName}[] = {{')
  for argType, _ in args[:-1]:
    emitArgTypePattern(argType)
  if not returns:
    print('  { TYPE_PATTERN_NUMBER },')
  print('};')

  ## func
  print(f'static CFunction func{baseName} = {{')
  print(f'  impl{baseName}, "{mtotsName}",')
  print(f'  sizeof(args{baseName})/sizeof(TypePattern), 0, args{baseName},')
  print('};')


def handleSingularize():
  global baseName, mtotsName
  assert baseName.endswith('s'), baseName
  baseName = baseName[:-1]
  assert mtotsName.endswith('s'), mtotsName
  mtotsName = mtotsName[:-1]
  assert len(args) == 2, args
  assert args[0] == ('GLsizei', 'n'), args
  assert args[1][0] in ('GLuint*', 'const GLuint*'), args
  baseNames.append(baseName)

  # The function returns a value iff the second parameter is not a 'const'
  # value
  returns = not args[1][0].startswith('const ')

  ## impl
  print(f'static ubool impl{baseName}(i16 argCount, Value *args, Value *out) {{')
  if returns:
    print('  GLuint value;')
    print(f'  {glFuncName}(1, &value);')
    print('  *out = NUMBER_VAL(value);')
  else:
    print('  GLuint value = AS_U32(args[0]);')
    print(f'  {glFuncName}(1, &value);')
  print('  return UTRUE;')
  print('}')

  ## args and func
  if returns:
    # zero args
    print(f'static CFunction func{baseName} = {{')
    print(f'  impl{baseName}, "{mtotsName}",')
    print('};')
  else:
    # one or more args
    print(f'static TypePattern args{baseName}[] = {{')
    print('  { TYPE_PATTERN_NUMBER },')
    print('};')
    print(f'static CFunction func{baseName} = {{')
    print(f'  impl{baseName}, "{mtotsName}",')
    print(f'  sizeof(args{baseName})/sizeof(TypePattern), 0, args{baseName},')
    print('};')


def emitInitArg(i, argType, argName):
  if argType == 'GLboolean':
    print(f'  ubool {argName} = AS_BOOL(args[{i}]);')
  elif argType == 'GLsizeiptr':
    print(f'  size_t {argName} = (size_t)AS_NUMBER(args[{i}]);')
  elif argType in ('GLenum', 'GLuint', 'GLbitfield'):
    print(f'  u32 {argName} = AS_U32(args[{i}]);')
  elif argType in ('GLint', 'GLsizei'):
    print(f'  i32 {argName} = AS_I32(args[{i}]);')
  elif argType == 'GLintptr':
    print(f'  long {argName} = (long)AS_NUMBER(args[{i}]);')
  elif argType == 'GLfloat':
    print(f'  float {argName} = (float)AS_NUMBER(args[{i}]);')
  elif argType == 'const GLchar*':
    print(f'  const char *{argName} = AS_STRING(args[{i}])->chars;')
  elif argType == 'PtrOffset':
    print(f'  const void *{argName} = (void*)(size_t)AS_NUMBER(args[{i}]);')
  else:
    raise Exception(f"unrecognized parameter type {argType} (in {glFuncName})")


def emitArgTypePattern(argType):
  if argType == 'GLboolean':
    print('  { TYPE_PATTERN_BOOL },')
  elif argType in (
      'GLsizeiptr', 'GLsizei', 'GLintptr',
      'GLenum', 'GLuint', 'GLbitfield', 'GLint',
      'GLfloat', 'PtrOffset'):
    print('  { TYPE_PATTERN_NUMBER },')
  elif argType == 'const GLchar*':
    print('  { TYPE_PATTERN_STRING },')
  else:
    raise Exception(f"unrecognized paramter type (2) {argType}")


def handleNormal():
    baseNames.append(baseName)
    fmtArgs = ', '.join(arg[1] for arg in args)
    call = f'{glFuncName}({fmtArgs})'

    ## impl
    print(f'static ubool impl{baseName}(i16 argCount, Value *args, Value *out) {{')
    for i, (argType, argName) in enumerate(args):
      emitInitArg(i, argType, argName)

    if returnType == 'void':
      print(f'  {call};')
    elif returnType in ('GLenum', 'GLuint', 'GLint'):
      print(f'  *out = NUMBER_VAL({call});')
    elif returnType == 'GLboolean':
      print(f'  *out = BOOL_VAL({call});')
    else:
      raise Exception(f"unrecognized return type {returnType} (in {glFuncName})")
    print(f'  return UTRUE;')
    print('}')

    if args:
      ## args
      print(f'static TypePattern args{baseName}[] = {{')
      for argType, _ in args:
        emitArgTypePattern(argType)
      print('};')

      ## func
      print(f'static CFunction func{baseName} = {{')
      print(f'  impl{baseName}, "{mtotsName}",')
      print(f'  sizeof(args{baseName})/sizeof(TypePattern), 0, args{baseName},')
      print('};')
    else:
      print(f'static CFunction func{baseName} = {{')
      print(f'  impl{baseName}, "{mtotsName}",')
      print('};')


with open(headerPath) as f:
  for line in f:
    if line.startswith(('#include', 'typedef ', 'const ')):
      continue
    line = line.strip()
    assert line.endswith(');'), line
    line = line[:-len(');')]
    returnType, line = line.split(maxsplit=1)
    glFuncName, line = line.split('(')
    args = tuple(
      tuple(p.strip() for p in entry.rsplit(maxsplit=1))
      for entry in line.split(',')) if line else ()

    assert glFuncName.startswith('gl'), glFuncName
    baseName = glFuncName[len('gl'):]
    mtotsName = baseName[0].lower() + baseName[1:]

    if glFuncName in unsupportedFunctions:
      continue

    if glFuncName in ivfvFunctions:
      handleivfv()
      continue

    if glFuncName in singularizableFunctions:
      handleSingularize()
      continue

    # Skip if any of the types are unsupported
    if any(arg[0] in unsupported for arg in args) or returnType in unsupported:
      sys.stderr.write(f'skipping {glFuncName}\n')
      continue

    handleNormal()

print('static CFunction *genFunctions[] = {')
for baseName in baseNames:
  print(f'  &func{baseName},')
print('};')

print('#endif/*mtots_m_opengles3_gen_h*/')