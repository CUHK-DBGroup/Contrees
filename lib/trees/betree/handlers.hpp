#pragma once

#include <cstdint>

#include "lib/common/arrayops.hpp"
#include "node.hpp"
#include "context.hpp"
#include "update_cow.hpp"

namespace betree {

operand handle_init(context* ctx) {
  operand op = update_upper_cow(ctx, &ctx->root);
  ctx->t_cur = ctx->root;
  return op;
}

operand handle_upper(context* ctx) {
  copych(ctx->t_cur, ctx->t_past);
  uint8_t i = findch(ctx->t_past, ctx->key);
  ctx->t_past = ctx->t_past->chs[i];
  operand op = update_upper_cow(ctx, &ctx->t_cur->chs[i]);
  ctx->t_cur = ctx->t_cur->chs[i];
  return op;
}

operand handle_elastic(context* ctx) {
  copych(ctx->t_cur, ctx->t_past);
  uint8_t i = findch(ctx->t_past, ctx->key);
  nodeptr t_past = ctx->t_past = ctx->t_past->chs[i];
  nodeptr *tp = &ctx->t_cur->chs[i];
  uint32_t ver = ctx->sno;

  if (t_past->type == LEAF) {
    auto [key, t_l, t_r] = update_leaf_cow(t_past, ctx);
    if (t_r == nullptr) { *tp = t_l; }
    else {
      *tp = new_root(ver, key, t_l, t_r);
      (*tp)->height = 1;
      if ((*tp)->height > E) { ctx->new_checkpoint = 1; } }
    return operand::DONE; }

  if (t_past->size < 2*B-1) {
    *tp = copy_internal(t_past, ver);
    copych(*tp, t_past);
    ctx->t_cur = *tp; }
  else {
    auto [key, t_l, t_r] = split(t_past, ver);
    *tp = new_root(ver, key, t_l, t_r);
    (*tp)->height = t_past->height+1;
    if ((*tp)->height > E) { ctx->new_checkpoint = 1; }
    copych_split((*tp)->chs, t_past);
    ctx->t_cur = ctx->key < key ? (*tp)->chs[0] : (*tp)->chs[1]; }

  return update_lower_cow(ctx);
}

operand handle_lower(context* ctx) {
  copych(ctx->t_cur, ctx->t_past);
  return update_lower_cow(ctx);
}

operand handle_split(context* ctx) {
  nodeptr t_cur = ctx->t_cur;
  copych_split(t_cur->chs+ctx->split_idx, ctx->t_past);
  ctx->t_cur = t_cur->chs[ctx->split_idx + (t_cur->keys[ctx->split_idx] <= ctx->key)];
  return update_lower_cow(ctx);
}

}
