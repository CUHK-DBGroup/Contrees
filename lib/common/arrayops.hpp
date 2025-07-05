#pragma once

#include <cstdint>
#include <cstring>

template <typename T>
static inline void copy(T* dst, T* src, uint64_t size) {
  memcpy(dst, src, size*sizeof(T));
}

template <typename T>
static inline void insert(T* arr, uint64_t size, uint64_t pos, T elem) {
  memmove(arr+pos+1, arr+pos, (size-pos)*sizeof(T));
  arr[pos] = elem;
}

template <typename T>
static inline void copy_insert(T* dst, T* src, uint64_t size, uint64_t pos, T elem) {
  memcpy(dst, src, pos*sizeof(T));
  dst[pos] = elem;
  memcpy(dst+pos+1, src+pos, (size-pos)*sizeof(T));
}

template <typename T>
static inline void copy_update(T* dst, T* src, uint64_t size, uint64_t pos, T elem) {
  memcpy(dst, src, size*sizeof(T));
  dst[pos] = elem;
}

template <typename T>
static inline void copy_split(T* dsth, T* dstt, T* src, uint64_t size, uint64_t pos) {
  memcpy(dsth, src, pos*sizeof(T));
  memcpy(dstt, src+pos, (size-pos)*sizeof(T));
}
