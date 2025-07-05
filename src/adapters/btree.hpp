#pragma once

#include <cstdint>

#include "../config.hpp"
#include "lib/common/types.hpp"
#include "lib/trees/btree/context.hpp"
#include "lib/trees/btree/node.hpp"
#include "lib/trees/btree/build.hpp"
#include "lib/trees/btree/handlers.hpp"
#include "lib/trees/btree/query.hpp"
#include "lib/trees/btree/update_inplace.hpp"

namespace btree {

struct interface {
  using operand = btree::operand;
  using context = btree::context;
  using nodeptr = btree::nodeptr;
  using cnodeptr = btree::cnodeptr;

  static inline constexpr operand (*handlers[16])(context*) = {
    handle_init, handle_search_down, handle_search_split, nullptr,
    nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr };

  static inline void checkpoint_handler(context* ctx) { }

  static nodeptr build(const config& cfg, uint64_t n, kv* elems) { return btree::build(n, elems); }
  static nodeptr update(nodeptr t, uint64_t key, uint64_t val) { return btree::update_root_inplace(t, key, val); }
  static opt<uint64_t> find(cnodeptr t, uint64_t key) { return btree::find(t, key); }
  static vec<kv> scan(cnodeptr t, uint64_t key, uint64_t n) { return btree::scan(t, key, n); }
};

}
