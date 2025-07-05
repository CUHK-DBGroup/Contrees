#pragma once

#include <chrono>
#include <cstdio>

struct timer {
public:
  void start() {
    t_ = clock::now();
  }

  double count() {
    clock::time_point t = clock::now();
    duration span = std::chrono::duration_cast<duration>(t - t_);
    return span.count();
  }

  void report(FILE* log, const char* msg = nullptr) {
    if (msg != nullptr) { fprintf(log, "[%s] time used (s)\n", msg); }
    else { fprintf(log, "# time used (s)\n"); }
    fprintf(log, "%.8f\n", count());
  }

  void report(const char* msg = nullptr) {
    report(stderr, msg);
  }

private:
  using clock = std::chrono::high_resolution_clock;
  using duration = std::chrono::duration<double>;

  clock::time_point t_;
};
