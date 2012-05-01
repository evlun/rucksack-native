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
  if (ptr_ > limit_ - bytes) {                                             \
    Allocate();                                                                \
  }

OutputBuffer::OutputBuffer() {
  Buffer* buf = Buffer::New(kBufferSize);
  buffer_ = Persistent<Object>::New(buf->handle_);
  data_ = Buffer::Data(buf);

  start_ = 4;
  ptr_ = 4;
  limit_ = kBufferSize;
}

OutputBuffer::~OutputBuffer() {
  buffer_.Dispose();
}

// @todo: this will fail horribly when attempting to serialize values that won't
//        fit in kBufferSize bytes
void OutputBuffer::Allocate() {
  bool copy = ptr_ != start_;
  char* previous = data_ + start_;
  size_t length = static_cast<size_t>(ptr_ - start_);

  // free the previous SlowBuffer for garbage collection
  buffer_.Dispose();

  // create a new Buffer instance
  Buffer* buf = Buffer::New(kBufferSize);

  buffer_ = Persistent<Object>::New(buf->handle_);
  data_ = Buffer::Data(buf);

  start_ = 4;
  ptr_ = 4;
  limit_ = kBufferSize;

  if (copy) {
    memcpy(previous, data_ + start_, length);
    ptr_ += length;
  }
}

void OutputBuffer::WriteByte(char byte) {
  RUCKSACK_ALLOCATE(1)
  data_[ptr_++] = byte;
}

void OutputBuffer::WriteVarint(uint64_t v) {
  // here we optimize for lower values of `v` because they're going to be much
  // more common than larger ones

  if (v < 128) {
    RUCKSACK_ALLOCATE(1)
    data_[ptr_++] = static_cast<uint8_t>(v);
  } else if (v < 16384) {
    RUCKSACK_ALLOCATE(2)
    data_[ptr_++] = static_cast<uint8_t>(v      ) | 0x80;
    data_[ptr_++] = static_cast<uint8_t>(v >>  7) & 0x7f;
  } else if (v < 2097152) {
    RUCKSACK_ALLOCATE(3)
    data_[ptr_++] = static_cast<uint8_t>(v      ) | 0x80;
    data_[ptr_++] = static_cast<uint8_t>(v >>  7) | 0x80;
    data_[ptr_++] = static_cast<uint8_t>(v >> 14) & 0x7f;
  } else if (v < 268435456) {
    RUCKSACK_ALLOCATE(4)
    data_[ptr_++] = static_cast<uint8_t>(v      ) | 0x80;
    data_[ptr_++] = static_cast<uint8_t>(v >>  7) | 0x80;
    data_[ptr_++] = static_cast<uint8_t>(v >> 14) | 0x80;
    data_[ptr_++] = static_cast<uint8_t>(v >> 21) & 0x7f;
  } else if (v < 34359738368) {
    RUCKSACK_ALLOCATE(5)
    data_[ptr_++] = static_cast<uint8_t>(v      ) | 0x80;
    data_[ptr_++] = static_cast<uint8_t>(v >>  7) | 0x80;
    data_[ptr_++] = static_cast<uint8_t>(v >> 14) | 0x80;
    data_[ptr_++] = static_cast<uint8_t>(v >> 21) | 0x80;
    data_[ptr_++] = static_cast<uint8_t>(v >> 28) & 0x7f;
  } else if (v < 4398046511104) {
    RUCKSACK_ALLOCATE(6)
    data_[ptr_++] = static_cast<uint8_t>(v      ) | 0x80;
    data_[ptr_++] = static_cast<uint8_t>(v >>  7) | 0x80;
    data_[ptr_++] = static_cast<uint8_t>(v >> 14) | 0x80;
    data_[ptr_++] = static_cast<uint8_t>(v >> 21) | 0x80;
    data_[ptr_++] = static_cast<uint8_t>(v >> 28) | 0x80;
    data_[ptr_++] = static_cast<uint8_t>(v >> 35) & 0x7f;
  } else if (v < 562949953421312) {
    RUCKSACK_ALLOCATE(7)
    data_[ptr_++] = static_cast<uint8_t>(v      ) | 0x80;
    data_[ptr_++] = static_cast<uint8_t>(v >>  7) | 0x80;
    data_[ptr_++] = static_cast<uint8_t>(v >> 14) | 0x80;
    data_[ptr_++] = static_cast<uint8_t>(v >> 21) | 0x80;
    data_[ptr_++] = static_cast<uint8_t>(v >> 28) | 0x80;
    data_[ptr_++] = static_cast<uint8_t>(v >> 35) | 0x80;
    data_[ptr_++] = static_cast<uint8_t>(v >> 42) & 0x7f;
  } else {
    RUCKSACK_ALLOCATE(8)
    data_[ptr_++] = static_cast<uint8_t>(v      ) | 0x80;
    data_[ptr_++] = static_cast<uint8_t>(v >>  7) | 0x80;
    data_[ptr_++] = static_cast<uint8_t>(v >> 14) | 0x80;
    data_[ptr_++] = static_cast<uint8_t>(v >> 21) | 0x80;
    data_[ptr_++] = static_cast<uint8_t>(v >> 28) | 0x80;
    data_[ptr_++] = static_cast<uint8_t>(v >> 35) | 0x80;
    data_[ptr_++] = static_cast<uint8_t>(v >> 42) | 0x80;
    data_[ptr_++] = static_cast<uint8_t>(v >> 49) & 0x7f;
  }
}

