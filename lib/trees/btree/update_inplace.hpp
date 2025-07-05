#pragma once

#include <cstdint>
#include <tuple>

#include "lib/common/arrayops.hpp"
#include "node.hpp"

namespace btree {

static inline std::tuple<uint64_t, nodeptr, nodeptr> update_leaf_inplace(nodeptr t, uint64_t k, uint64_t v) {
  uint8_t i = findval(t, k);
  if (i < t->size && t->keys[i] == k) {
    t->vals[i] = v;
    return std::make_tuple(k, t, nullptr); }

  if (t->size < 2*B-1) {
    insert(t->keys, t->size, i, k);
    insert(t->vals, t->size, i, v);
    t->size++;
    return std::make_tuple(k, t, nullptr); }

  nodeptr t_l = new_leaf(0, B);
  nodeptr t_r = new_leaf(0, B);
  if (i < B) {
    copy_insert(t_l->keys, t->keys, B-1, i, k);
    copy_insert(t_l->vals, t->vals, B-1, i, v);
    copy(t_r->keys, t->keys+B-1, B);
    copy(t_r->vals, t->vals+B-1, B); }
  else {
    copy(t_l->keys, t->keys, B);
    copy(t_l->vals, t->vals, B);
    copy_insert(t_r->keys, t->keys+B, B-1, i-B, k);
    copy_insert(t_r->vals, t->vals+B, B-1, i-B, v); }
  return std::make_tuple(t_r->keys[0], t_l, t_r);
}

static inline void update_internal_inplace(nodeptr t, uint64_t k, uint64_t v) {
  uint8_t i = findch(t, k);
  nodeptr ch = t->chs[i];

  if (ch->type == LEAF) {
    auto [p, t_l, t_r] = update_leaf_inplace(ch, k, v);
    if (t_r != nullptr) {
      free(ch);
      insert_child(t, p, t_l, t_r, i); } }
  else if (ch->size < 2*B-1) { update_internal_inplace(ch, k, v); }
  else {
    auto [p, t_l, t_r] = split(0, ch);
    insert_child(t, p, t_l, t_r, i);
    copych_split(&t->chs[i], ch);
    free(ch);
    i += p <= k;
    update_internal_inplace(t->chs[i], k, v); }
}

static inline nodeptr update_root_inplace(nodeptr t, uint64_t k, uint64_t v) {
  if (t->type == LEAF) {
    auto [p, t_l, t_r] = update_leaf_inplace(t, k, v);
    if (t_r != nullptr) {
      free(t);
      t = new_root(0, p, t_l, t_r); } }
  else if (t->size < 2*B-1) { update_internal_inplace(t, k, v); }
  else {
    auto [p, t_l, t_r] = split(0, t);
    nodeptr t_new = new_root(0, p, t_l, t_r);
    copych_split(&t_new->chs[0], t);
    free(t);
    t = t_new;
    update_internal_inplace(t->chs[p <= k], k, v); }
  return t;
}

}
