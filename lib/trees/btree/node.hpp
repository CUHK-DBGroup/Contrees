#pragma once

#include <cstdint>
#include <cstring>
#include <tuple>

#include <mimalloc.h>
#include <mimalloc-override.h>
#include <mimalloc-new-delete.h>

#include "lib/common/arrayops.hpp"

namespace btree {

enum NODE_TYPE { INTERNAL, LEAF };

static inline constexpr uint8_t B = 8;

struct alignas(128) node {
  uint32_t ver;
  uint8_t type;
  uint8_t size;
  uint64_t keys[2*B-1];
  union {
    node*    chs[2*B];
    uint64_t vals[2*B-1];
  };
};

using nodeptr = node*;
using cnodeptr = const node*;

static inline nodeptr new_node(uint32_t ver, uint8_t type, uint8_t size) {
  nodeptr t = (nodeptr)aligned_alloc(128, sizeof(node));
  t->ver = ver;
  t->type = type;
  t->size = size;
  return t;
}

static inline nodeptr new_leaf(uint32_t ver, uint8_t size) {
  nodeptr t = new_node(ver, LEAF, size);
  memset(t->keys+size, 0, (2*B-size-1)*sizeof(uint64_t));
  memset(t->vals+size, 0, (2*B-size)*sizeof(uint64_t));
  return t;
}

static inline nodeptr new_internal(uint32_t ver, uint8_t size) {
  nodeptr t = new_node(ver, INTERNAL, size);
  memset(t->keys+size, 0, (2*B-size-1)*sizeof(uint64_t));
  memset(t->chs+size+1, 0, (2*B-size-1)*sizeof(nodeptr));
  return t;
}

static inline nodeptr new_root(uint32_t ver, uint64_t key, nodeptr ch0, nodeptr ch1) {
  nodeptr t = new_internal(ver, 1);
  t->keys[0] = key;
  t->chs[0] = ch0;
  t->chs[1] = ch1;
  return t;
}

static inline nodeptr copy_leaf(uint32_t ver, nodeptr t) {
  nodeptr t_new = (nodeptr)aligned_alloc(128, sizeof(node));
  memcpy(t_new, t, 128);
  t_new->ver = ver;
  return t_new;
}

static inline nodeptr copy_internal(uint32_t ver, nodeptr t) {
  nodeptr t_new = (nodeptr)aligned_alloc(128, sizeof(node));
  t_new->ver = ver;
  t_new->type = INTERNAL;
  t_new->size = t->size;
  memcpy(t_new->keys, t->keys, (2*B-1)*sizeof(uint64_t));
  return t_new;
}

static inline std::tuple<uint64_t, nodeptr, nodeptr> split(uint32_t ver, nodeptr t) {
  nodeptr t_l = new_internal(ver, B-1);
  nodeptr t_r = new_internal(ver, B-1);
  memcpy(t_l->keys, t->keys, (B-1)*sizeof(uint64_t));
  memcpy(t_r->keys, t->keys+B, (B-1)*sizeof(uint64_t));
  // chs are copied at next stage
  return std::make_tuple(t->keys[B-1], t_l, t_r);
}

static inline void copych(nodeptr t_new, nodeptr t_old) {
  memcpy(t_new->chs, t_old->chs, (2*B)*sizeof(uint64_t));
}

static inline void copych_split(nodeptr* ts, nodeptr t_old) {
  memcpy(ts[0]->chs, t_old->chs, B*sizeof(nodeptr));
  memcpy(ts[1]->chs, t_old->chs+B, B*sizeof(nodeptr));
}

static inline uint8_t findch(cnodeptr t, uint64_t key) {
  for (uint8_t pos = 0; pos < t->size; pos++) {
    if (t->keys[pos] > key) { return pos; } }
  return t->size;
}

static inline uint8_t findval(cnodeptr t, uint64_t key) {
  uint8_t pos = 0;
  while (pos < t->size && t->keys[pos] < key) { pos++; }
  return pos;
}

static inline void insert_child(nodeptr t, uint64_t key, nodeptr t_l, nodeptr t_r, uint8_t pos) {
  insert(t->keys, t->size, pos, key);
  t->chs[pos] = t_l;
  insert(t->chs, t->size+1, pos+1, t_r);
  t->size++;
}

}
