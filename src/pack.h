#pragma once

#include <node.h>
#include <node_buffer.h>
#include <v8.h>

namespace rucksack {

static const size_t kBufferSize = 1024 * 8;

static const double kMaximumVarint =  9007199254740992;
static const double kMinimumVarint = -9007199254740992;

static const double kPositiveInfinity = 1.0 / 0.0;
static const double kNegativeInfinity = -1.0 / 0.0;

class OutputBuffer {
 private:
  v8::Persistent<v8::Object> buffer_;
  char* data_;
  uintptr_t start_;
  uintptr_t ptr_;
  uintptr_t limit_;

 public:
  OutputBuffer();
  ~OutputBuffer();

  void Allocate();

  void WriteByte(char);
  void WriteVarint(uint64_t);
  void WriteDouble(double);

  v8::Handle<v8::Object> Flush(v8::Handle<v8::Object> target);
};

void pack(v8::Handle<v8::Value> value, OutputBuffer* out);

} // namespace rucksack
