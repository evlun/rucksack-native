#include "pack.h"

#include <node.h>
#include <node_buffer.h>
#include <v8.h>

using namespace node;
using namespace v8;

namespace rucksack {

static OutputBuffer* out = new OutputBuffer();

static Handle<Value> Pack(const Arguments& args) {
  pack(args[0], out);
  return out->Flush(args[1]->ToObject());
}

extern "C" {
  void init(Handle<Object> target) {
    NODE_SET_METHOD(target, "pack", Pack);
  }

  NODE_MODULE(rucksack, init);
}

} // namespace rucksack
