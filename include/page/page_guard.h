
#pragma once

#include "common/config.h"
#include "buffer/replacer.h"
#include "shared_ptr/shared_ptr.hpp"
#include <mutex>
#include <shared_mutex>

using sjtu::shared_ptr;
using sjtu::make_shared;

using std::unique_lock;
using std::shared_lock;
using std::mutex;
using std::shared_mutex;

class BufferPoolManager;
class FrameInfo;

class ReadPageGuard {
  friend class BufferPoolManager;

 public:
  ReadPageGuard() = default;
  ReadPageGuard(const ReadPageGuard &) = delete;
  auto operator=(const ReadPageGuard &) -> ReadPageGuard & = delete;
  ReadPageGuard(ReadPageGuard &&that) noexcept;
  auto operator=(ReadPageGuard &&that) noexcept -> ReadPageGuard &;
  auto GetPageId() const -> page_id_t;
  auto GetData() const -> const char *;
  template <class T>
  auto As() const -> const T * {
    return reinterpret_cast<const T *>(GetData());
  }
  void Drop();
  ~ReadPageGuard();

 private:
  explicit ReadPageGuard(shared_ptr<FrameInfo> frame, shared_ptr<Replacer> replacer, shared_ptr<mutex> bpm_latch);
  shared_ptr<FrameInfo> frame_;
  shared_ptr<Replacer> replacer_;
  shared_ptr<mutex> bpm_latch_;
  bool is_valid_ = false;
  shared_lock<shared_mutex> lock_;
};

class WritePageGuard {
  friend class BufferPoolManager;

 public:
  WritePageGuard() = default;

  WritePageGuard(const WritePageGuard &) = delete;
  auto operator=(const WritePageGuard &) -> WritePageGuard & = delete;
  WritePageGuard(WritePageGuard &&that) noexcept;
  auto operator=(WritePageGuard &&that) noexcept -> WritePageGuard &;
  auto GetPageId() const -> page_id_t;
  auto GetData() const -> const char *;
  template <class T>
  auto As() const -> const T * {
    return reinterpret_cast<const T *>(GetData());
  }
  auto GetDataMut() -> char *;
  template <class T>
  auto AsMut() -> T * {
    return reinterpret_cast<T *>(GetDataMut());
  }
  void Drop();
  ~WritePageGuard();

 private:
  /** @brief Only the buffer pool manager is allowed to construct a valid `WritePageGuard.` */
  explicit WritePageGuard(shared_ptr<FrameInfo> frame ,shared_ptr<Replacer> replacer, shared_ptr<mutex> bpm_latch);
  shared_ptr<FrameInfo> frame_;
  shared_ptr<Replacer> replacer_;
  shared_ptr<mutex> bpm_latch_;
  bool is_valid_ = false;
  unique_lock<shared_mutex> lock_;
};

