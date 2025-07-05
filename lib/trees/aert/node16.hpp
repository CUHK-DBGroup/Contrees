#pragma once

#include <cstdint>
#include <cstring>
#include <x86intrin.h>

#include "lib/common/arrayops.hpp"
#include "node.hpp"


namespace aert {

static inline uint8_t node16_find(const node16* t, uint8_t pkey) {
  uint32_t bitfield =
    _mm_movemask_epi8(
      _mm_cmpeq_epi8(
        _mm_load_si128((__m128i*)t->keys),
        _mm_set1_epi8(pkey))) &
    (~(~0U << t->size));
  if (!bitfield) { return 0; }
  return __builtin_ctz(bitfield)+1;
}

static inline uint8_t node16_findgt(const node16* t, uint8_t pkey) {
  uint mask =
    _mm_cmp_epu8_mask(
      _mm_load_si128((__m128i *)((node16*)t)->keys),
      _mm_set1_epi8(pkey),
      _MM_CMPINT_GT) &
    (~(~0U << t->size));
  if (!mask) { return t->size; }
  return __builtin_ctz(mask);
}

static inline nodeptr* node16_getch(node16* t, uint8_t idx) {
  return &t->chs[idx];
}

static inline nodeptr node16_findch(const node16* t, uint8_t pkey) {
  uint8_t idx = node16_find(t, pkey);
  if (!idx) { return nullptr; }
  return t->chs[idx-1];
}

static inline uint8_t node16_append(node16* t, uint8_t pkey) {
  uint32_t idx = node16_findgt(t, pkey);
  insert(t->keys, t->size, idx, pkey);
  t->size++;
  return idx;
}

static inline uint8_t node16_copy_append(nodeptr* tp, node16* t, uint64_t ver, uint8_t pkey) {
  node16* t_new = (node16*)aligned_alloc(64, sizeof(node16));
  memcpy(t_new, t, offsetof(node16, keys));
  t_new->ver = ver;
  t_new->size = t->size+1;
  uint8_t idx = node16_findgt(t, pkey);
  copy_insert(t_new->keys, t->keys, t->size, idx, pkey);
  *tp = (nodeptr)t_new;
  return idx;
}

static inline void node16_insertch(node16* t, uint8_t idx, nodeptr ch) {
  insert(t->chs, t->size-1, idx, ch);
}

static inline void node16_copych(node16* t_new, node16* t_old) {
  copy(t_new->chs, t_old->chs, t_old->size);
}

static inline void node16_copych(node16* t_new, node16* t_old, uint8_t pos) {
  copy_insert<nodeptr>(t_new->chs, t_old->chs, t_old->size, pos, nullptr);
}

}
