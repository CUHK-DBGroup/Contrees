#pragma once

#include <cstdint>

#include "lib/common/arrayops.hpp"
#include "lib/common/types.hpp"
#include "node.hpp"

namespace art {

static inline opt<uint8_t> node4_find(const node4* t, uint8_t pkey) {
  if (t->keys[0] == pkey) { return std::make_optional(0); }
  if (t->keys[1] == pkey) { return std::make_optional(1); }
  if (t->size == 2) { return std::nullopt; }
  if (t->keys[2] == pkey) { return std::make_optional(2); }
  if (t->size == 3) { return std::nullopt; }
  if (t->keys[3] == pkey) { return std::make_optional(3); }
  return std::nullopt;
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
  opt<uint8_t> idx = node4_find(t, pkey);
  if (!idx) { return nullptr; }
  return t->chs[idx.value()];
}

static inline uint8_t node4_append(node4* t, uint8_t pkey) {
  uint8_t idx = node4_findgt(t, pkey);
  insert(t->keys, t->size, idx, pkey);
  t->size++;
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

void print_node4(const node4* t) {
#ifndef NDEBUG
  fprintf(stderr, "NODE4 %lx %u\n", (uintptr_t)t, t->size);
  for (uint i = 0; i < t->size; i++) {
    fprintf(stderr, "  %x %lx\n", t->keys[i], (uintptr_t)t->chs[i]); }
#endif
}

}
