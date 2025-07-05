#pragma once

#include <cstdint>

#include "lib/common/types.hpp"
#include "update_inplace.hpp"

namespace aert {

nodeptr build(uint64_t n, std::pair<uint64_t, uint64_t>* elems) {
  nodeptr t = nullptr;
  for (uint64_t i = 0; i < n; i++) {
    t = update_inplace(t, 0, elems[i].first, elems[i].second); }
  return t;
}

}
