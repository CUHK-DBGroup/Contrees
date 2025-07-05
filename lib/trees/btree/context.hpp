#pragma once

#include <cstdint>

#include "lib/conctrl/context.hpp"
#include "node.hpp"

namespace btree {

enum operand {
  INIT         = conctrl::INIT,
  SEARCH_DOWN  = 0x1,
  SEARCH_SPLIT = 0x2,
  DONE         = conctrl::DONE
};

struct alignas(128) context : conctrl::context<node> {
  uint8_t split_idx;
};

}
