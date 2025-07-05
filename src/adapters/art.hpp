#pragma once

#include "../config.hpp"
#include "lib/common/types.hpp"
#include "lib/trees/art/context.hpp"
#include "lib/trees/art/node.hpp"
#include "lib/trees/art/build.hpp"
#include "lib/trees/art/handlers.hpp"
#include "lib/trees/art/query.hpp"
#include "lib/trees/art/update_inplace.hpp"

namespace art {

struct interface {
  using operand = art::operand;
  using context = art::context;
  using nodeptr = art::nodeptr;
  using cnodeptr = art::cnodeptr;

  static inline constexpr operand (*handlers[16])(context*) = {
    handle_init, handle_search, handle_insert, handle_fork,
    nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr };

  static inline void checkpoint_handler(context*) { }

  static nodeptr build(const config& cfg, uint64_t n, kv* elems) { return art::build(n, elems); }
  static nodeptr update(nodeptr t, uint64_t key, uint64_t val) { return art::update_inplace(t, 0, key, val); }
  static opt<uint64_t> find(cnodeptr t, uint64_t key) { return art::find(t, key); }
  static vec<kv> scan(cnodeptr t, uint64_t key, uint64_t n) { return art::scan(t, key, n); }
};

}
