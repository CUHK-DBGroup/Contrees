#pragma once

#include <cstdint>

#include "leaf.hpp"
#include "node4.hpp"
#include "node16.hpp"
#include "nodeh4.hpp"
#include "nodeh16.hpp"

namespace aert {

static inline uint8_t findidx(nodeptr t, uint8_t pkey) {
  switch (t->type) {
    case NODE4:   return node4_find((node4*)t, pkey);
    case NODE16:  return node16_find((node16*)t, pkey);
    case NODEH4:  return nodeh4_find((nodeh4*)t, pkey);
    case NODEH16: return nodeh16_find((nodeh16*)t, pkey);
    default: /* assert(false); */ return 0; }
}

static inline nodeptr* getch(nodeptr t, uint8_t idx) {
  switch (t->type) {
    case NODE4:   return node4_getch((node4*)t, idx);
    case NODE16:  return node16_getch((node16*)t, idx);
    case NODEH4:  return nodeh4_getch((nodeh4*)t, idx);
    case NODEH16: return nodeh16_getch((nodeh16*)t, idx);
    default: /* assert(false); */ return nullptr; }
}

static inline nodeptr findch(cnodeptr t, uint8_t pkey) {
  switch (t->type) {
    case NODE4:   return node4_findch((const node4*)t, pkey);
    case NODE16:  return node16_findch((const node16*)t, pkey);
    case NODEH4:  return nodeh4_findch((const nodeh4*)t, pkey);
    case NODEH16: return nodeh16_findch((const nodeh16*)t, pkey);
    default: /* assert(false); */ return nullptr; }
}

static inline uint8_t append(nodeptr t, uint8_t pkey) {
  switch (t->type) {
    case NODE4:   return node4_append((node4*)t, pkey);
    case NODE16:  return node16_append((node16*)t, pkey);
    case NODEH4:  return nodeh4_append((nodeh4*)t, pkey);
    case NODEH16: return nodeh16_append((nodeh16*)t, pkey);
    default: /* assert(false); */ return 0; }
}

static inline uint8_t copy_append(nodeptr* tp, nodeptr t, uint64_t ver, uint8_t pkey) {
  switch (t->type) {
    case NODE4:   return node4_copy_append(tp, (node4*)t, ver, pkey);
    case NODE16:  return node16_copy_append(tp, (node16*)t, ver, pkey);
    case NODEH4:  return nodeh4_copy_append(tp, (nodeh4*)t, ver, pkey);
    case NODEH16: return nodeh16_copy_append(tp, (nodeh16*)t, ver, pkey);
    default: /* assert(false); */ return 0; }
}

static inline void insertch(nodeptr t, uint8_t idx, nodeptr ch) {
  switch (t->type) {
    case NODE4:   return node4_insertch((node4*)t, idx, ch);
    case NODE16:  return node16_insertch((node16*)t, idx, ch);
    case NODEH4:  return nodeh4_insertch((nodeh4*)t, idx, ch);
    case NODEH16: return nodeh16_insertch((nodeh16*)t, idx, ch);
    default: /* assert(false); */ return; }
}

static inline bool is_full(nodeptr t) {
  switch (t->type) {
    case NODE4:   return t->size == 4;
    case NODE16:  return t->size == 16;
    case NODEH4:  return t->size == 4;
    case NODEH16: return false;
    default: /* assert(false); */ return true; }
}

static inline nodeptr node_extend(nodeptr t, uint64_t ver) {
  switch (t->type) {
    case NODE4:  return (nodeptr)node4_extend((node4*)t, ver);
    case NODEH4: return (nodeptr)nodeh4_extend((nodeh4*)t, ver);
    default: /* assert(false); */ return nullptr; }
}

static inline uint8_t extend_append(nodeptr* tp, nodeptr t, uint64_t ver, uint8_t pkey) {
  switch (t->type) {
    case NODE4:  return node4_extend_append(tp, (node4*)t, ver, pkey);
    case NODEH4: return nodeh4_extend_append(tp, (nodeh4*)t, ver, pkey);
    default: /* assert(false); */ return 0; }
}

static inline void copych(nodeptr t_new, nodeptr t_old) {
  if (t_old->type != t_new->type) {
    switch (t_old->type) {
      case NODE4:  return node4_copych_extend((node16*)t_new, (node4*)t_old);
      case NODEH4: return nodeh4_copych_extend((nodeh16*)t_new, (nodeh4*)t_old);
      default: /* assert(false); */ return; } }
  else {
    switch (t_new->type) {
      case NODE4:   return node4_copych((node4*)t_new, (node4*)t_old);
      case NODE16:  return node16_copych((node16*)t_new, (node16*)t_old);
      case NODEH4:  return nodeh4_copych((nodeh4*)t_new, (nodeh4*)t_old);
      case NODEH16: return nodeh16_copych((nodeh16*)t_new, (nodeh16*)t_old);
      default: /* assert(false); */ return; } }
}

static inline void copych(nodeptr t_new, nodeptr t_old, uint8_t idx) {
  if (t_old->type != t_new->type) {
    switch (t_old->type) {
      case NODE4:  return node4_copych_extend((node16*)t_new, (node4*)t_old, idx);
      case NODEH4: return nodeh4_copych_extend((nodeh16*)t_new, (nodeh4*)t_old);
      default: /* assert(false); */ return; } }
  else {
    switch (t_new->type) {
      case NODE4:   return node4_copych((node4*)t_new, (node4*)t_old, idx);
      case NODE16:  return node16_copych((node16*)t_new, (node16*)t_old, idx);
      case NODEH4:  return nodeh4_copych((nodeh4*)t_new, (nodeh4*)t_old, idx);
      case NODEH16: return nodeh16_copych((nodeh16*)t_new, (nodeh16*)t_old);
      default: /* assert(false); */ return; } }
}

}
