
#pragma once

#include <memory>
#include "common/config.h"

using std::shared_ptr;

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
  //auto IsDirty() const -> bool;
  //void Flush();
  void Drop();
  ~ReadPageGuard();

 private:
  explicit ReadPageGuard(shared_ptr<FrameInfo> frame);
  shared_ptr<FrameInfo> frame_;
  //std::shared_ptr<std::mutex> bpm_latch_;
  bool is_valid_{false};
  //std::shared_lock<std::shared_mutex> lock_;
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
  //auto IsDirty() const -> bool;
  //void Flush();
  void Drop();
  ~WritePageGuard();

 private:
  /** @brief Only the buffer pool manager is allowed to construct a valid `WritePageGuard.` */
  explicit WritePageGuard(shared_ptr<FrameInfo> frame);
  shared_ptr<FrameInfo> frame_;
  //std::shared_ptr<std::mutex> bpm_latch_;
  bool is_valid_{false};
  //std::unique_lock<std::shared_mutex> lock_;
};

