//
//  basic_db.h
//  YCSB-C
//
//  Created by Jinglei Ren on 12/17/14.
//  Copyright (c) 2014 Jinglei Ren <jinglei@ren.systems>.
//

#ifndef YCSB_C_BASIC_DB_H_
#define YCSB_C_BASIC_DB_H_

#include "core/db.h"
#include "core/properties.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>

namespace ycsbc {

class Export : public DB {
private:
  std::mutex mutex_;
  std::ofstream fs_;
  bool running_;
  bool inited_;
  size_t num_records_;
  size_t num_transactions_;

  uint64_t n_query_;
  uint64_t n_insert_;
  uint64_t n_update_;
  uint64_t n_scan_;

  static inline constexpr uint8_t OP_QUERY  = 0;
  static inline constexpr uint8_t OP_INSERT = 1;
  static inline constexpr uint8_t OP_DELETE = 2;

  struct tx_context {
    uint8_t type;
    uint64_t arg0;
    uint64_t arg1;
  };

  uint64_t str2key_(const std::string &str) const {
    char *p;
    return std::strtoull(str.c_str()+4, &p, 10);
  }

  int export_insert_(uint64_t key) {
    tx_context tx{OP_INSERT, key, key};
    fs_.write((char*)&tx, sizeof(tx_context));
    return 0;
  }

  int export_delete_(uint64_t key) {
    tx_context tx{OP_DELETE, key, key};
    fs_.write((char*)&tx, sizeof(tx_context));
    return 0;
  }

  int export_query_(uint64_t l, uint64_t n) {
    tx_context tx{OP_QUERY, l, n};
#ifndef NDEBUG
    std::cerr << "Query "<< l << " " << n << std::endl;
#endif
    fs_.write((char*)&tx, sizeof(tx_context));
    return 0;
  }

public:
  Export(std::string filename, size_t num_records, size_t num_transactions) :
    fs_(filename, std::ios_base::binary), running_(false), inited_(false),
    num_records_(num_records), num_transactions_(num_transactions)
  { }

  ~Export() {
    std::cout << "n_query: " << n_query_ << std::endl;
    std::cout << "n_insert: " << n_insert_ << std::endl;
    std::cout << "n_update: " << n_update_ << std::endl;
    std::cout << "n_scan: " << n_scan_ << std::endl;
    fs_.close();
  }

  void Init() override {
    std::lock_guard<std::mutex> lock(mutex_);
    if (running_) {
      std::cerr << "concurrent access is not supported" << std::endl;
      abort(); }
    running_ = true;
    if (!inited_) {
      fs_.write((char*)&num_records_, sizeof(size_t));
      fs_.write((char*)&num_transactions_, sizeof(size_t)); }
  }

  void Close() override {
    std::lock_guard<std::mutex> lock(mutex_);
    inited_ = true;
    running_ = false;
  }

  int Read(
      const std::string &table,
      const std::string &key,
      const std::vector<std::string> *fields,
      std::vector<KVPair> &result) override {
    if (!inited_) {
      std::cerr << "transaction before initialization" << std::endl;
      abort(); }
    n_query_++;
    uint64_t k = str2key_(key);
    return export_query_(k, 0);
  }

  int Scan(
      const std::string &table,
      const std::string &key,
      int len,
      const std::vector<std::string> *fields,
      std::vector<std::vector<KVPair>> &result) override {
    if (!inited_) {
      std::cerr << "transaction before initialization" << std::endl;
      abort(); }
    n_scan_++;
    return export_query_(str2key_(key), len);
  }

  int Update(
      const std::string &table,
      const std::string &key,
      std::vector<KVPair> &values) override {
    if (!inited_) {
      std::cerr << "transaction before initialization" << std::endl;
      abort(); }
    n_update_++;
    return export_insert_(str2key_(key));
  }

  int Insert(
      const std::string &table,
      const std::string &key,
      std::vector<KVPair> &values) override {
    if (inited_) {
      n_insert_++;
      return export_insert_(str2key_(key)); }
    uint64_t k = str2key_(key);
    fs_.write((char*)&k, sizeof(uint64_t));
    return 0;
  }

  int Delete(
      const std::string &table,
      const std::string &key) override {
    if (!inited_) {
      std::cerr << "transaction before initialization" << std::endl;
      abort(); }
    return export_delete_(str2key_(key));
  }
};

} // ycsbc

#endif // YCSB_C_BASIC_DB_H_
