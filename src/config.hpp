#pragma once

#include <iostream>
#include <string>

#include "utils/log.h"

enum SCHEDULER_TYPE {
  SC_NONE, SC_INPLACE, SC_SEQCOW, SC_CONCOW
};

constexpr const char* SCHEDULER_TYPE_STR[] = {
  "", "inplace", "seqcow", "concow" };

enum STRUCTURE_TYPE {
  ST_NONE, ST_BETREE, ST_BTREE, ST_AERT, ST_ART
};

constexpr const char* STRUCTURE_TYPE_STR[] = {
  "", "betree", "btree", "aert", "art"
};

static inline constexpr size_t N_SCHEDULER_TYPE = sizeof(SCHEDULER_TYPE_STR)/sizeof(char*);
static inline constexpr size_t N_STRUCTURE_TYPE = sizeof(STRUCTURE_TYPE_STR)/sizeof(char*);

struct config {
  std::string dataset = "";
  size_t structure = 0;
  size_t scheduler = 0;
  size_t num_clients = 1;
  size_t num_pipes = 1;
  size_t num_workers = 1;
};

void exit_with_usage(const char *prog_name) {
  std::cerr << "Usage: ./" << prog_name << " "
            << "<dataset> <scheduler> <structure> "
            << "[-c <num_clients>] [-p <num_pipes>] [-w <num_workers>]\n"
            << "Structures: betree, btree, aert, art\n"
            << "Schedulers: inplace, seqcow, concow"
            << std::endl;
  exit(EXIT_FAILURE);
}

config parse_args(int argc, char* argv[]) {
  if (argc < 4) { exit_with_usage(argv[0]); }
  config cfg;
  cfg.dataset = argv[1];
  std::string str;

  str = argv[2];
  for (size_t i = 1; i < N_SCHEDULER_TYPE; i++) {
    if (str == SCHEDULER_TYPE_STR[i]) {
      cfg.scheduler = i;
      break; } }
  if (cfg.scheduler == SC_NONE) {
    std::cerr << "invalid scheduler: " << str << "\n";
    exit_with_usage(argv[0]); }

  str = argv[3];
  for (size_t i = 1; i < N_STRUCTURE_TYPE; i++) {
    if (str == STRUCTURE_TYPE_STR[i]) {
      cfg.structure = i;
      break; } }
  if (cfg.structure == ST_NONE) {
    std::cerr << "invalid structure: " << str << "\n";
    exit_with_usage(argv[0]); }

  for (int i = 4; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-c") {
      if (++i >= argc) { exit_with_usage(argv[0]); }
      cfg.num_clients = std::stoull(argv[i]);
      if (cfg.num_clients == 0) { cfg.num_clients = 1; } }
    else if (arg == "-p") {
      if (++i >= argc) { exit_with_usage(argv[0]); }
      cfg.num_pipes = std::stoull(argv[i]);
      if (cfg.num_pipes == 0) { cfg.num_pipes = 1; } }
    else if (arg == "-w") {
      if (++i >= argc) { exit_with_usage(argv[0]); }
      cfg.num_workers = std::stoull(argv[i]);
      if (cfg.num_workers == 0) { cfg.num_workers = 1; } }
    else {
      log_fatal("unknown option '%s'", argv[i]);
      exit_with_usage(argv[0]); } }

  return cfg;
}

void print_cfg(const config &cfg) {
  // generate code to print config to console
  log_report("dataset: %s", cfg.dataset.c_str());
  log_report("scheduler: %s", SCHEDULER_TYPE_STR[cfg.scheduler]);
  log_report("structure: %s", STRUCTURE_TYPE_STR[cfg.structure]);
  log_report("num_clients: %lu", cfg.num_clients);
  log_report("num_pipes: %lu", cfg.num_pipes);
  log_report("num_workers: %lu", cfg.num_workers);
}
