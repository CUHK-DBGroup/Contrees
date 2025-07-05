#pragma once

#include <algorithm>
#include <cstdint>

#include "lib/common/types.hpp"
#include "node.hpp"

namespace btree {

nodeptr build_internal(vec<uint64_t>& chkeys, vec<nodeptr>& chs) {
  uint64_t n = chkeys.size();

  if (n < 2*B) {
    nodeptr t = new_internal(0, n-1);
    for (uint8_t i = 1; i < n; i++) { t->keys[i-1] = chkeys[i]; }
    for (uint8_t i = 0; i < n; i++) { t->chs[i] = chs[i]; }
    return t; }

  uint64_t m = n/B;
  uint64_t r = n - m*B;
  vec<uint64_t> keys;
  vec<nodeptr> ts;

  for (uint64_t i = 0, k = 0; i < n; ) {
    uint8_t size = B + (k+r)/m;
    k = (k+r) % m;
    keys.push_back(chkeys[i]);
    ts.push_back(new_internal(0, size-1));
    ts.back()->chs[0] = chs[i++];
    for (uint8_t j = 1; j < size; i++, j++) {
      ts.back()->keys[j-1] = chkeys[i];
      ts.back()->chs[j] = chs[i]; } }

  return build_internal(keys, ts);
}

nodeptr build_sorted(uint64_t n, kv* elems) {
  if (n < 2*B) {
    nodeptr t = new_leaf(0, n);
    for (uint8_t i = 0; i < n; i++) {
      t->keys[i] = elems[i].first;
      t->vals[i] = elems[i].second; }
    return t; }

  uint64_t m = n/B;
  uint64_t r = n - m*B;
  vec<uint64_t> keys;
  vec<nodeptr> ts;

  for (uint64_t i = 0, k = 0; i < n; ) {
    uint8_t size = B + (k+r)/m;
    k = (k+r) % m;
    keys.push_back(elems[i].first);
    ts.push_back(new_leaf(0, size));
    for (uint8_t j = 0; j < size; i++, j++) {
      ts.back()->keys[j] = elems[i].first;
      ts.back()->vals[j] = elems[i].second; } }

  return build_internal(keys, ts);
}

nodeptr build(uint64_t n, kv* elems) {
  std::sort(elems, elems+n);
  return build_sorted(n, elems);
}

}
