#pragma once

#include <atomic>
#include <cstdint>
#include <cstring>
#include <thread>

#include "common.hpp"
#include "context.hpp"

namespace conctrl {

template <typename T>
class alignas(128) seqcow {
private:
  static constexpr uint64_t BATCH_SIZE = LIBCONCTRL_BUFFER_SIZE/16;

  std::atomic_bool start_;
  std::atomic_bool stop_;

  std::thread* const worker_;

  alignas(128) std::atomic<typename T::nodeptr> root_;

  alignas(128) std::atomic_uint64_t n_submitted_;
  alignas(128) std::atomic_uint64_t n_committed_;

  alignas(128) std::atomic_uint64_t checkpoint_;

  static_assert(sizeof(typename T::context) == 128);
  static_assert(alignof(typename T::context) == 128);
  T::context ctxs_[BUFFER_SIZE];

  void exec_task_(uint64_t tid) {
    auto ctx = &ctxs_[tid%BUFFER_SIZE];
    ctx->t_past = ctxs_[(tid-1)%BUFFER_SIZE].root;
    ctx->op = T::handlers[0](ctx);
    while (ctx->op != DONE) { ctx->op = T::handlers[ctx->op](ctx); }
  }

  void worker_main_() {
    while (start_.load(std::memory_order_acquire) == 0);
    bool stop = false;

    uint64_t n_submitted_local = 0;
    uint64_t n_committed_local = 0;

    while (!stop || n_committed_local < n_submitted_local) {
      stop = stop_.load(std::memory_order_acquire);
      n_submitted_local = n_submitted_.load(std::memory_order_acquire);

      uint64_t n_committed_next = n_committed_local;
      while (n_committed_next < n_submitted_local) {
        exec_task_(++n_committed_next);
        if (n_committed_next > n_committed_local + BUFFER_SIZE / 2) { break; }
      }
      if (n_committed_next > n_committed_local) {
        n_committed_local = n_committed_next;
        root_.store(ctxs_[n_committed_local%BUFFER_SIZE].root, std::memory_order_release);
        n_committed_.store(n_committed_local, std::memory_order_release); } }
  }

public:
  seqcow(T::nodeptr t_init) :
    start_(false), stop_(false),
    worker_(new std::thread),
    root_(t_init),
    n_submitted_(0), n_committed_(0), checkpoint_(0)
  {
    memset((void*)ctxs_, 0, sizeof(ctxs_));
    for (uint i = 0; i < BUFFER_SIZE; i++) { ctxs_[i].op = T::operand::DONE; }

    ctxs_[0].sno = 0;
    ctxs_[0].root = t_init;

    new (worker_) std::thread(&seqcow::worker_main_, this);
    start_.store(true, std::memory_order_release);
  }

  ~seqcow() {
    stop_.store(true, std::memory_order_release);
    worker_->join();

    std::atomic_thread_fence(std::memory_order_seq_cst);

    delete worker_;
  }

  void wait_for_processing(uint64_t n) {
    while (n_committed_.load(std::memory_order_acquire) < n);
  }

  uint64_t update(uint64_t key, uint64_t val) {
    uint64_t tid;

    tid = 1+n_submitted_.fetch_add(1, std::memory_order_acquire);
    while (tid >= n_committed_.load(std::memory_order_acquire) + BUFFER_SIZE);

    uint64_t pos = tid % BUFFER_SIZE;
    ctxs_[pos].op = T::operand::INIT;
    ctxs_[pos].flags = 0;
    ctxs_[pos].sno = (uint32_t)tid;
    ctxs_[pos].key = key;
    ctxs_[pos].val = val;

    return tid;
  }

  template <typename F>
  auto query(F&& f) {
    typename T::cnodeptr root = root_.load(std::memory_order_acquire);
    /* to implement gc, use hazard ptr to access root */
    return f(root);
  }
};

}
