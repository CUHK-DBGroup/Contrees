#pragma once

#include <cstdint>
#include <optional>
#include <tuple>
#include <vector>

#include "node.hpp"

namespace betree {

static inline std::optional<uint64_t> find(cnodeptr t, uint64_t key) {
  if (t == nullptr) { return std::nullopt; }
  while (t->type != LEAF) { t = t->chs[findch(t, key)]; }
  uint8_t i = findval(t, key);
  if (i < t->size && t->keys[i] == key) { return std::make_optional(t->vals[i]); }
  return std::nullopt;
}

using kvpair = std::pair<uint64_t, uint64_t>;

void scan_append(cnodeptr t, uint8_t i, uint64_t n, std::vector<kvpair>& ret) {
  if (t->type != LEAF) {
    for (; ret.size() < n && i <= t->size; i++) {
      scan_append(t->chs[i], 0, n, ret); }
    return; }
  for (; ret.size() < n && i < t->size; i++) {
    ret.emplace_back(t->keys[i], t->vals[i]); }
}

void scan_search(cnodeptr t, uint64_t key, uint64_t n, std::vector<kvpair>& ret) {
  if (t->type == LEAF) { scan_append(t, findval(t, key), n, ret); }
  else {
    uint8_t i = findch(t, key);
    scan_search(t->chs[i], key, n, ret);
    scan_append(t, i+1, n, ret);
    return; }
}

static inline std::vector<kvpair> scan(cnodeptr t, uint64_t key, uint64_t n) {
  std::vector<kvpair> ret;
  scan_search(t, key, n, ret);
  return ret;
}

}
