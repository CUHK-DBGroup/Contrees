#pragma once

#include "lib/common/arrayops.hpp"
#include "node.hpp"
#include "context.hpp"
#include "update_cow.hpp"

namespace btree {

operand handle_init(context* ctx) {
  nodeptr t_past = ctx->t_past;
  nodeptr* tp = &ctx->root;

  if (t_past->type == LEAF) {
    auto [key, t_l, t_r] = update_leaf_cow(t_past, ctx);
    if (t_r == nullptr) { *tp = t_l; }
    else { *tp = new_root(ctx->sno, key, t_l, t_r);
    return operand::DONE; } }

  if (t_past->size < 2*B-1) {
    *tp = copy_internal(ctx->sno, t_past);
    copych(*tp, t_past);
    ctx->t_cur = *tp; }
  else {
    auto [key, t_l, t_r] = split(ctx->sno, t_past);
    *tp = new_root(ctx->sno, key, t_l, t_r);
    copych_split((*tp)->chs, t_past);
    ctx->t_cur = ctx->key < key ? (*tp)->chs[0] : (*tp)->chs[1]; }

  return update_cow(ctx);
}

operand handle_search_down(context* ctx) {
  copych(ctx->t_cur, ctx->t_past);
  return update_cow(ctx);
}

operand handle_search_split(context* ctx) {
  nodeptr t_cur = ctx->t_cur;
  copych_split(t_cur->chs+ctx->split_idx, ctx->t_past);
  ctx->t_cur = t_cur->chs[ctx->split_idx + (t_cur->keys[ctx->split_idx] <= ctx->key)];
  return update_cow(ctx);
}

}
