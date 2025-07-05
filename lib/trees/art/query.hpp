#pragma once

#include <cstdint>

#include "lib/common/types.hpp"
#include "node_wrapper.hpp"

namespace art {

static inline opt<uint64_t> find(cnodeptr t, uint64_t key) {
  while (t != nullptr) {
    uint8_t pmlen = prefix_match(t, key);
    if (t->pfx_len > pmlen) { return std::nullopt; }
    if (t->type == LEAF) { return std::make_optional(((leaf*)t)->val); }
    uint8_t pkey = partial_key(key, t->pfx_ofs+t->pfx_len);
    t = findch(t, pkey); }
  return std::nullopt;
}

void scan_append(cnodeptr t, opt<uint64_t> pkey, uint64_t n, vec<kv>& ret);

void node4_scan_append(const node4 *t, opt<uint64_t> pkey, uint64_t n, vec<kv>& ret) {
  uint8_t idx = 0;
  if (pkey) { idx = node4_findgt(t, pkey.value()); }

  for (; ret.size() < n && idx < t->size; idx++) {
    scan_append(t->chs[idx], std::nullopt, n, ret); }
}

void node16_scan_append(const node16 *t, opt<uint64_t> pkey, uint64_t n, vec<kv>& ret) {
  uint8_t idx = 0;
  if (pkey) { idx = node16_findgt(t, pkey.value()); }

  for (; ret.size() < n && idx < t->size; idx++) {
    scan_append(t->chs[idx], std::nullopt, n, ret); }
}

void node48_scan_append(const node48 *t, opt<uint64_t> pkey, uint64_t n, vec<kv>& ret) {
  uint8_t idx = 0;
  if (pkey) { if ((idx = pkey.value()+1) == 0) { return; } }

  while (ret.size() < n) {
    if (t->slts[idx] != (uint8_t)~0) { scan_append(t->chs[t->slts[idx]], std::nullopt, n, ret); }
    if (++idx == 0) { break; } }
}

void node256_scan_append(const node256 *t, opt<uint64_t> pkey, uint64_t n, vec<kv>& ret) {
  uint8_t idx = 0;
  if (pkey) { if ((idx = pkey.value()+1) == 0) { return; } }

  while (ret.size() < n) {
    if (t->chs[idx]) { scan_append(t->chs[idx], std::nullopt, n, ret); }
    if (++idx == 0) { break; } }
}

void scan_append(cnodeptr t, opt<uint64_t> pkey, uint64_t n, vec<kv>& ret) {
  switch (t->type) {
    case NODE4:   return node4_scan_append((const node4*)t, pkey, n, ret);
    case NODE16:  return node16_scan_append((const node16*)t, pkey, n, ret);
    case NODE48:  return node48_scan_append((const node48*)t, pkey, n, ret);
    case NODE256: return node256_scan_append((const node256*)t, pkey, n, ret);
    default: ret.emplace_back(t->pfx, ((leaf*)t)->val); }
}

void scan_search(cnodeptr t, uint64_t key, uint64_t n, vec<kv>& ret) {
  if (t == nullptr) { return; }
  uint8_t pmlen = prefix_match(t, key);

  if (t->pfx_len > pmlen) {
    uint8_t t_pkey = partial_key(t->pfx, t->pfx_ofs+pmlen);
    uint8_t pkey = partial_key(key, t->pfx_ofs+pmlen);
    if (t_pkey < pkey) { return; }
    scan_append(t, std::nullopt, n, ret);
    return; }

  if (t->type == LEAF) {
    ret.emplace_back(t->pfx, ((leaf*)t)->val);
    return; }

  uint8_t pkey = partial_key(key, t->pfx_ofs+t->pfx_len);
  scan_search(findch(t, pkey), key, n, ret);
  scan_append(t, std::make_optional(pkey), n, ret);
}

static inline vec<kv> scan(cnodeptr t, uint64_t key, uint64_t n) {
  vec<kv> ret;
  scan_search(t, key, n, ret);
  return ret;
}

}
