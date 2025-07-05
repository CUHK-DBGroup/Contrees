#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <thread>

#include "utils/log.h"
#include "utils/timer.hpp"
#include "lib/common/types.hpp"
#include "lib/conctrl/concow.hpp"
#include "lib/conctrl/concow_cyclic.hpp"
#include "lib/conctrl/seqcow.hpp"
#include "config.hpp"

#include "adapters/betree.hpp"
#include "adapters/btree.hpp"
#include "adapters/aert.hpp"
#include "adapters/art.hpp"

struct tx_context {
  uint8_t type;
  uint64_t arg0;
  uint64_t arg1;
};

template<typename T, typename S>
void multi_client_execute(uint64_t num_clients, uint64_t m, tx_context* txs, S* schd) {
  static constexpr uint64_t BATCH_SIZE = 64;
  alignas(128) std::atomic_uint64_t cursor(0);
  alignas(128) std::atomic_uint64_t n_submitted(0);
  std::vector<std::thread> threads;

  for (uint64_t cid = 0; cid < num_clients; cid++) {
    threads.emplace_back([cid, m, txs, schd, &cursor, &n_submitted]() {
      while (true) {
        uint64_t b = cursor.fetch_add(BATCH_SIZE, std::memory_order_acq_rel);
        if (b >= m) { break; }
        uint64_t local_submitted = 0;
        for (uint64_t i = b; i < b + BATCH_SIZE && i < m; i++) {
          auto [o, key, x] = txs[i];
          if (o == 0) {
            if (x == 0) schd->query([key](auto t) { return T::find(t, key); });
            else schd->query([key,len=x](auto t) { return T::scan(t, key, len); }); }
          else if (o == 1) {
            schd->update(key, x);
            local_submitted += 1; } }
        n_submitted.fetch_add(local_submitted, std::memory_order_relaxed); } }); }

  for (auto& client : threads) { client.join(); }
  schd->wait_for_processing(n_submitted.load(std::memory_order_acquire));
}

template<typename T>
void run_inplace_impl(const config &cfg, uint64_t n, uint64_t m, kv* kvs, tx_context* txs) {
  timer tmr;

  tmr.start();
  auto t = T::build(cfg, n, kvs);
  log_report("init time (s): %.8lf", tmr.count());

  tmr.start();
  for (uint64_t i = 0; i < m; i++) {
    auto [o, key, x] = txs[i];
    if (o == 0) {
      if (x == 0) T::find(t, key);
      else T::scan(t, key, x); }
    else if (o == 1) { t = T::update(t, key, x); } }
  log_report("process time (s): %.8lf", tmr.count());
}

template<typename T>
void run_seqcow_impl(const config &cfg, uint64_t n, uint64_t m, kv* kvs, tx_context* txs) {
  timer tmr;

  tmr.start();
  auto t = T::build(cfg, n, kvs);
  auto schd = new conctrl::seqcow<T>(t);
  log_report("init time (s): %.8lf", tmr.count());

  tmr.start();
  multi_client_execute<T>(cfg.num_clients, m, txs, schd);
  log_report("process time (s): %.8lf", tmr.count());

  delete schd;
}

template<typename T>
void run_concow_impl(const config &cfg, uint64_t n, uint64_t m, kv* kvs, tx_context* txs) {
  timer tmr;

  tmr.start();
  auto t = T::build(cfg, n, kvs);
  auto schd = new conctrl::concow<T>(cfg.num_pipes, cfg.num_workers, t);
  log_report("init time (s): %.8lf", tmr.count());

  tmr.start();
  multi_client_execute<T>(cfg.num_clients, m, txs, schd);
  log_report("process time (s): %.8lf", tmr.count());

  delete schd;
}

template<uint P>
void run_concow_btree_p(const config &cfg, uint64_t n, uint64_t m, kv* kvs, tx_context* txs) {
  timer tmr;
  tmr.start();
  auto t = btree::interface::build(cfg, n, kvs);
  auto schd = new conctrl::concow_cyclic<btree::interface, P>(cfg.num_workers, t);
  log_report("init time (s): %.8lf", tmr.count());

  tmr.start();
  multi_client_execute<btree::interface>(cfg.num_clients, m, txs, schd);
  log_report("process time (s): %.8lf", tmr.count());

  delete schd;
}

void run_concow_btree(const config &cfg, uint64_t n, uint64_t m, kv* kvs, tx_context* txs) {
  switch (cfg.num_pipes) {
    case 1: run_concow_btree_p<1>(cfg, n, m, kvs, txs); break;
    case 2: run_concow_btree_p<2>(cfg, n, m, kvs, txs); break;
    case 3: run_concow_btree_p<3>(cfg, n, m, kvs, txs); break;
    case 4: run_concow_btree_p<4>(cfg, n, m, kvs, txs); break;
    case 5: run_concow_btree_p<5>(cfg, n, m, kvs, txs); break;
    default: abort(); break; }
}

template <typename Args>
static inline void run_inplace(size_t type, const Args& args) {
  switch (type) {
    case ST_BETREE: std::apply(run_inplace_impl<betree::interface>, args); break;
    case ST_BTREE: std::apply(run_inplace_impl<btree::interface>, args); break;
    case ST_AERT: std::apply(run_inplace_impl<aert::interface>, args); break;
    case ST_ART: std::apply(run_inplace_impl<art::interface>, args); break;
    default: break; }
}

template <typename Args>
static inline void run_seqcow(size_t type, const Args& args) {
  switch (type) {
    case ST_BETREE: std::apply(run_seqcow_impl<betree::interface>, args); break;
    case ST_BTREE: std::apply(run_seqcow_impl<btree::interface>, args); break;
    case ST_AERT: std::apply(run_seqcow_impl<aert::interface>, args); break;
    case ST_ART: std::apply(run_seqcow_impl<art::interface>, args); break;
    default: break; }
}

template <typename Args>
static inline void run_concow(size_t type, const Args& args) {
  switch (type) {
    case ST_BETREE: std::apply(run_concow_impl<betree::interface>, args); break;
    case ST_BTREE: std::apply(run_concow_btree, args); break;
    case ST_AERT: std::apply(run_concow_impl<aert::interface>, args); break;
    case ST_ART: std::apply(run_concow_impl<art::interface>, args); break;
    default: break; }
}

void run(const config &cfg, uint64_t n, uint64_t m, uint64_t* elems, tx_context* txs) {
  kv* kvs = new kv[n];
  for (uint64_t i = 0; i < n; i++) { kvs[i] = std::make_pair(elems[i], elems[i]); }

  auto args = std::make_tuple(std::ref(cfg), n, m, kvs, txs);

  switch (cfg.scheduler) {
    case SC_INPLACE: run_inplace(cfg.structure, args); break;
    case SC_SEQCOW: run_seqcow(cfg.structure, args); break;
    case SC_CONCOW: run_concow(cfg.structure, args); break;
    default: break; }

  delete[] kvs;
}

int main(int argc, char* argv[]) {
  config cfg = parse_args(argc, argv);
  print_cfg(cfg);

  std::ifstream fs(cfg.dataset, std::ios::binary);
  size_t n, m;
  fs.read((char*)&n, sizeof(n));
  fs.read((char*)&m, sizeof(m));
  log_report("num_records: %lu", n);
  log_report("num_transactions: %lu", m);
  uint64_t* elems = new uint64_t[n];
  fs.read((char*)elems, n * sizeof(uint64_t));
  tx_context* txs = new tx_context[m];
  fs.read((char*)txs, m * sizeof(tx_context));

  run(cfg, n, m, elems, txs);

  return 0;
}
