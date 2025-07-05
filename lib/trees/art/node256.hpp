#pragma once

#include <cstdint>

#include "lib/common/arrayops.hpp"
#include "lib/common/types.hpp"
#include "node.hpp"

namespace art {

static inline opt<uint8_t> node256_find(const node256* t, uint8_t pkey) {
  return std::make_optional(pkey);
}

static inline nodeptr* node256_getch(node256* t, uint8_t pkey) {
  return &t->chs[pkey];
}

static inline nodeptr node256_findch(const node256* t, uint8_t pkey) {
  return t->chs[pkey];
}

static inline uint8_t node256_append(node256* t, uint8_t pkey) {
  return pkey;
}

static inline void node256_insertch(node256* t, uint8_t pkey, nodeptr ch) {
  t->chs[pkey] = ch;
}

static inline void node256_copych(node256* t_new, node256* t_old) {
  copy(t_new->chs, t_old->chs, 256);
}

void print_node256(const node256* t) {
#ifndef NDEBUG
  fprintf(stderr, "NODE256 %lx %u\n", (uintptr_t)t, t->size);
  uint8_t pkey = 0;
  do {
    if (t->chs[pkey]) { fprintf(stderr, "  %x %lx\n", pkey, (uintptr_t)t->chs[pkey]); }
  } while (++pkey != 0);
#endif
}

}
