#pragma once

#include <atomic>
#include <cstdint>
#include <cstring>
#include <thread>

#include "common.hpp"
#include "mpsc_list.hpp"

namespace conctrl {

template <typename T>
class alignas(128) concow {
private:
  static constexpr uint64_t BATCH_SIZE = LIBCONCTRL_BUFFER_SIZE/16;

  std::atomic_bool start_;
  std::atomic_bool stop_;

  alignas(uint64_t) uint8_t _flags_[0];

  const uint n_pipes_;
  const uint n_workers_;

  std::thread* const monitor_;

  struct alignas(128) pipe_context {
    std::thread p;
    volatile uint64_t pace = 0;
    void wait(uint64_t* cache, uint64_t expect) { while (*cache < expect) { *cache = load_consume(&pace); } }
    void advance(uint64_t next) { store_release(&pace, next); }
  } *const pipes_;

  struct alignas(128) worker_context {
    std::thread p;
    std::atomic_uint64_t tid = 0;
    worker_context* volatile next = nullptr;
  } *const workers_;

  alignas(128) mpsc_list<worker_context> workers_available_;

  alignas(128) std::atomic<typename T::nodeptr> root_;

  alignas(128) std::atomic_uint64_t n_submitted_;
  alignas(128) std::atomic_uint64_t n_committed_;

  alignas(128) std::atomic_uint64_t checkpoint_;

  struct alignas(128) {
    volatile uint64_t stage;
    uint64_t load() { return load_consume(&stage); }
    void store(uint64_t next) { store_release(&stage, next); }
  } stages_[BUFFER_SIZE];

  static_assert(alignof(typename T::context) == 128);
  T::context ctxs_[BUFFER_SIZE];

  inline void wait_version(T::context* ctx) {
    uint32_t ver = ctx->t_past->ver;
    while (ctx->cno < ver) {
      ctx->cno = (uint32_t)n_committed_.load(std::memory_order_acquire); }
  }

  inline void wait_stage(T::context* ctx, uint64_t stage) {
    uint32_t ver = ctx->t_past->ver;
    while (ctx->cno < ver) {
      uint64_t pstage = stages_[ver%BUFFER_SIZE].load();
      if (pstage >= stage) { break; }
      ctx->cno = (uint32_t)n_committed_.load(std::memory_order_acquire); }
  }

  inline void iter_exec(T::context* ctx) {
    if (ctx->wait_child) { return; } // will be handled by workers
    if (ctx->wait_subtree) { wait_version(ctx); }
    ctx->op = T::handlers[ctx->op](ctx);
  }

  void pipe_entry_iter_(uint64_t first, uint64_t last) {
    alignas(128) static thread_local uint64_t n_tasks = 0;
    alignas(128) static thread_local uint64_t last_checkpoint = 0;
    for (uint64_t tid = first+1; tid <= last; tid++) {
      pipes_[0].wait(&n_tasks, tid);
      auto ctx = &ctxs_[tid%BUFFER_SIZE];
      if (tid % BUFFER_SIZE == 0) [[unlikely]] {
        if (last_checkpoint < checkpoint_.load(std::memory_order_acquire)) {
          last_checkpoint = tid;
          ctx->t_past = ctxs_[(tid-1)%BUFFER_SIZE].root;
          wait_version(ctx);
          T::checkpoint_handler(ctx); }
        else { ctx->root = ctxs_[(tid-1)%BUFFER_SIZE].root; } }
      else {
        if (ctx->op == T::operand::INIT) [[likely]] {
          ctx->t_past = ctxs_[(tid-1)%BUFFER_SIZE].root;
          ctx->op = T::handlers[0](ctx);
          while (ctx->retry) [[unlikely]] { iter_exec(ctx); } }
        else /* just a bubble */ { /* assert(ctx->op == DONE); */ } }
      pipes_[1].advance(tid); }
  }

  void pipe_inner_iter_(uint pid, uint64_t first, uint64_t last) {
    alignas(128) static thread_local uint64_t pred_pace = 0;
    for (uint64_t tid = first+1; tid <= last; tid++) {
      pipes_[pid].wait(&pred_pace, tid);
      auto ctx = &ctxs_[tid%BUFFER_SIZE];
      if (ctx->op != T::operand::DONE) {
        iter_exec(ctx);
        while (ctx->retry) [[unlikely]] { iter_exec(ctx); } }
      pipes_[pid+1].advance(tid); }
  }

