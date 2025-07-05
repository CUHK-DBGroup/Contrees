#pragma once

#include <atomic>
#include <cstdint>
#include <cstring>
#include <thread>

#include "common.hpp"
#include "mpsc_list.hpp"

namespace conctrl {

template <typename T, uint P>
class alignas(128) concow_cyclic {
private:
  static constexpr uint N_PIPES = P+1;
  static constexpr uint64_t BATCH_ROUNDS = LIBCONCTRL_BUFFER_SIZE/32;
  static constexpr uint64_t BATCH_SIZE = BATCH_ROUNDS*N_PIPES;

  std::atomic_bool start_;
  std::atomic_bool stop_;

  alignas(uint64_t) uint8_t _flags_[0];

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
  alignas(128) std::atomic_uint64_t n_inited_;
  alignas(128) std::atomic_uint64_t n_committed_;

  struct alignas(128) {
    volatile uint64_t stage;
    uint64_t load() { return load_consume(&stage); }
    void store(uint64_t next) { store_release(&stage, next); }
  } stages_[BUFFER_SIZE];

  static_assert(alignof(typename T::context) == 128);
  T::context ctxs_[BUFFER_SIZE];

  inline void wait_stage(T::context* ctx, uint64_t stage) {
    uint32_t ver = ctx->t_past->ver;
    while (ctx->cno < ver) {
      uint64_t pstage = stages_[ver%BUFFER_SIZE].load();
      if (pstage >= stage) { break; }
      ctx->cno = (uint32_t)n_committed_.load(std::memory_order_acquire); }
  }

  template <uint PID>
  void process_entry(uint64_t tid, uint64_t* n_tasks) {
    while (*n_tasks < tid) { *n_tasks = n_inited_.load(std::memory_order_acquire); }
    auto ctx = &ctxs_[tid%BUFFER_SIZE];
    ctx->t_past = ctxs_[(tid-1)%BUFFER_SIZE].root;
    if (ctx->op != 0xf) [[likely]] { ctx->op = T::handlers[ctx->op](ctx); }
    pipes_[(PID+1)%N_PIPES].advance(tid);
  }

  template <uint PID>
  void process_inner(uint64_t tid, uint64_t* pred_pace) {
    pipes_[PID].wait(pred_pace, tid);
    auto ctx = &ctxs_[tid%BUFFER_SIZE];
    if (ctx->op != 0xf) [[likely]] { ctx->op = T::handlers[ctx->op](ctx); }
    pipes_[(PID+1)%N_PIPES].advance(tid);
  }

  template <uint PID>
  void process_exit(uint64_t tid, uint64_t* pred_pace, uint64_t* n_compl) {
    pipes_[PID].wait(pred_pace, tid);
    auto ctx = &ctxs_[tid%BUFFER_SIZE];
    if (ctx->op != 0xf) [[likely]] {
      ctx->cno = *n_compl;
      wait_stage(ctx, 1);
      *n_compl = ctx->cno;
      ctx->op = T::handlers[ctx->op](ctx);
      worker_context* wctx = workers_available_.pop();
      while (wctx == nullptr) { wctx = workers_available_.pop(); }
      wctx->tid.store(tid, std::memory_order_release); }
    else { stages_[tid%BUFFER_SIZE].store(~0UL); }
  }

  template <uint PID, uint TID>
  inline void process_step(uint64_t tbase, uint64_t* n_tasks, uint64_t* pred_pace, uint64_t* n_compl) {
    if constexpr (PID == TID) { process_entry<PID>(tbase+TID, n_tasks); }
    else if constexpr ((PID+1)%N_PIPES == TID) { process_exit<PID>(tbase+TID, pred_pace, n_compl); }
    else { process_inner<PID>(tbase+TID, pred_pace); }
  }

  template <uint PID, uint TID>
  void pipe_subloop_(uint64_t tbase, uint64_t* n_tasks, uint64_t* pred_pace, uint64_t* n_compl) {
    if constexpr (PID == TID) { process_entry<PID>(tbase+TID, n_tasks); }
    else if constexpr ((PID+1)%N_PIPES == TID) { process_exit<PID>(tbase+TID, pred_pace, n_compl); }
    else { process_inner<PID>(tbase+TID, pred_pace); }

    if constexpr (TID+1 < N_PIPES) { pipe_subloop_<PID, TID+1>(tbase, n_tasks, pred_pace, n_compl); }
  }

