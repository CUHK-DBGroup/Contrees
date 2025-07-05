#pragma once

#include <cstdint>

#include "lib/common/types.hpp"
#include "node_wrapper.hpp"

namespace aert {

static inline opt<uint64_t> find(cnodeptr t, uint64_t key) {
  while (t != nullptr) {
    uint8_t ppfx;
    std::tie(t, ppfx) = unembed_ptr(t);
    if ((ppfx & 0x10) && ptr_pfx_match(ppfx, t->pfx_ofs, key) < 4) { return std::nullopt; }
    uint8_t pmlen = prefix_match(t, key) & 0xf8;
    if (t->pfx_len > pmlen) { return std::nullopt; }
    if (t->type == NODE_TYPE::LEAF) { return std::make_optional(((leaf*)t)->val); }
    uint8_t pkey = partial_key(key, t->pfx_ofs+t->pfx_len, node_key_len(t->type));
    t = findch(t, pkey); }
  return std::nullopt;
}

void scan_append(cnodeptr t, opt<uint64_t> pkey, uint64_t n, vec<kv>& ret);

void node4_scan_append(const node4* t, opt<uint64_t> pkey, uint64_t n, vec<kv>& ret) {
  uint8_t idx = 0;
  if (pkey) { idx = node4_findgt(t, pkey.value()); }

  for (; ret.size() < n && idx < t->size; idx++) {
    scan_append(t->chs[idx], std::nullopt, n, ret); }
}

void node16_scan_append(const node16* t, opt<uint64_t> pkey, uint64_t n, vec<kv>& ret) {
  uint8_t idx = 0;
  if (pkey) { idx = node16_findgt(t, pkey.value()); }

  for (; ret.size() < n && idx < t->size; idx++) {
    scan_append(t->chs[idx], std::nullopt, n, ret); }
}

void nodeh4_scan_append(const nodeh4* t, opt<uint64_t> pkey, uint64_t n, vec<kv>& ret) {
  uint8_t idx = 0;
  if (pkey) { idx = nodeh4_findgt(t, pkey.value()); }

  for (; ret.size() < n && idx < t->size; idx++) {
    scan_append(t->chs[idx], std::nullopt, n, ret); }
}

void nodeh16_scan_append(const nodeh16 *t, opt<uint64_t> pkey, uint64_t n, vec<kv>& ret) {
  uint8_t idx = 0;
  if (pkey) { idx = pkey.value()+1; }

  for (; ret.size() < n && idx < 16; idx++) {
    if (t->chs[idx]) { scan_append(t->chs[idx], std::nullopt, n, ret); } }
}

void scan_append(cnodeptr t, opt<uint64_t> pkey, uint64_t n, vec<kv>& ret) {
  t = extract_ptr(t);
  switch (t->type) {
    case NODE4:   return node4_scan_append((const node4*)t, pkey, n, ret);
    case NODE16:  return node16_scan_append((const node16*)t, pkey, n, ret);
    case NODEH4:  return nodeh4_scan_append((const nodeh4*)t, pkey, n, ret);
    case NODEH16: return nodeh16_scan_append((const nodeh16*)t, pkey, n, ret);
    default: ret.emplace_back(t->pfx, ((leaf*)t)->val); }
}

void scan_search(cnodeptr t, uint64_t key, uint64_t n, vec<kv>& ret) {
  if (t == nullptr) { return; }
  uint8_t ppfx;
  std::tie(t, ppfx) = unembed_ptr(t);
  if ((ppfx & 0x10) && ptr_pfx_match(ppfx, t->pfx_ofs, key) < 4) {
    uint8_t hpfx = partial_key(key, t->pfx_ofs, 4);
    if ((ppfx&0xf) < hpfx) { return; }
    scan_append(extract_ptr(t), std::nullopt, n, ret);
    return; }

  uint8_t pmlen = prefix_match(t, key) & 0xf8;
  if (t->pfx_len > pmlen) {
    uint8_t t_pkey = partial_key(t->pfx, t->pfx_ofs+pmlen);
    uint8_t pkey = partial_key(key, t->pfx_ofs+pmlen);
    if (t_pkey < pkey) { return; }
    scan_append(extract_ptr(t), std::nullopt, n, ret);
    return; }

  if (t->type == LEAF) {
    ret.emplace_back(t->pfx, ((leaf*)t)->val);
    return; }

  uint8_t pkey = partial_key(key, t->pfx_ofs+t->pfx_len, node_key_len(t->type));
  scan_search(findch(t, pkey), key, n, ret);
  scan_append(extract_ptr(t), std::make_optional(pkey), n, ret);
}

static inline vec<kv> scan(cnodeptr t, uint64_t key, uint64_t n) {
  vec<kv> ret;
  scan_search(t, key, n, ret);
  return ret;
}

}
