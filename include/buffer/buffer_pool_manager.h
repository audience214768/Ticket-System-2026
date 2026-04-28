#pragma once

#include "replacer.h"
#include "common/config.h"
#include "disk/disk_manager.h"
#include "page/page_guard.h"
#include "vector/vector.hpp"
#include "shared_ptr/shared_ptr.hpp"

using sjtu::vector;
using sjtu::shared_ptr;
using sjtu::make_shared;

class BufferPoolManager;

class FrameInfo {
  friend BufferPoolManager;
  friend ReadPageGuard;
  friend WritePageGuard;
 private:
  bool is_dirty_ = false;
  page_id_t page_id_; 
  frame_id_t frame_id_;
  size_t pin_count_ = 0;
  vector<char> data_;
  auto GetData() -> const char * {
    return data_.data(); 
  }
  auto GetDataMut() -> char * { return data_.data(); }
  void Reset() {
    is_dirty_ = false;
    page_id_ = INVALID_PAGE_ID;
  }
 public:
  FrameInfo(page_id_t page_id, frame_id_t frame_id) : page_id_(page_id), frame_id_(frame_id), data_(DISK_PAGE_SIZE, 0) {} 
  ~FrameInfo() = default;
};

class BufferPoolManager {
 private:
  vector<shared_ptr<FrameInfo>> frame_info_;
  shared_ptr<Replacer> replacer_;
  size_t frame_num_;
  vector<shared_ptr<DiskManager>> disk_manager_;
  void Evict(frame_id_t frame_id, page_id_t page_id);
  void Access(frame_id_t frame_id);
 public:
  BufferPoolManager(size_t frame_num, vector<shared_ptr<DiskManager>> &disk_manager);
  ~BufferPoolManager();
  auto WritePage(page_id_t page_id) -> WritePageGuard;
  auto ReadPage(page_id_t page_id) -> ReadPageGuard;
  auto NewPage(size_t file_id) -> page_id_t;
  void DeletePage(page_id_t page_id);
};