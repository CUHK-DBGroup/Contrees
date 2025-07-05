#pragma once

#include <atomic>

namespace conctrl {

template <typename T>
class alignas(128) mpsc_list {
private:
  T stub_;
  alignas(128) std::atomic<T*> head_;
  alignas(128) T* volatile tail_;

public:
  mpsc_list() : stub_(), head_(&stub_), tail_(&stub_) { }

  void push(T* item) {
    item->next = nullptr;
    T* prev = head_.exchange(item, std::memory_order_acq_rel);
    prev->next = item;
  }

  T* pop() {
    T* tail = tail_;
    T* next = tail->next;

    if (tail == &stub_) {
      if (next == nullptr) { return nullptr; }
      tail_ = next;
      tail = next;
      next = next->next; }
    if (next) {
      tail_ = next;
      return tail; }

    T* head = head_.load(std::memory_order_acquire);
    if (tail != head) { return nullptr; }
    push(&stub_);
    next = tail->next;
    if (next) {
      tail_ = next;
      return tail; }
    return nullptr;
  }
};

}
