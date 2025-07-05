#pragma once

#include <cstdint>

#include "lib/common/types.hpp"
#include "node.hpp"
#include "build.hpp"

namespace betree {

void unfold_elems(nodeptr t, vec<kv>& elems) {
  if (t->type != LEAF) {
    for (uint8_t i = 0; i <= t->size; i++) { unfold_elems(t->chs[i], elems); }
    return; }
  for (uint8_t i = 0; i < t->size; i++) {
    elems.push_back(std::make_pair(t->keys[i], t->vals[i])); }
}

static inline nodeptr rebuild_entire(nodeptr t, uint8_t h) {
  vec<kv> elems;
  unfold_elems(t, elems);
  return build_sorted(elems.size(), h, elems.data());
}

void unfold_extra(nodeptr t, vec<uint64_t>& keys, vec<nodeptr>& chs) {
  if (t->height == 0) {
    chs.push_back(t);
    return; }
  for (uint8_t i = 0; i < t->size; i++) {
    unfold_extra(t->chs[i], keys, chs);
    keys.push_back(t->keys[i]); }
  unfold_extra(t->chs[t->size], keys, chs);
}

void unfold_upper_part(nodeptr t, vec<uint64_t>& keys, vec<nodeptr>& chs) {
  if (t->type != BOUNDARY) {
    for (uint8_t i = 0; i < t->size; i++) {
      unfold_upper_part(t->chs[i], keys, chs);
      keys.push_back(t->keys[i]); }
    unfold_upper_part(t->chs[t->size], keys, chs); }
  else {
    for (uint8_t i = 0; i < t->size; i++) {
      unfold_extra(t->chs[i], keys, chs);
      keys.push_back(t->keys[i]); }
    unfold_extra(t->chs[t->size], keys, chs); }
}

static inline nodeptr rebuild_internal(nodeptr t, uint8_t h) {
  vec<uint64_t> keys;
  vec<nodeptr> chs;
  keys.push_back(0);
  unfold_upper_part(t, keys, chs);
  return std::get<0>(build_internal(h, keys, chs));
}

static inline bool leaf_over_boundary(nodeptr t, uint8_t h) {
  for (uint8_t i = 0; i < h; i++) {
    if (t->type == LEAF) { return true; }
    t = t->chs[0]; }
  return t->type == LEAF;
}

nodeptr rebuild(nodeptr t, uint8_t h) {
  return leaf_over_boundary(t, h) ? rebuild_entire(t, h) : rebuild_internal(t, h);
}

}
