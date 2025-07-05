#pragma once

#include <cstdint>
#include <cstring>

#include "lib/common/arrayops.hpp"
#include "node.hpp"

namespace aert {

static inline uint8_t node4_find(const node4* t, uint8_t pkey) {
  if (t->keys[0] == pkey) { return 1; }
  if (t->keys[1] == pkey) { return 2; }
  if (t->size == 2) { return 0; }
  if (t->keys[2] == pkey) { return 3; }
  if (t->size == 3) { return 0; }
  if (t->keys[3] == pkey) { return 4; }
  return 0;
}

static inline uint8_t node4_findgt(const node4* t, uint8_t pkey) {
  uint8_t idx = 0;
  while (idx < t->size && t->keys[idx] <= pkey) { idx++; }
  return idx;
}

static inline nodeptr* node4_getch(node4* t, uint8_t idx) {
  return &t->chs[idx];
}

static inline nodeptr node4_findch(const node4* t, uint8_t pkey) {
  uint8_t idx = node4_find(t, pkey);
  if (!idx) { return nullptr; }
  return t->chs[idx-1];
}

static inline uint8_t node4_append(node4* t, uint8_t pkey) {
  uint8_t idx = node4_findgt(t, pkey);
  insert(t->keys, t->size, idx, pkey);
  t->size++;
  return idx;
}

static inline uint8_t node4_copy_append(nodeptr* tp, node4* t, uint64_t ver, uint8_t pkey) {
  node4* t_new = (node4*)aligned_alloc(64, sizeof(node4));
  memcpy(t_new, t, offsetof(node4, keys));
  t_new->ver = ver;
  t_new->size = t->size+1;
  uint8_t idx = node4_findgt(t, pkey);
  copy_insert(t_new->keys, t->keys, t->size, idx, pkey);
  *tp = (nodeptr)t_new;
  return idx;
}

static inline void node4_insertch(node4* t, uint8_t idx, nodeptr ch) {
  insert(t->chs, t->size-1, idx, ch);
}

static inline node16* node4_extend(node4* t_old, uint64_t ver) {
  node16* t_new = (node16*)new_node(ver, NODE16, t_old->pfx_ofs, t_old->pfx_len, t_old->pfx);
  *(uint32_t*)t_new->keys = *(uint32_t*)t_old->keys;
  return t_new;
}

static inline uint8_t node4_extend_append(nodeptr* tp, node4* t, uint64_t ver, uint8_t pkey) {
  node16* t_new = (node16*)new_node(ver, NODE16, t->pfx_ofs, t->pfx_len, t->pfx);
  t_new->size++;
  uint8_t idx = node4_findgt(t, pkey);
  copy_insert(t_new->keys, t->keys, t->size, idx, pkey);
  *tp = (nodeptr)t_new;
  return idx;
}

static inline void node4_copych(node4* t_new, node4* t_old) {
  copy(t_new->chs, t_old->chs, t_old->size);
}

static inline void node4_copych(node4* t_new, node4* t_old, uint8_t pos) {
  copy_insert<nodeptr>(t_new->chs, t_old->chs, t_old->size, pos, nullptr);
}

static inline void node4_copych_extend(node16* t_new, node4* t_old) {
  copy(t_new->chs, t_old->chs, t_old->size);
}

static inline void node4_copych_extend(node16* t_new, node4* t_old, uint8_t pos) {
  copy_insert<nodeptr>(t_new->chs, t_old->chs, t_old->size, pos, nullptr);
}

}