  void pipe_exit_iter_(uint64_t first, uint64_t last) {
    alignas(128) static thread_local uint64_t pred_pace = 0;
    for (uint64_t tid = first+1; tid <= last; tid++) {
      pipes_[n_pipes_].wait(&pred_pace, tid);
      auto ctx = &ctxs_[tid%BUFFER_SIZE];
      if (ctx->op != T::operand::DONE) [[likely]] {
        worker_context* wctx;
        do { wctx = workers_available_.pop(); } while (wctx == nullptr);
        wctx->tid.store(tid, std::memory_order_release); }
      else { stages_[tid%BUFFER_SIZE].store(~0UL); } }
  }

  enum pipetype { ENTRY, INNER, EXIT };

  template <pipetype PType>
  void pipe_main_(uint pid) {
    while (start_.load(std::memory_order_acquire) == 0);

    uint64_t local_epoch = 0;
    while (!stop_.load(std::memory_order_acquire)) {
      if constexpr (PType == ENTRY) { pipe_entry_iter_(local_epoch*BATCH_SIZE, (local_epoch+1)*BATCH_SIZE); }
      if constexpr (PType == INNER) { pipe_inner_iter_(pid, local_epoch*BATCH_SIZE, (local_epoch+1)*BATCH_SIZE); }
      if constexpr (PType == EXIT) { pipe_exit_iter_(local_epoch*BATCH_SIZE, (local_epoch+1)*BATCH_SIZE); }
      local_epoch++; }

    uint64_t n_tasks = n_submitted_.load(std::memory_order_acquire);
    if constexpr (PType == ENTRY) { pipe_entry_iter_(local_epoch*BATCH_SIZE, n_tasks); }
    if constexpr (PType == INNER) { pipe_inner_iter_(pid, local_epoch*BATCH_SIZE, n_tasks); }
    if constexpr (PType == EXIT) { pipe_exit_iter_(local_epoch*BATCH_SIZE, n_tasks); }
  }

  inline void worker_exec(T::context* ctx, uint64_t stage) {
    if (ctx->wait_subtree) { wait_version(ctx); }
    else { wait_stage(ctx, stage+ctx->wait_child); }
    ctx->op = T::handlers[ctx->op](ctx);
  }

  void worker_main_(uint wid) {
    while (start_.load(std::memory_order_acquire) == 0);

    worker_context* wctx = &workers_[wid-1];

    uint64_t last_tid = 0;
    while (true) {
      uint64_t tid;
      while ((tid = wctx->tid.load(std::memory_order_acquire)) == last_tid);
      if (tid == ~0UL) { break; }

      auto ctx = &ctxs_[tid%BUFFER_SIZE];
      for (uint64_t local_stage = 1; ctx->op != 0xf; local_stage++) {
        worker_exec(ctx, local_stage);
        while (ctx->retry) [[unlikely]] { worker_exec(ctx, local_stage); }
        stages_[(ctx->sno)%BUFFER_SIZE].store(local_stage); }
      stages_[(ctx->sno)%BUFFER_SIZE].store(~0UL);

      last_tid = tid;
      workers_available_.push(wctx); }
  }

