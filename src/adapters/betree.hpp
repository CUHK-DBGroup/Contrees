#pragma once

#include <cstdint>

#include "../config.hpp"
#include "lib/common/types.hpp"
#include "lib/trees/betree/context.hpp"
#include "lib/trees/betree/node.hpp"
#include "lib/trees/betree/build.hpp"
#include "lib/trees/betree/rebuild.hpp"
#include "lib/trees/betree/handlers.hpp"
#include "lib/trees/betree/query.hpp"
#include "lib/trees/betree/update_inplace.hpp"

namespace betree {

struct interface {
  using operand = betree::operand;
  using context = betree::context;
  using nodeptr = betree::nodeptr;
  using cnodeptr = betree::cnodeptr;

  static inline constexpr operand (*handlers[16])(context*) = {
    handle_init, handle_upper, handle_elastic, handle_lower,
    handle_split, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr };

  static inline void checkpoint_handler(context* ctx) {
    ctx->root = rebuild(ctx->t_past, ctx->t_past->verge);
  }

  static nodeptr build(const config& cfg, uint64_t n, kv* elems) { return betree::build(n, cfg.num_pipes-1, elems); }
  static nodeptr update(nodeptr t, uint64_t key, uint64_t val) { return betree::update_root_inplace(t, key, val); }
  static opt<uint64_t> find(cnodeptr t, uint64_t key) { return betree::find(t, key); }
  static vec<kv> scan(cnodeptr t, uint64_t key, uint64_t n) { return betree::scan(t, key, n); }
};

}
