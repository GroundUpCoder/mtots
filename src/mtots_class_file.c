#include "mtots_class_file.h"
#include "mtots_vm.h"

#include <string.h>
#include <stdlib.h>

static ubool implFileWrite(i16 argCount, Value *args, Value *out) {
  Value receiver = args[-1], arg = args[0];
  ObjFile *file;
  String *str;
  size_t writeSize;
  if (!IS_FILE(receiver)) {
    runtimeError("Expected file as receiver to File.write()");
    return UFALSE;
  }
  file = AS_FILE(receiver);
  if (!IS_STRING(arg)) {
    runtimeError("Expected string to write in File.write()");
    return UFALSE;
  }
  str = AS_STRING(arg);
  writeSize = fwrite(str->chars, sizeof(char), str->length, file->file);
  *out = NUMBER_VAL(writeSize);
  return UTRUE;
}

static CFunction funcFileWrite = { implFileWrite, "write", 1 };

static String *readAll(FILE *fin) {
  char buffer[FREAD_BUFFER_SIZE], *str = NULL;
  size_t nread, size = 0;
  nread = fread(buffer, 1, FREAD_BUFFER_SIZE, fin);
  while (nread == FREAD_BUFFER_SIZE) {
    size_t oldSize = size;
    size += FREAD_BUFFER_SIZE;
    str = (char*)realloc(str, size);
    memcpy(str + oldSize, buffer, nread);
    nread = fread(buffer, 1, FREAD_BUFFER_SIZE, fin);
  }
  str = (char*)realloc(str, size + nread + 1);
  memcpy(str + size, buffer, nread);
  size += nread;
  str[size] = '\0';
  return internOwnedString(str, size);
}

static ubool readBytes(FILE *fin, size_t count, String **out) {
  char *buffer = malloc(sizeof(char) * (count + 1));
  size_t nread;
  clearerr(fin);
  nread = fread(buffer, 1, count, fin);
  if (nread != count) {
    if (ferror(fin)) {
      runtimeError("Error while trying to read bytes");
      return UFALSE;
    }
    runtimeError(
      "Tried to read %lu bytes but got %lu",
      (unsigned long) count, (unsigned long) nread);
    return UFALSE;
  }
  buffer[count] = '\0';
  *out = internOwnedString(buffer, count);
  return UTRUE;
}

static ubool implFileRead(i16 argCount, Value *args, Value *out) {
  Value receiver = args[-1];
  ObjFile *file;
  if (!IS_FILE(receiver)) {
    runtimeError("Expected file as receiver to File.write()");
    return UFALSE;
  }
  file = AS_FILE(receiver);

  /* Read an exact specified number of bytes */
  if (argCount == 1) {
    double count;
    ubool result;
    String *outstr = NULL;
    if (!IS_NUMBER(args[0])) {
      runtimeError(
        "File.write() requires number, but got %s",
        getKindName(args[0]));
      return UFALSE;
    }
    count = AS_NUMBER(args[0]);
    if (count < 0) {
      runtimeError(
        "File.wrie() requires a positive number but got %f",
        count);
      return UFALSE;
    }
    result = readBytes(file->file, count, &outstr);
    if (result) {
      *out = STRING_VAL(outstr);
      return UTRUE;
    }
    return UFALSE;
  }

  /* Read the entire string */
  clearerr(file->file);
  *out = STRING_VAL(readAll(file->file));
  if (ferror(file->file)) {
    runtimeError("Error reading file %s", file->name);
    return UFALSE;
  }
  return UTRUE;
}

static CFunction funcFileRead = { implFileRead, "read", 0, 1 };

static ubool implFileClose(i16 argCount, Value *args, Value *out) {
  Value receiver = args[-1];
  ObjFile *file;
  if (!IS_FILE(receiver)) {
    runtimeError("Expected file as receiver to File.write()");
    return UFALSE;
  }
  file = AS_FILE(receiver);
  if (file->isOpen) {
    fclose(file->file);
    file->isOpen = UFALSE;
  }
  return UTRUE;
}

static CFunction funcFileClose = { implFileClose, "close", 0 };

void initFileClass() {
  String *tmpstr;
  CFunction *methods[] = {
    &funcFileWrite,
    &funcFileRead,
    &funcFileClose
  };
  size_t i;
  ObjClass *cls;

  tmpstr = internCString("File");
  push(STRING_VAL(tmpstr));
  cls = vm.fileClass = newClass(tmpstr);
  cls->isBuiltinClass = UTRUE;
  pop();

  for (i = 0; i < sizeof(methods) / sizeof(CFunction*); i++) {
    tmpstr = internCString(methods[i]->name);
    push(STRING_VAL(tmpstr));
    mapSetStr(
      &cls->methods, tmpstr, CFUNCTION_VAL(methods[i]));
    pop();
  }
}
