/**
 * We assume that our platform is little endian.
 */

#include "mtots_util_buffer.h"

#include "mtots_util_error.h"

#include <stdlib.h>
#include <string.h>

/**
 * For now, we assume that we're always little endian.
 */
#define PLATFORM_BYTE_ORDER LITTLE_ENDIAN

/** Reverse the bytes if the given byte order does not match
 * the platform byte order */
static void maybeReverseBytes(
    u8 *data, size_t length, ByteOrder byteOrder) {
  if (byteOrder != PLATFORM_BYTE_ORDER) {
    size_t i, j;
    if (length) {
      for (i = 0, j = length - 1; i < j; i++, j--) {
        u8 tmp = data[i];
        data[i] = data[j];
        data[j] = tmp;
      }
    }
  }
}

static void checkIndex(Buffer *buf, size_t pos, size_t length) {
  if (pos + length < pos || pos + length > buf->length) {
    panic(
      "Buffer index out of bounds (buflen = %lu, pos = %lu, length = %lu)",
      (unsigned long)buf->length,
      (unsigned long)pos,
      (unsigned long)length);
  }
}

static void getDataWithByteOrder(Buffer *buf, size_t pos, void *dest, size_t length) {
  checkIndex(buf, pos, length);
  memcpy(dest, buf->data + pos, length);
  maybeReverseBytes((u8*)dest, length, buf->byteOrder);
}

static void setDataWithByteOrder(Buffer *buf, size_t pos, void *src, size_t length) {
  bufferSetBytes(buf, pos, src, length);
  maybeReverseBytes(buf->data + pos, length, buf->byteOrder);
}

static void addBufferSize(Buffer *buf, size_t length) {
  if (buf->isLocked) {
    panic("Cannot increase the size of a locked Buffer");
  }
  if (buf->length + length > buf->capacity) {
    do {
      buf->capacity = buf->capacity < 8 ? 8 : 2 * buf->capacity;
    } while (buf->length + length > buf->capacity);
    buf->data = (u8*)realloc(buf->data, buf->capacity);
  }
  buf->length += length;
}

static void addDataWithByteOrder(Buffer *buf, void *src, size_t length) {
  size_t pos = buf->length;
  addBufferSize(buf, length);
  setDataWithByteOrder(buf, pos, src, length);
}

void initBuffer(Buffer *buf) {
  buf->data = NULL;
  buf->length = buf->capacity = 0;
  buf->byteOrder = LITTLE_ENDIAN;
  buf->isLocked = UFALSE;
}

void bufferLock(Buffer *buf) {
  buf->isLocked = UTRUE;
}

void bufferSetLength(Buffer *buf, size_t newLength) {
  if (buf->isLocked) {
    panic("Cannot change the size of a locked buffer");
  }
  if (newLength <= buf->length) {
    buf->length = newLength;
  } else {
    size_t i, oldLength = buf->length;
    addBufferSize(buf, newLength - buf->length);
    for (i = oldLength; i < newLength; i++) {
      buf->data[i] = 0;
    }
  }
}

void freeBuffer(Buffer *buf) {
  free(buf->data);
}

u8 bufferGetU8(Buffer *buf, size_t pos) {
  checkIndex(buf, pos, 1);
  return buf->data[pos];
}

u16 bufferGetU16(Buffer *buf, size_t pos) {
  u16 value;
  getDataWithByteOrder(buf, pos, (void*)&value, 2);
  return value;
}

u32 bufferGetU32(Buffer *buf, size_t pos) {
  u32 value;
  getDataWithByteOrder(buf, pos, (void*)&value, 4);
  return value;
}

i8 bufferGetI8(Buffer *buf, size_t pos) {
  checkIndex(buf, pos, 1);
  return (i8)buf->data[pos];
}

i16 bufferGetI16(Buffer *buf, size_t pos) {
  i16 value;
  getDataWithByteOrder(buf, pos, (void*)&value, 2);
  return value;
}

i32 bufferGetI32(Buffer *buf, size_t pos) {
  i32 value;
  getDataWithByteOrder(buf, pos, (void*)&value, 4);
  return value;
}

f32 bufferGetF32(Buffer *buf, size_t pos) {
  f32 value;
  getDataWithByteOrder(buf, pos, (void*)&value, 4);
  return value;
}

f64 bufferGetF64(Buffer *buf, size_t pos) {
  f64 value;
  getDataWithByteOrder(buf, pos, (void*)&value, 8);
  return value;
}

void bufferSetU8(Buffer *buf, size_t pos, u8 value) {
  setDataWithByteOrder(buf, pos, (void*)&value, 1);
}

void bufferSetU16(Buffer *buf, size_t pos, u16 value) {
  setDataWithByteOrder(buf, pos, (void*)&value, 2);
}

void bufferSetU32(Buffer *buf, size_t pos, u32 value) {
  setDataWithByteOrder(buf, pos, (void*)&value, 4);
}

void bufferSetI8(Buffer *buf, size_t pos, i8 value) {
  setDataWithByteOrder(buf, pos, (void*)&value, 1);
}

void bufferSetI16(Buffer *buf, size_t pos, i16 value) {
  setDataWithByteOrder(buf, pos, (void*)&value, 2);
}

void bufferSetI32(Buffer *buf, size_t pos, i32 value) {
  setDataWithByteOrder(buf, pos, (void*)&value, 4);
}

void bufferSetF32(Buffer *buf, size_t pos, f32 value) {
  setDataWithByteOrder(buf, pos, (void*)&value, 4);
}

void bufferSetF64(Buffer *buf, size_t pos, f64 value) {
  setDataWithByteOrder(buf, pos, (void*)&value, 8);
}

void bufferSetBytes(Buffer *buf, size_t pos, void *data, size_t length) {
  checkIndex(buf, pos, length);
  memcpy((void*)(buf->data + pos), data, length);
}

void bufferAddU8(Buffer *buf, u8 value) {
  addDataWithByteOrder(buf, (void*)&value, 1);
}

void bufferAddU16(Buffer *buf, u16 value) {
  addDataWithByteOrder(buf, (void*)&value, 2);
}

void bufferAddU32(Buffer *buf, u32 value) {
  addDataWithByteOrder(buf, (void*)&value, 4);
}

void bufferAddI8(Buffer *buf, i8 value) {
  addDataWithByteOrder(buf, (void*)&value, 1);
}

void bufferAddI16(Buffer *buf, i16 value) {
  addDataWithByteOrder(buf, (void*)&value, 2);
}

void bufferAddI32(Buffer *buf, i32 value) {
  addDataWithByteOrder(buf, (void*)&value, 4);
}

void bufferAddF32(Buffer *buf, f32 value) {
  addDataWithByteOrder(buf, (void*)&value, 4);
}

void bufferAddF64(Buffer *buf, f64 value) {
  addDataWithByteOrder(buf, (void*)&value, 8);
}

void bufferAddBytes(Buffer *buf, void *data, size_t length) {
  size_t pos = buf->length;
  addBufferSize(buf, length);
  bufferSetBytes(buf, pos, data, length);
}
