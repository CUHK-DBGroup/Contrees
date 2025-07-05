#pragma once

#include <bit>
#include <cstdint>

#include "mimalloc.h"
#include "mimalloc-new-delete.h"
#include "mimalloc-override.h"

#ifndef LIBCONCTRL_BUFFER_SIZE
#define LIBCONCTRL_BUFFER_SIZE 65536
#endif

namespace conctrl {

static inline constexpr uint BUFFER_SIZE = std::bit_ceil<uint>(LIBCONCTRL_BUFFER_SIZE);

template<typename T>
T load_consume(T const volatile* addr)
{
  T v = *const_cast<T const volatile*>(addr);
  std::atomic_signal_fence(std::memory_order_acq_rel);
  return v;
}

template<typename T>
void store_release(T volatile* addr, T v)
{
  std::atomic_signal_fence(std::memory_order_acq_rel);
  *const_cast<T volatile*>(addr) = v;
}

}
