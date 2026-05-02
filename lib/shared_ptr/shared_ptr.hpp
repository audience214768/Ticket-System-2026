#pragma once

#include <iostream>
#include <atomic>

using std::atomic;


namespace sjtu {

template <typename T> 
class shared_ptr {
 private: 
  T *ptr;
  atomic<size_t> *refCount;
  void swap(shared_ptr &other) {
    std::swap(ptr, other.ptr);
    std::swap(refCount, other.refCount);
  }
 public:
  shared_ptr() : ptr(nullptr), refCount(nullptr) {}
  shared_ptr(T *p) : ptr(p) {
    if (p) {
      refCount = new atomic<size_t> (1);
    }
  }
  shared_ptr(const shared_ptr &other): ptr(other.ptr), refCount(other.refCount) {
    (*refCount)++;
  }

  shared_ptr(shared_ptr &&other) noexcept: ptr(other.ptr), refCount(other.refCount) {
    other.ptr = nullptr;
    other.refCount = nullptr;
  }

  shared_ptr &operator=(const shared_ptr &other) {
    if (this != &other) {
      shared_ptr temp(other);
      swap(temp);
    }
    return *this;
  }
  shared_ptr &operator=(shared_ptr &&other) noexcept {
    if (this != &other) {
      shared_ptr temp(std::move(other));
      swap(temp);
    }
    return *this;
  }
  void release() {
    if (ptr) {
      if (--(*refCount) == 0) {
        delete ptr;
        delete refCount;
      }
    }
    ptr = nullptr;
    refCount = nullptr;
  }

  ~shared_ptr() { release(); }

  T *get() const { return ptr; }

  T &operator*() const { return *ptr; }

  T *operator->() const { return ptr; }
};

template<typename T, typename... Args>
auto make_shared(Args&&... args) -> shared_ptr<T> {
  return shared_ptr<T>(new T(std::forward<Args>(args)...));
}
} // namespace sjtu