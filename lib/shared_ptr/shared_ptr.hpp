#pragma once

#include <iostream>


namespace sjtu {

template <typename T> 
class shared_ptr {
 public:
  shared_ptr() : ptr(nullptr), refCount(nullptr) {}
  shared_ptr(T *p) : ptr(p), refCount(new size_t(1)) {}
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

  ~shared_ptr() { release(); }

  T *get() const { return ptr; }

  T &operator*() const { return *ptr; }

  T *operator->() const { return ptr; }

 private: 
  T *ptr;
  size_t *refCount;
  void release() {
    if (ptr == nullptr) {
      return;
    }
    if (--(*refCount) == 0) {
      delete ptr;
      delete refCount;
    }
  }
  void swap(shared_ptr &other) {
    std::swap(ptr, other.ptr);
    std::swap(refCount, other.refCount);
  }
};

template<typename T, typename... Args>
auto make_shared(Args&&... args) -> shared_ptr<T> {
  return shared_ptr<T>(new T(std::forward<Args>(args)...));
}
} // namespace sjtu