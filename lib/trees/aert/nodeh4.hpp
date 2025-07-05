#pragma once

#include <cstdint>
#include <cstring>

#include "lib/common/arrayops.hpp"
#include "node.hpp"

namespace aert {

static inline uint8_t nodeh4_find(const nodeh4* t, uint8_t pkey) {
  if (t->keys[0] == pkey) { return 1; }
  if (t->keys[1] == pkey) { return 2; }
  if (t->size == 2) { return 0; }
  if (t->keys[2] == pkey) { return 3; }
  if (t->size == 3) { return 0; }
  if (t->keys[3] == pkey) { return 4; }
  return 0;
}

static inline uint8_t nodeh4_findgt(const nodeh4* t, uint8_t pkey) {
  uint8_t idx = 0;
  while (idx < t->size && t->keys[idx] <= pkey) { idx++; }
  return idx;
}

static inline nodeptr* nodeh4_getch(nodeh4* t, uint8_t idx) {
  return &t->chs[idx];
}

static inline nodeptr nodeh4_findch(const nodeh4* t, uint8_t pkey) {
  uint8_t idx = nodeh4_find(t, pkey);
  if (!idx) { return nullptr; }
  return t->chs[idx-1];
}

static inline uint8_t nodeh4_append(nodeh4* t, uint8_t pkey) {
  uint8_t idx = nodeh4_findgt(t, pkey);
  insert(t->keys, t->size, idx, pkey);
  t->size++;
  return idx;
}

static inline uint8_t nodeh4_copy_append(nodeptr* tp, nodeh4* t, uint64_t ver, uint8_t pkey) {
  nodeh4* t_new = (nodeh4*)aligned_alloc(64, sizeof(nodeh4));
  memcpy(t_new, t, offsetof(nodeh4, keys));
  t_new->ver = ver;
  t_new->size = t->size+1;
  uint8_t idx = nodeh4_findgt(t, pkey);
  copy_insert(t_new->keys, t->keys, t->size, idx, pkey);
  *tp = (nodeptr)t_new;
  return idx;
}

static inline void nodeh4_insertch(nodeh4* t, uint8_t idx, nodeptr ch) {
  insert(t->chs, t->size-1, idx, ch);
}

static inline nodeh16* nodeh4_extend(nodeh4* t_old, uint64_t ver) {
  nodeh16* t_new = (nodeh16*)new_node(ver, NODEH16, t_old->pfx_ofs, t_old->pfx_len, t_old->pfx);
  return t_new;
}

static inline uint8_t nodeh4_extend_append(nodeptr* tp, nodeh4* t, uint64_t ver, uint8_t pkey) {
  *tp = (nodeptr)nodeh4_extend(t, ver);
  return pkey;
}

static inline void nodeh4_copych(nodeh4* t_new, nodeh4* t_old) {
  copy(t_new->chs, t_old->chs, t_old->size);
}

static inline void nodeh4_copych(nodeh4* t_new, nodeh4* t_old, uint8_t pos) {
  copy_insert<nodeptr>(t_new->chs, t_old->chs, t_old->size, pos, nullptr);
}

static inline void nodeh4_copych_extend(nodeh16* t_new, nodeh4* t_old) {
  for (uint8_t idx = 0; idx < t_old->size; idx++) {
    t_new->chs[t_old->keys[idx]] = t_old->chs[idx]; }
}

}
