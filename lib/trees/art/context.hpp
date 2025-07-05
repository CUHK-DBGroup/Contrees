#pragma once

#include <cstdint>

#include "lib/conctrl/context.hpp"
#include "node.hpp"

namespace art {

enum operand {
  INIT    = conctrl::INIT,
  SEARCH  = 0x1,
  INSERT  = 0x2,
  FORK    = 0x3,
  DONE    = conctrl::DONE
};

struct alignas(128) context : conctrl::context<node> {
  uint8_t key_ofs;
  uint8_t ch_idx;
};

}
