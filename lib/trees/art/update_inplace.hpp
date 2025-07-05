#pragma once

#include <cstdint>

#include "lib/common/types.hpp"
#include "node_wrapper.hpp"

namespace art {

nodeptr node_fork_inplace(nodeptr t, uint8_t ofs, uint8_t pmlen, uint64_t key, uint64_t val) {
  node4* t_par = (node4*)new_node(0, NODE4, ofs, pmlen, key);

  uint8_t pk0 = partial_key(t->pfx, t->pfx_ofs+pmlen);
  t->pfx_ofs += pmlen+1;
  t->pfx_len -= pmlen+1;
  if (t->type != LEAF) { t->pfx &= prefix_mask(t->pfx_ofs, t->pfx_len); }

  ofs += pmlen;
  uint8_t pk1 = partial_key(key, ofs);

  uint8_t idx = pk0 > pk1;
  t_par->keys[idx] = pk0;
  t_par->chs[idx] = t;
  t_par->keys[1-idx] = pk1;
  t_par->chs[1-idx] = new_leaf(0, ofs+1, key, val);

  return (nodeptr)t_par;
}

nodeptr update_inplace(nodeptr t, uint8_t ofs, uint64_t key, uint64_t val) {
  if (t == nullptr) { return new_leaf(0, ofs, key, val); }

  uint8_t pmlen = prefix_match(t, key);

  if (pmlen < t->pfx_len) { return node_fork_inplace(t, ofs, pmlen, key, val); }

  if (t->type == LEAF) {
    ((leaf*)t)->val = val;
    return t; }

  ofs += t->pfx_len;
  uint8_t pkey = partial_key(key, ofs);
  ofs++;
  if (opt<uint8_t> idx = find_idx(t, pkey); idx) {
    nodeptr* ch = getch(t, idx.value());
    *ch = update_inplace(*ch, ofs, key, val);
    return t; }

  if (is_full(t)) {
    nodeptr t_new = node_extend(t, 0);
    uint8_t i = append(t_new, pkey);
    copych(t_new, t, i);
    *getch(t_new, i) = update_inplace(nullptr, ofs, key, val);
    free_node(t);
    t = t_new; }
  else {
    uint8_t i = append(t, pkey);
    insertch(t, i, update_inplace(nullptr, ofs, key, val)); }
  return t;
}

}
