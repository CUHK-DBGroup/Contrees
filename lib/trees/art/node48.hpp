#pragma once

#include <cstdint>

#include "lib/common/arrayops.hpp"
#include "lib/common/types.hpp"
#include "node.hpp"

namespace art {

static inline opt<uint8_t> node48_find(const node48* t, uint8_t pkey) {
  if (t->slts[pkey] == (uint8_t)~0) { return std::nullopt; }
  return std::make_optional(pkey);
}

static inline nodeptr* node48_getch(node48* t, uint8_t pkey) {
  return &t->chs[t->slts[pkey]];
}

static inline nodeptr node48_findch(const node48* t, uint8_t pkey) {
  if (t->slts[pkey] == (uint8_t)~0) { return nullptr; }
  return t->chs[t->slts[pkey]];
}

static inline uint8_t node48_append(node48* t, uint8_t pkey) {
  t->slts[pkey] = t->size++;
  return pkey;
}

static inline void node48_insertch(node48* t, uint8_t pkey, nodeptr ch) {
  t->chs[t->slts[pkey]] = ch;
}

static inline node256* node48_extend(node48* t_old, uint64_t ver) {
  node256* t_new = (node256*)new_node(ver, NODE256, t_old->pfx_ofs, t_old->pfx_len, t_old->pfx);
  return t_new;
}

static inline void node48_copych(node48* t_new, node48* t_old) {
  copy(t_new->chs, t_old->chs, t_old->size);
}

static inline void node48_copych_extend(node256* t_new, node48* t_old) {
  uint8_t pkey = 0;
  do {
    if (t_old->slts[pkey] != (uint8_t)~0) {
      t_new->chs[pkey] = t_old->chs[t_old->slts[pkey]]; }
  } while (++pkey != 0);
}

void print_node48(const node48* t) {
#ifndef NDEBUG
  fprintf(stderr, "NODE48 %lx %u\n", (uintptr_t)t, t->size);
  uint8_t pkey = 0;
  do {
    if (t->slts[pkey] != (uint8_t)~0) { fprintf(stderr, "  %x %lx\n", pkey, (uintptr_t)t->chs[t->slts[pkey]]); }
  } while (++pkey != 0);
#endif
}

}
