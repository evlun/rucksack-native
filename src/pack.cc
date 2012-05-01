#include "endian.h"
#include "pack.h"

#include <cmath> // modf
#include <cstring> // memcpy

#include <node.h>
#include <node_buffer.h>
#include <v8.h>

using namespace node;
using namespace v8;

namespace rucksack {


#define RUCKSACK_ALLOCATE(bytes)                                               \
  if (output.ptr > output.limit - bytes) {                                     \
    Allocate();                                                                \
  }


static struct {
  Persistent<Object> handle;
  uint8_t* data;
  uintptr_t last_flushed;
  uintptr_t ptr;
  uintptr_t limit;
} output;


void InitOutput() {
  Buffer* buffer = Buffer::New(kBufferSize);

  output.handle = Persistent<Object>::New(buffer->handle_);
  output.data = reinterpret_cast<uint8_t*>(Buffer::Data(buffer));
  output.last_flushed = 4;
  output.ptr = 4;
  output.limit = kBufferSize;
}


// @todo: this will crash and burn when attempting to serialize values that
//        won't fit in kBufferSize bytes
void Allocate() {
  uint8_t* previous = output.data + output.last_flushed;
  size_t length = static_cast<size_t>(output.ptr - output.last_flushed);

  // determine whether or not we need to move data from the current buffer to
  // the new, by comparing the current `output.ptr` value to when `Flush()` was
  // last called
  bool move = output.ptr != output.last_flushed;

  Buffer* buffer = Buffer::New(kBufferSize);

  // free the old SlowBuffer for garbage collection
  output.handle.Dispose();
  output.handle = Persistent<Object>::New(buffer->handle_);

  output.data = reinterpret_cast<uint8_t*>(Buffer::Data(buffer));
  output.last_flushed = 4;
  output.limit = kBufferSize;

  if (move) {
    memcpy(output.data + output.last_flushed, previous, length);
    output.ptr = 4 + length;
  } else {
    output.ptr = 4;
  }
}


void WriteByte(uint8_t byte) {
  RUCKSACK_ALLOCATE(1)
  output.data[output.ptr++] = byte;
}


void WriteVarint(uint64_t v) {
  // we optimize for lower values of `v` because they're going to be much more
  // common than larger ones

  if (v < 128) {
    RUCKSACK_ALLOCATE(1)
    output.data[output.ptr++] = static_cast<uint8_t>(v);
  } else if (v < 16384) {
    RUCKSACK_ALLOCATE(2)
    output.data[output.ptr++] = static_cast<uint8_t>(v      ) | 0x80;
    output.data[output.ptr++] = static_cast<uint8_t>(v >>  7) & 0x7f;
  } else if (v < 2097152) {
    RUCKSACK_ALLOCATE(3)
    output.data[output.ptr++] = static_cast<uint8_t>(v      ) | 0x80;
    output.data[output.ptr++] = static_cast<uint8_t>(v >>  7) | 0x80;
    output.data[output.ptr++] = static_cast<uint8_t>(v >> 14) & 0x7f;
  } else if (v < 268435456) {
    RUCKSACK_ALLOCATE(4)
    output.data[output.ptr++] = static_cast<uint8_t>(v      ) | 0x80;
    output.data[output.ptr++] = static_cast<uint8_t>(v >>  7) | 0x80;
    output.data[output.ptr++] = static_cast<uint8_t>(v >> 14) | 0x80;
    output.data[output.ptr++] = static_cast<uint8_t>(v >> 21) & 0x7f;
  } else if (v < 34359738368) {
    RUCKSACK_ALLOCATE(5)
    output.data[output.ptr++] = static_cast<uint8_t>(v      ) | 0x80;
    output.data[output.ptr++] = static_cast<uint8_t>(v >>  7) | 0x80;
    output.data[output.ptr++] = static_cast<uint8_t>(v >> 14) | 0x80;
    output.data[output.ptr++] = static_cast<uint8_t>(v >> 21) | 0x80;
    output.data[output.ptr++] = static_cast<uint8_t>(v >> 28) & 0x7f;
  } else if (v < 4398046511104) {
    RUCKSACK_ALLOCATE(6)
    output.data[output.ptr++] = static_cast<uint8_t>(v      ) | 0x80;
    output.data[output.ptr++] = static_cast<uint8_t>(v >>  7) | 0x80;
    output.data[output.ptr++] = static_cast<uint8_t>(v >> 14) | 0x80;
    output.data[output.ptr++] = static_cast<uint8_t>(v >> 21) | 0x80;
    output.data[output.ptr++] = static_cast<uint8_t>(v >> 28) | 0x80;
    output.data[output.ptr++] = static_cast<uint8_t>(v >> 35) & 0x7f;
  } else if (v < 562949953421312) {
    RUCKSACK_ALLOCATE(7)
    output.data[output.ptr++] = static_cast<uint8_t>(v      ) | 0x80;
    output.data[output.ptr++] = static_cast<uint8_t>(v >>  7) | 0x80;
    output.data[output.ptr++] = static_cast<uint8_t>(v >> 14) | 0x80;
    output.data[output.ptr++] = static_cast<uint8_t>(v >> 21) | 0x80;
    output.data[output.ptr++] = static_cast<uint8_t>(v >> 28) | 0x80;
    output.data[output.ptr++] = static_cast<uint8_t>(v >> 35) | 0x80;
    output.data[output.ptr++] = static_cast<uint8_t>(v >> 42) & 0x7f;
  } else {
    RUCKSACK_ALLOCATE(8)
    output.data[output.ptr++] = static_cast<uint8_t>(v      ) | 0x80;
    output.data[output.ptr++] = static_cast<uint8_t>(v >>  7) | 0x80;
    output.data[output.ptr++] = static_cast<uint8_t>(v >> 14) | 0x80;
    output.data[output.ptr++] = static_cast<uint8_t>(v >> 21) | 0x80;
    output.data[output.ptr++] = static_cast<uint8_t>(v >> 28) | 0x80;
    output.data[output.ptr++] = static_cast<uint8_t>(v >> 35) | 0x80;
    output.data[output.ptr++] = static_cast<uint8_t>(v >> 42) | 0x80;
    output.data[output.ptr++] = static_cast<uint8_t>(v >> 49) & 0x7f;
  }
}


void WriteDouble(double v) {
  RUCKSACK_ALLOCATE(8)

  uint8_t* dest = output.data + output.ptr;
  uint8_t* src = reinterpret_cast<uint8_t*>(&v);

#if BYTE_ORDER == LITTLE_ENDIAN
  dest[0] = src[0]; dest[1] = src[1]; dest[2] = src[2]; dest[3] = src[3];
  dest[4] = src[4]; dest[5] = src[5]; dest[6] = src[6]; dest[7] = src[7];
#else
  dest[0] = src[7]; dest[1] = src[6]; dest[2] = src[5]; dest[3] = src[4];
  dest[4] = src[3]; dest[5] = src[2]; dest[6] = src[1]; dest[7] = src[0];
#endif

  output.ptr += 8;
}


void Write(v8::Handle<v8::Value> v) {
  if (v->IsNumber()) {
    double d = v->NumberValue();
    double t; // dummy variable for `modf(d, &t)`

    // infinities and NaNs
    if (d == kPositiveInfinity) {
      WriteByte(0xa6);
    } else if (d == kNegativeInfinity) {
      WriteByte(0xa7);
    } else if (d != d) {
      WriteByte(0xa5);
    }

    // integers
    else if (d <= kMaximumVarint && d >= kMinimumVarint && modf(d, &t) == 0.0) {
      long long i = v->IntegerValue();

      if (i >= 0 && (d != 0 || 1.0 / d >= 0.0)) {
        if (i < 128) {
          WriteByte(static_cast<uint8_t>(i));
        } else {
          WriteByte(0xa8);
          WriteVarint(static_cast<uint64_t>(i - 128));
        }
      } else {
        if (i > -32) {
          WriteByte(static_cast<uint8_t>(0x80 - i));
        } else {
          WriteByte(0xa9);
          WriteVarint(static_cast<uint64_t>(-i - 32));
        }
      }
    }

    // floating point values, or particularly large integers
    else {
      WriteByte(0xaa);
      WriteDouble(d);
    }
  }

  else if (v->IsString()) {
    String::Utf8Value utf8(v);
    int length = utf8.length();

    if (length < 31) {
      WriteByte(0xe0 + length);
    } else {
      WriteByte(0xff);
      WriteVarint(length - 31);
    }

    RUCKSACK_ALLOCATE(length);
    memcpy(output.data + output.ptr, *utf8, length);
    output.ptr += length;
  }

  else {
    // @todo: everything else -- default to undefined for now
    WriteByte(0xa1);
  }
}


v8::Handle<v8::Object> Flush(v8::Handle<v8::Object> target) {
  uint16_t offset = static_cast<uint16_t>(output.last_flushed);
  uint16_t length = static_cast<uint16_t>(output.ptr - output.last_flushed);

  output.data[0] = static_cast<uint8_t>(offset     ) & 0xff;
  output.data[1] = static_cast<uint8_t>(offset >> 8) & 0xff;
  output.data[2] = static_cast<uint8_t>(length     ) & 0xff;
  output.data[3] = static_cast<uint8_t>(length >> 8) & 0xff;

  target->SetIndexedPropertiesToExternalArrayData(
    output.data + output.last_flushed, kExternalUnsignedByteArray, length);

  // update how much of the buffer has been flushed already
  output.last_flushed = output.ptr;

  // return the SlowBuffer object
  return output.handle;
}

} // namespace rucksack
