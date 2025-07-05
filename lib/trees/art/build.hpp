#pragma once

#include <cstdint>

#include "lib/common/types.hpp"
#include "update_inplace.hpp"

namespace art {

nodeptr build(uint64_t n, kv* elems) {
  nodeptr t = nullptr;
  for (uint64_t i = 0; i < n; i++) {
    t = update_inplace(t, 0, elems[i].first, elems[i].second); }
  return t;
}

}
