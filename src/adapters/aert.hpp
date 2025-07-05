#pragma once

#include "../config.hpp"
#include "lib/common/types.hpp"
#include "lib/trees/aert/context.hpp"
#include "lib/trees/aert/node.hpp"
#include "lib/trees/aert/build.hpp"
#include "lib/trees/aert/handlers.hpp"
#include "lib/trees/aert/query.hpp"
#include "lib/trees/aert/update_inplace.hpp"

namespace aert {

struct interface {
  using operand = aert::operand;
  using context = aert::context;
  using nodeptr = aert::nodeptr;
  using cnodeptr = aert::cnodeptr;

  static inline constexpr operand (*handlers[16])(context*) = {
    handle_init, handle_search, handle_insert, handle_fork,
    handle_split, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr };

  static inline void checkpoint_handler(context*) { }

  static nodeptr build(const config& cfg, uint64_t n, kv* elems) { return aert::build(n, elems); }
  static nodeptr update(nodeptr t, uint64_t key, uint64_t val) { return aert::update_inplace(t, 0, key, val); }
  static opt<uint64_t> find(cnodeptr t, uint64_t key) { return aert::find(t, key); }
  static vec<kv> scan(cnodeptr t, uint64_t key, uint64_t n) { return aert::scan(t, key, n); }
};

}