  template <uint PID>
  void pipe_iter_(uint64_t first, uint64_t last) {
    alignas(128) static thread_local struct {
      uint64_t n_tasks = 0;
      uint64_t pred_pace = 0;
      uint64_t n_compl = 0;
    } cache;
    for (uint64_t tid = first+1; tid <= last; tid += N_PIPES) {
      pipe_subloop_<PID, 0>(tid, &cache.n_tasks, &cache.pred_pace, &cache.n_compl); }
  }

  template <uint PID>
  void pipe_main_() {
    while (start_.load(std::memory_order_acquire) == 0);

    uint64_t local_epoch = 0;
    while (!stop_.load(std::memory_order_acquire)) {
      pipe_iter_<PID>(local_epoch*BATCH_SIZE, (local_epoch+1)*BATCH_SIZE);
      local_epoch++; }

    uint64_t n_tasks = n_submitted_.load(std::memory_order_acquire);
    pipe_iter_<PID>(local_epoch*BATCH_SIZE, n_tasks);
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
        /* assert(ctx->op != 0x0; */
        wait_stage(ctx, local_stage+1);
        ctx->op = T::handlers[ctx->op](ctx);
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
        n_inited_.store(n_inited_local, std::memory_order_release); }

      /* sync outputs of workers */
      uint64_t n_committed_next = n_committed_local;
      while (n_committed_next < n_inited_local) {
          if (~stages_[(n_committed_next+1)%BUFFER_SIZE].load()) { break; }
          ++n_committed_next; }
      if (n_committed_next > n_committed_local) {
        n_committed_local = n_committed_next;
        root_.store(ctxs_[n_committed_local%BUFFER_SIZE].root, std::memory_order_release);
        n_committed_.store(n_committed_local, std::memory_order_release); } }

    /* append bubbles to terminate the pipeline */
    uint64_t margin = BATCH_SIZE + (BATCH_SIZE - (n_inited_local%BATCH_SIZE)) % BATCH_SIZE;
    n_inited_.store(n_inited_local+margin+N_PIPES, std::memory_order_release);
  }

  template <uint I>
  void create_pipes_() {
    new (&pipes_[I].p) std::thread(&concow_cyclic::pipe_main_<I>, this);
    if constexpr (I+1 < N_PIPES) { create_pipes_<I+1>(); }
  }

public:
  concow_cyclic(uint n_workers, T::nodeptr t_init) :
    start_(false), stop_(false), n_workers_(n_workers),
    monitor_(new std::thread),
    pipes_(new pipe_context[N_PIPES]),
    workers_(new worker_context[n_workers]),
    workers_available_(),
    root_(t_init),
    n_submitted_(0), n_committed_(0)
  {
    memset((void*)ctxs_, 0, sizeof(ctxs_));
    for (uint i = 0; i < BUFFER_SIZE; i++) {
      stages_[i].store(~0UL);
      ctxs_[i].op = T::operand::DONE; }

    ctxs_[0].sno = 0;
    ctxs_[0].root = t_init;

    new (monitor_) std::thread(&concow_cyclic::monitor_main_, this);
    create_pipes_<0>();
    for (uint i = 0; i < n_workers; i++) {
      new (&(workers_+i)->p) std::thread(&concow_cyclic::worker_main_, this, i+1);
      workers_available_.push(workers_+i); }

    start_.store(true, std::memory_order_release);
  }

  ~concow_cyclic() {
    stop_.store(true, std::memory_order_release);
    monitor_->join();

    std::atomic_thread_fence(std::memory_order_seq_cst);

    for (uint i = 0; i < N_PIPES; i++) { pipes_[i].p.join(); }

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
    uint64_t tid = 1+n_submitted_.fetch_add(1, std::memory_order_acquire);

    // simply spin if the buffer is full
    while (tid >= n_committed_.load(std::memory_order_acquire) + BUFFER_SIZE);

    uint64_t pos = tid % BUFFER_SIZE;
    ctxs_[pos].op = T::operand::INIT;
    ctxs_[pos].sno = (uint32_t)tid;
    ctxs_[pos].key = key;
    ctxs_[pos].val = val;
    stages_[pos].store(0); // to indicate the context is inited

    return tid;
  }

  template <typename F>
  auto query(F&& f) {
    typename T::cnodeptr root = root_.load(std::memory_order_acquire);
    return f(root);
  }
};

}
