#pragma once

#include <cstdint>
#include <cstring>

#include "node_wrapper.hpp"

namespace aert {

nodeptr node_fork_inplace(uint8_t pmlen, nodeptr t, uint8_t ofs, uint64_t key, uint64_t val) {
  node4* t_par = (node4*)new_node(0, NODE4, ofs, pmlen, key);

  uint8_t pk0 = partial_key(t->pfx, t->pfx_ofs+pmlen, 8);
  t->pfx_ofs += pmlen+8;
  t->pfx_len -= pmlen+8;
  if (t->type != LEAF) { t->pfx &= prefix_mask(t->pfx_ofs, t->pfx_len); }

  ofs += pmlen;
  uint8_t pk1 = partial_key(key, ofs, 8);

  uint8_t idx = pk0 > pk1;
  t_par->keys[idx] = pk0;
  t_par->chs[idx] = t;
  t_par->keys[1-idx] = pk1;
  t_par->chs[1-idx] = new_leaf(0, ofs+8, key, val);

  return (nodeptr)t_par;
}

nodeptr nodeh_fork_inplace(nodeptr t, uint8_t ppfx, uint8_t ofs, uint64_t key, uint64_t val) {
  nodeh4* t_par = (nodeh4*)new_node(0, NODEH4, ofs, 0, key);
  uint8_t pk0 = ppfx & 0x0f;
  uint8_t pk1 = partial_key(key, ofs, 4);
  uint8_t idx = pk0 > pk1;

  t_par->keys[idx] = pk0;
  t_par->chs[idx] = t;
  t_par->keys[1-idx] = pk1;
  t_par->chs[1-idx] = (nodeptr)new_leaf(0, ofs+4, key, val);

  return (nodeptr)t_par;
}

static inline nodeptr create_upper_half(node16* t_old, uint64_t ver, uint8_t pkey) {
  uint8_t ch_sz[16];
  memset(ch_sz, 0, 16);
  for (uint8_t idx = 0; idx < 16; idx++) { ch_sz[t_old->keys[idx] >> 4]++; }
  ch_sz[pkey >> 4]++;
  uint8_t cnt = 0;
  for (uint8_t idx = 0; idx < 16; idx++) { cnt += !!ch_sz[idx]; }

  nodeptr t_uh = new_nodeh(ver, cnt, t_old->pfx_ofs, t_old->pfx_len, t_old->pfx);
  for (uint8_t idx = 0; idx < 16; idx++) {
    if (ch_sz[idx] > 1) {
      nodeptr* t_lh = getch(t_uh, append(t_uh, idx));
      *t_lh = new_nodeh(ver, ch_sz[idx], t_old->pfx_ofs+t_old->pfx_len+4, 0, 0); } }
  return t_uh;
}

static inline void create_lower_half(nodeptr t_uh, nodeptr t_old_ch, uint64_t ver, uint8_t pkey) {
  uint8_t hi = pkey >> 4;
  uint8_t lo = pkey & 0x0f;
  nodeptr t_lh = findch(t_uh, hi);
  if (t_lh == nullptr) {
    insertch(t_uh, append(t_uh, hi), embed_ptr(t_old_ch, lo | 0x10));}
  else {
    insertch(t_lh, append(t_lh, lo), t_old_ch); }
}

nodeptr node_split_inplace(node16* t, uint8_t pkey, uint8_t ofs, uint64_t key, uint64_t val) {
  nodeptr t_uh = create_upper_half(t, 0, pkey);
  for (uint8_t idx = 0; idx < t->size; ++idx) {
    create_lower_half(t_uh, t->chs[idx], 0, t->keys[idx]); }
  nodeptr t_leaf = new_leaf(0, ofs, key, val);
  uint8_t pkey_leaf = partial_key(key, ofs-8, 8);
  create_lower_half(t_uh, t_leaf, 0, pkey_leaf);
  free_node(t);
  return t_uh;
}

nodeptr update_inplace(nodeptr t, uint8_t ofs, uint64_t key, uint64_t val) {
  if (t == nullptr) { return new_leaf(0, ofs, key, val); }

  uint8_t ppfx;
  std::tie(t, ppfx) = unembed_ptr(t);
  bool is_embed = ppfx & 0x10;
  if (is_embed && ptr_pfx_match(ppfx, t->pfx_ofs, key) < 4) {
    return nodeh_fork_inplace(t, ppfx, ofs, key, val); }

  ofs += is_embed * 4;
  uint8_t pmlen = prefix_match(t, key) & 0xf8;
  if (pmlen < t->pfx_len) {
    nodeptr t_new = node_fork_inplace(pmlen, t, ofs, key, val);
    return embed_ptr(t_new, ppfx); }

  uint8_t key_len = node_key_len(t->type);
  if (t->type == LEAF) {
    ((leaf*)t)->val = val;
    return embed_ptr(t, ppfx); }

  ofs += t->pfx_len;
  uint8_t pkey = partial_key(key, ofs, key_len);
  ofs += key_len;
  uint8_t idx = findidx(t, pkey);
  if (idx) {
    nodeptr* ch = getch(t, idx-1);
    *ch = update_inplace(*ch, ofs, key, val);
    return embed_ptr(t, ppfx); }

  if (t->size == 16) {
    nodeptr t_new = node_split_inplace((node16*)t, pkey, ofs, key, val);
    return embed_ptr(t_new, ppfx); }

  if (is_full(t)) {
    nodeptr t_old = t;
    uint8_t i = extend_append(&t, t_old, 0, pkey);
    copych(t, t_old, i);
    *getch(t, i) = new_leaf(0, ofs, key, val);
    free_node(t_old);
    return embed_ptr(t, ppfx); }

  uint8_t i = append(t, pkey);
  insertch(t, i, new_leaf(0, ofs, key, val));
  return embed_ptr(t, ppfx);
}

}
