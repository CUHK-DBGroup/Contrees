#pragma once

#include <cstdint>

namespace conctrl {

static inline constexpr uint8_t INIT = 0x0;
static inline constexpr uint8_t DONE = 0xf;

template <typename T>
struct context {
  uint8_t op;

  union {
    uint8_t flags;
    struct {
      uint8_t retry          : 1;
      uint8_t wait_child     : 1;
      uint8_t wait_subtree   : 1;
      uint8_t new_checkpoint : 1;
    };
  };

  alignas(uint64_t) uint32_t sno;
  uint32_t cno;

  T* root;
  T* t_cur;
  T* t_past;

  uint64_t key;
  uint64_t val;
};

}
