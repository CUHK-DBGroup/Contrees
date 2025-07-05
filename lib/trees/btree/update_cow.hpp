#pragma once

#include <cstdint>
#include <tuple>

#include "lib/common/arrayops.hpp"
#include "context.hpp"
#include "node.hpp"

namespace btree {

static inline std::tuple<uint64_t, nodeptr, nodeptr> update_leaf_cow(nodeptr t, context* ctx) {
  uint32_t ver = ctx->sno;
  uint64_t key = ctx->key;
  uint64_t val = ctx->val;

  uint8_t i = findval(t, key);
  if (i < t->size && t->keys[i] == key) {
    nodeptr t_new = copy_leaf(ver, t);
    t_new->vals[i] = val;
    return std::make_tuple(key, t_new, nullptr); }

  if (t->size < 2*B-1) {
    nodeptr t_new = new_leaf(ver, t->size+1);
    copy_insert(t_new->keys, t->keys, t->size, i, key);
    copy_insert(t_new->vals, t->vals, t->size, i, val);
    return std::make_tuple(key, t_new, nullptr); }

  nodeptr t_l = new_leaf(ver, B);
  nodeptr t_r = new_leaf(ver, B);
  if (i < B) {
    copy_insert(t_l->keys, t->keys, B-1, i, key);
    copy_insert(t_l->vals, t->vals, B-1, i, val);
    copy(t_r->keys, t->keys+B-1, B);
    copy(t_r->vals, t->vals+B-1, B); }
  else {
    copy(t_l->keys, t->keys, B);
    copy(t_l->vals, t->vals, B);
    copy_insert(t_r->keys, t->keys+B, B-1, i-B, key);
    copy_insert(t_r->vals, t->vals+B, B-1, i-B, val); }
  return std::make_tuple(t_r->keys[0], t_l, t_r);
}

operand update_cow(context* ctx) {
  nodeptr t_cur = ctx->t_cur;
  uint32_t ver = ctx->sno;

  uint8_t i = findch(t_cur, ctx->key);
  nodeptr t_past = ctx->t_past = t_cur->chs[i];

  if (t_past->type == LEAF) {
    auto [key, t_l, t_r] = update_leaf_cow(t_past, ctx);
    if (t_r == nullptr) { t_cur->chs[i] = t_l; }
    else { insert_child(t_cur, key, t_l, t_r, i); }
    return operand::DONE; }

  if (t_past->size == 2*B-1) {
    auto [key, t_l, t_r] = split(ver, t_past);
    insert_child(t_cur, key, t_l, t_r, i);
    ctx->split_idx = i;
    return operand::SEARCH_SPLIT; }

  t_cur->chs[i] = copy_internal(ver, t_past);
  ctx->t_cur = t_cur->chs[i];
  return operand::SEARCH_DOWN;
}

}
