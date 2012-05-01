#pragma once

#include <node.h>
#include <node_buffer.h>
#include <v8.h>

namespace rucksack {

static const size_t kBufferSize = 1024 * 8;

static const double kMaximumVarint =  9007199254740992;
static const double kMinimumVarint = -9007199254740992;

static const double kPositiveInfinity =  1.0 / 0.0;
static const double kNegativeInfinity = -1.0 / 0.0;

void InitOutput();
void Allocate();

inline void WriteByte(uint8_t byte);
inline void WriteVarint(uint64_t value);
inline void WriteDouble(double value);

void Write(v8::Handle<v8::Value> value);
v8::Handle<v8::Object> Flush(v8::Handle<v8::Object> target);

} // namespace rucksack
