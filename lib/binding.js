var native = require('../build/Release/rucksack');

function FakeBuffer() {
  this.length = null;
  this.parent = null;
  this.offset = null;
}

require('util').inherits(FakeBuffer, Buffer);

exports.pack = function(input) {
  var fast = new FakeBuffer(),
      slow = native.pack(input, fast);

  fast.parent = slow;
  fast.offset = slow[0] | slow[1] << 8;
  fast.length = slow[2] | slow[3] << 8;

  return fast;
};
