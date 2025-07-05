#pragma once

#include <cstdint>

#include "lib/common/arrayops.hpp"
#include "node.hpp"

namespace aert {

static inline uint8_t nodeh16_find(const nodeh16* t, uint8_t pkey) {
  return pkey+1;
}

static inline nodeptr* nodeh16_getch(nodeh16* t, uint8_t pkey) {
  return &t->chs[pkey];
}

static inline nodeptr nodeh16_findch(const nodeh16* t, uint8_t pkey) {
  return t->chs[pkey];
}

static inline uint8_t nodeh16_append(nodeh16* t, uint8_t pkey) {
  return pkey;
}

static inline uint8_t nodeh16_copy_append(nodeptr* tp, nodeh16* t, uint64_t ver, uint8_t pkey) {
  *tp = node_copy((nodeptr)t, ver);
  return pkey;
}

static inline void nodeh16_insertch(nodeh16* t, uint8_t pkey, nodeptr ch) {
  t->chs[pkey] = ch;
}

static inline void nodeh16_copych(nodeh16* t_new, nodeh16* t_old) {
  copy(t_new->chs, t_old->chs, 16);
}

}