  void monitor_main_() {
    while (start_.load(std::memory_order_acquire) == 0);
    bool stop = false;

    uint64_t n_submitted_local = 0;
    uint64_t n_inited_local = 0;
    uint64_t n_committed_local = 0;

    while (!stop || n_committed_local < n_submitted_local) {
      stop = stop_.load(std::memory_order_acquire);
      n_submitted_local = n_submitted_.load(std::memory_order_acquire);

      /* sync contexts from clients */
      uint64_t n_inited_next = n_inited_local;
      while (n_inited_next < n_submitted_local) {
        if (stages_[(n_inited_next+1)%BUFFER_SIZE].load()) { break; }
        ++n_inited_next; }
      if (n_inited_next > n_inited_local) {
        n_inited_local = n_inited_next;
        pipes_[0].advance(n_inited_local); }

      /* sync outputs of workers */
      uint64_t n_committed_next = n_committed_local;
      while (n_committed_next < n_inited_local) {
          if (~stages_[(n_committed_next+1)%BUFFER_SIZE].load()) { break; }
          /* to implement gc, append the root to stale list */
          ++n_committed_next;
          if (ctxs_[n_committed_next%BUFFER_SIZE].new_checkpoint) {
            checkpoint_.store(n_committed_next, std::memory_order_release); } }
      if (n_committed_next > n_committed_local) {
        n_committed_local = n_committed_next;
        root_.store(ctxs_[n_committed_local%BUFFER_SIZE].root, std::memory_order_release);
        n_committed_.store(n_committed_local, std::memory_order_release); } }

    /* append bubbles to terminate the pipeline */
    uint64_t margin = BATCH_SIZE + (BATCH_SIZE - (n_inited_local%BATCH_SIZE)) % BATCH_SIZE;
    pipes_[0].advance(n_inited_local+margin);
  }

public:
  concow(uint n_pipes, uint n_workers, T::nodeptr t_init) :
    start_(false), stop_(false),
    n_pipes_(n_pipes), n_workers_(n_workers),
    monitor_(new std::thread),
    pipes_(new pipe_context[n_pipes+1]),
    workers_(new worker_context[n_workers]),
    workers_available_(),
    root_(t_init),
    n_submitted_(0), n_committed_(0), checkpoint_(0)
  {
    memset((void*)ctxs_, 0, sizeof(ctxs_));
    for (uint i = 0; i < BUFFER_SIZE; i++) {
      stages_[i].store(~0UL);
      ctxs_[i].op = T::operand::DONE; }

    ctxs_[0].sno = 0;
    ctxs_[0].root = t_init;

    new (monitor_) std::thread(&concow::monitor_main_, this);
    new (&pipes_->p) std::thread(&concow::pipe_main_<ENTRY>, this, 0);
    for (uint i = 1; i < n_pipes; i++) {
      new (&(pipes_+i)->p) std::thread(&concow::pipe_main_<INNER>, this, i); }
    new (&(pipes_+n_pipes)->p) std::thread(&concow::pipe_main_<EXIT>, this, n_pipes);
    for (uint i = 0; i < n_workers; i++) {
      new (&(workers_+i)->p) std::thread(&concow::worker_main_, this, i+1);
      workers_available_.push(workers_+i); }

    start_.store(true, std::memory_order_release);
  }

  ~concow() {
    stop_.store(true, std::memory_order_release);
    monitor_->join();

    std::atomic_thread_fence(std::memory_order_seq_cst);

    for (uint i = 0; i <= n_pipes_; i++) { pipes_[i].p.join(); }

    std::atomic_thread_fence(std::memory_order_seq_cst);

    for (uint i = 0; i < n_workers_; i++) { workers_[i].tid.store(~0UL, std::memory_order_release); }
    for (uint i = 0; i < n_workers_; i++) { workers_[i].p.join(); }

    std::atomic_thread_fence(std::memory_order_seq_cst);

    delete monitor_;
    delete []pipes_;
    delete []workers_;
  }

  void wait_for_processing(uint64_t n) {
    while (n_committed_.load(std::memory_order_acquire) < n);
  }

  uint64_t update(uint64_t key, uint64_t val) {
    uint64_t tid;

    while (true) {
      tid = 1+n_submitted_.fetch_add(1, std::memory_order_acquire);
      // simply spin if the buffer is full
      while (tid >= n_committed_.load(std::memory_order_acquire) + BUFFER_SIZE);
      if (tid % BUFFER_SIZE != 0) [[likely]] { break; }
      ctxs_[0].sno = (uint32_t)tid;
      stages_[0].store(0); }

    uint64_t pos = tid % BUFFER_SIZE;
    ctxs_[pos].op = T::operand::INIT;
    ctxs_[pos].flags = 0;
    ctxs_[pos].sno = (uint32_t)tid;
    ctxs_[pos].key = key;
    ctxs_[pos].val = val;
    stages_[pos].store(0); // to indicate the context is inited

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
