#include "pack.h"

#include <node.h>
#include <node_buffer.h>
#include <v8.h>

using namespace node;
using namespace v8;

namespace rucksack {

static Handle<Value> Pack(const Arguments& args) {
  // because this function should only get called from `lib/bindings.js`, we can
  // assume that the arguments are correctly formed

  Write(args[0]);
  return Flush(args[1]->ToObject());
}

extern "C" {
  void Init(Handle<Object> target) {
    InitOutput();
    NODE_SET_METHOD(target, "pack", Pack);
  }

  NODE_MODULE(rucksack, Init);
}

} // namespace rucksack
