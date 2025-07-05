#pragma once

#include <cstdint>

#include "node.hpp"

namespace aert {

static inline nodeptr new_leaf(uint64_t ver, uint8_t pfx_ofs, uint64_t key, uint64_t val) {
  if (pfx_ofs % 8 == 4) {
    nodeptr t = new_node(ver, LEAF, pfx_ofs+4, KEY_BIT_LEN-4-pfx_ofs, key);
    ((leaf*)t)->val = val;
    t = embed_ptr(t, partial_key(key, pfx_ofs, 4) | 0x10);
    return t; }
  else {
    nodeptr t = new_node(ver, LEAF, pfx_ofs, KEY_BIT_LEN-pfx_ofs, key);
    ((leaf*)t)->val = val;
    return t; }
}

static inline nodeptr leaf_update(uint64_t ver, nodeptr t_old, uint64_t val) {
  nodeptr t_new = node_copy(t_old, ver);
  ((leaf*)t_new)->val = val;
  return t_new;
}

}
