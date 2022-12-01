#ifndef mtots_util_buffer_h
#define mtots_util_buffer_h

#include "mtots_common.h"

typedef enum ByteOrder {
  LITTLE_ENDIAN,
  BIG_ENDIAN
} ByteOrder;

typedef struct Buffer {
  u8 *data;
  size_t length, capacity;
  ByteOrder byteOrder;
} Buffer;

void initBuffer(Buffer *buf);
void freeBuffer(Buffer *buf);
u8 bufferGetU8(Buffer *buf, size_t pos);
u16 bufferGetU16(Buffer *buf, size_t pos);
u32 bufferGetU32(Buffer *buf, size_t pos);
i8 bufferGetI8(Buffer *buf, size_t pos);
i16 bufferGetI16(Buffer *buf, size_t pos);
i32 bufferGetI32(Buffer *buf, size_t pos);
f32 bufferGetF32(Buffer *buf, size_t pos);
f64 bufferGetF64(Buffer *buf, size_t pos);
void bufferSetU8(Buffer *buf, size_t pos, u8 value);
void bufferSetU16(Buffer *buf, size_t pos, u16 value);
void bufferSetU32(Buffer *buf, size_t pos, u32 value);
void bufferSetI8(Buffer *buf, size_t pos, i8 value);
void bufferSetI16(Buffer *buf, size_t pos, i16 value);
void bufferSetI32(Buffer *buf, size_t pos, i32 value);
void bufferSetF32(Buffer *buf, size_t pos, f32 value);
void bufferSetF64(Buffer *buf, size_t pos, f64 value);
void bufferSetBytes(Buffer *buf, size_t pos, void *data, size_t length);
void bufferAddU8(Buffer *buf, u8 value);
void bufferAddU16(Buffer *buf, u16 value);
void bufferAddU32(Buffer *buf, u32 value);
void bufferAddI8(Buffer *buf, i8 value);
void bufferAddI16(Buffer *buf, i16 value);
void bufferAddI32(Buffer *buf, i32 value);
void bufferAddF32(Buffer *buf, f32 value);
void bufferAddF64(Buffer *buf, f64 value);
void bufferAddBytes(Buffer *buf, void *data, size_t length);

#endif/*mtots_util_buffer_h*/
