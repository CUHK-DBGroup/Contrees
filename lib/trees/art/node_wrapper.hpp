#pragma once

#include <cstdint>

#include "lib/common/types.hpp"
#include "leaf.hpp"
#include "node4.hpp"
#include "node16.hpp"
#include "node48.hpp"
#include "node256.hpp"

namespace art {

static inline opt<uint8_t> find_idx(cnodeptr t, uint8_t pkey) {
  switch (t->type) {
    case NODE4:   return node4_find((const node4*)t, pkey);
    case NODE16:  return node16_find((const node16*)t, pkey);
    case NODE48:  return node48_find((const node48*)t, pkey);
    case NODE256: return node256_find((const node256*)t, pkey);
    default: /* assert(false); */ return std::nullopt; }
}

static inline nodeptr* getch(nodeptr t, uint8_t idx) {
  switch (t->type) {
    case NODE4:   return node4_getch((node4*)t, idx);
    case NODE16:  return node16_getch((node16*)t, idx);
    case NODE48:  return node48_getch((node48*)t, idx);
    case NODE256: return node256_getch((node256*)t, idx);
    default: /* assert(false); */ return nullptr; }
}

static inline nodeptr findch(cnodeptr t, uint8_t pkey) {
  switch (t->type) {
    case NODE4:   return node4_findch((const node4*)t, pkey);
    case NODE16:  return node16_findch((const node16*)t, pkey);
    case NODE48:  return node48_findch((const node48*)t, pkey);
    case NODE256: return node256_findch((const node256*)t, pkey);
    default: /* assert(false); */ return nullptr; }
}

static inline uint8_t append(nodeptr t, uint8_t pkey) {
  switch (t->type) {
    case NODE4:   return node4_append((node4*)t, pkey);
    case NODE16:  return node16_append((node16*)t, pkey);
    case NODE48:  return node48_append((node48*)t, pkey);
    case NODE256: return node256_append((node256*)t, pkey);
    default: /* assert(false); */ return 0; }
}

static inline void insertch(nodeptr t, uint8_t idx, nodeptr ch) {
  switch (t->type) {
    case NODE4:   return node4_insertch((node4*)t, idx, ch);
    case NODE16:  return node16_insertch((node16*)t, idx, ch);
    case NODE48:  return node48_insertch((node48*)t, idx, ch);
    case NODE256: return node256_insertch((node256*)t, idx, ch);
    default: /* assert(false); */ return; }
}

static inline bool is_full(nodeptr t) {
  switch (t->type) {
    case NODE4:   return t->size == 4;
    case NODE16:  return t->size == 16;
    case NODE48:  return t->size == 48;
    case NODE256: return false;
    default: /* assert(false); */ return true; }
}

static inline nodeptr node_extend(nodeptr t, uint64_t ver) {
  switch (t->type) {
    case NODE4:  return (nodeptr)node4_extend((node4*)t, ver);
    case NODE16: return (nodeptr)node16_extend((node16*)t, ver);
    case NODE48: return (nodeptr)node48_extend((node48*)t, ver);
    default: /* assert(false); */ return nullptr; }
}

static inline void copych(nodeptr t_new, nodeptr t_old) {
  switch (t_new->type) {
    case NODE4:   return node4_copych((node4*)t_new, (node4*)t_old);
    case NODE16:  return node16_copych((node16*)t_new, (node16*)t_old);
    case NODE48:  return node48_copych((node48*)t_new, (node48*)t_old);
    case NODE256: return node256_copych((node256*)t_new, (node256*)t_old);
    default: /* assert(false); */ return; }
}

static inline void copych(nodeptr t_new, nodeptr t_old, uint8_t idx) {
  if (t_old->type != t_new->type) {
    switch (t_old->type) {
      case NODE4:  return node4_copych_extend((node16*)t_new, (node4*)t_old, idx);
      case NODE16: return node16_copych_extend((node48*)t_new, (node16*)t_old);
      case NODE48: return node48_copych_extend((node256*)t_new, (node48*)t_old);
      default: /* assert(false); */ return; } }
  else {
    switch (t_new->type) {
      case NODE4:   return node4_copych((node4*)t_new, (node4*)t_old, idx);
      case NODE16:  return node16_copych((node16*)t_new, (node16*)t_old, idx);
      case NODE48:  return node48_copych((node48*)t_new, (node48*)t_old);
      case NODE256: return node256_copych((node256*)t_new, (node256*)t_old);
      default: /* assert(false); */ return; } }
}

static inline void print_node(cnodeptr t) {
  if (t == nullptr) {
#ifndef NDEBUG
    fprintf(stderr, "EMPTY");
#endif
    return; }
  switch (t->type) {
    case NODE4:   print_node4((const node4*)t); break;
    case NODE16:  print_node16((const node16*)t); break;
    case NODE48:  print_node48((const node48*)t); break;
    case NODE256: print_node256((const node256*)t); break;
    default :     print_leaf((const leaf*)t); }
}

}
