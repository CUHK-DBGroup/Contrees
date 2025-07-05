#pragma once

#include <cstdint>
#include <cstring>
#include <tuple>

#include <mimalloc.h>
#include <mimalloc-override.h>
#include <mimalloc-new-delete.h>

#include "lib/common/arrayops.hpp"

namespace betree {

enum NODE_TYPE { UPPER, LOWER, BOUNDARY, LEAF };

static inline constexpr uint8_t B = 8;
static inline constexpr uint8_t E = 3;

struct alignas(128) node {
  uint32_t ver;
  uint8_t type;
  uint8_t verge;
  uint8_t size;
  uint8_t height;
  uint64_t keys[2*B-1];
  union {
    node* chs[2*B];
    uint64_t vals[2*B-1]; };
};

using nodeptr = node*;
using cnodeptr = const node*;

static inline nodeptr new_node(uint32_t ver, uint8_t type, uint8_t size) {
  nodeptr t = (nodeptr)aligned_alloc(128, sizeof(node));
  t->ver = ver;
  t->type = type;
  t->verge = 0;
  t->size = size;
  t->height = 0;
  return t;
}

static inline nodeptr new_leaf(uint32_t ver, uint8_t size) {
  nodeptr t = new_node(ver, LEAF, size);
  memset(t->keys+size, 0, (2*B-size-1)*sizeof(uint64_t));
  memset(t->vals+size, 0, (2*B-size)*sizeof(uint64_t));
  return t;
}

static inline nodeptr new_internal(uint32_t ver, uint8_t size) {
  nodeptr t = new_node(ver, LOWER, size);
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

static inline nodeptr copy_leaf(nodeptr t, uint32_t ver) {
  nodeptr t_new = (nodeptr)aligned_alloc(128, sizeof(node));
  memcpy(t_new, t, 128);
  t_new->ver = ver;
  return t_new;
}

static inline nodeptr copy_internal(nodeptr t, uint32_t ver) {
  nodeptr t_new = (nodeptr)aligned_alloc(128, sizeof(node));
  memcpy(t_new, t, offsetof(node, chs));
  t_new->ver = ver;
  return t_new;
}

static inline std::tuple<uint64_t, nodeptr, nodeptr> split(nodeptr t, uint32_t ver) {
  nodeptr t_l = new_internal(ver, B-1);
  nodeptr t_r = new_internal(ver, B-1);
  t_l->height = t->height;
  t_r->height = t->height;
  copy(t_l->keys, t->keys, B-1);
  copy(t_r->keys, t->keys+B, B-1);
  // chs are copied at next stage
  return std::make_tuple(t->keys[B-1], t_l, t_r);
}

static inline void copych(nodeptr t_new, nodeptr t_old) {
  copy(t_new->chs, t_old->chs, 2*B);
}

static inline void copych_split(nodeptr* ts, nodeptr t_old) {
  copy_split(ts[0]->chs, ts[1]->chs, t_old->chs, 2*B, B);
}

static inline uint8_t findch(cnodeptr t, uint64_t key) {
  uint8_t i = 0;
  for ( ; i < t->size; i++) { if (t->keys[i] > key) { break; } }
  return i;
}

static inline uint8_t findval(cnodeptr t, uint64_t key) {
  uint8_t i = 0;
  for ( ; i < t->size; i++) { if (t->keys[i] >= key) { break; } }
  return i;
}

static inline void insert_child(nodeptr t, uint64_t key, nodeptr t_l, nodeptr t_r, uint8_t pos) {
  insert(t->keys, t->size, pos, key);
  t->chs[pos] = t_l;
  insert(t->chs, t->size+1, pos+1, t_r);
  t->size++;
}

}
