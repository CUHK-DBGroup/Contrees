#pragma once

#include <cstdint>

#include "lib/conctrl/context.hpp"
#include "node.hpp"

namespace betree {

enum operand {
  INIT            = conctrl::INIT,
  SEARCH_UPPER    = 0x1,
  SEARCH_ELASTIC  = 0x2,
  SEARCH_LOWER    = 0x3,
  SEARCH_SPLIT    = 0x4,
  DONE            = conctrl::DONE
};

struct alignas(128) context : conctrl::context<node> {
  uint8_t split_idx;
};

}