void OutputBuffer::WriteDouble(double v) {
  RUCKSACK_ALLOCATE(8);

  char* dest = data_ + ptr_;
  char* src = reinterpret_cast<char*>(&v);

#if BYTE_ORDER == LITTLE_ENDIAN
  dest[0] = src[0]; dest[1] = src[1]; dest[2] = src[2]; dest[3] = src[3];
  dest[4] = src[4]; dest[5] = src[5]; dest[6] = src[6]; dest[7] = src[7];
#else
  dest[0] = src[7]; dest[1] = src[6]; dest[2] = src[5]; dest[3] = src[4];
  dest[4] = src[3]; dest[5] = src[2]; dest[6] = src[1]; dest[7] = src[0];
#endif

  ptr_ += 8;
}

Handle<Object> OutputBuffer::Flush(Handle<Object> target) {
  uint16_t offset = static_cast<uint16_t>(start_);
  uint16_t length = static_cast<uint16_t>(ptr_ - start_);

  data_[0] = static_cast<uint8_t>((offset     ) & 0xff);
  data_[1] = static_cast<uint8_t>((offset >> 8) & 0xff);
  data_[2] = static_cast<uint8_t>((length     ) & 0xff);
  data_[3] = static_cast<uint8_t>((length >> 8) & 0xff);

  target->SetIndexedPropertiesToExternalArrayData(
    data_ + start_, kExternalUnsignedByteArray, length);

  // @todo: do proper stuff here
  ptr_ = 4;

  // return the SlowBuffer object
  return buffer_;
}

void pack(Handle<Value> v, OutputBuffer* out) {
  if (v->IsNumber()) {
    double d = v->NumberValue();
    double t; // modf(d, &t)

    // infinities and NaNs
    if (d == kPositiveInfinity) {
      out->WriteByte(0xa6);
    } else if (d == kNegativeInfinity) {
      out->WriteByte(0xa7);
    } else if (d != d) {
      out->WriteByte(0xa5);
    }

    // integers
    else if (d <= kMaximumVarint && d >= kMinimumVarint && modf(d, &t) == 0.0) {
      long long i = v->IntegerValue();

      if (i >= 0 && (d != 0 || 1.0 / d >= 0.0)) {
        if (i < 128) {
          out->WriteByte(static_cast<char>(i));
        } else {
          out->WriteByte(0xa8);
          out->WriteVarint(static_cast<uint64_t>(i - 128));
        }
      } else {
        if (i > -32) {
          out->WriteByte(static_cast<char>(0x80 - i));
        } else {
          out->WriteByte(0xa9);
          out->WriteVarint(static_cast<uint64_t>(-i - 32));
        }
      }
    }

    // floating point values, or particularly large integers
    else {
      out->WriteByte(0xaa);
      out->WriteDouble(d);
    }
  }

  else {
    // @todo: everything else -- default to undefined for now
    out->WriteByte(0xa1);
  }
}

} // namespace rucksack
