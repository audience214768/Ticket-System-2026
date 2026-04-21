#pragma once

#include "buffer/replacer.h"
#include "common/config.h"
#include "disk/disk_manager.h"
#include "page/page_guard.h"
#include <memory>
#include <vector>

using std::vector;
using std::shared_ptr;

class BufferPoolManager;

class FrameInfo {
  friend BufferPoolManager;
  friend ReadPageGuard;
  friend WritePageGuard;
 private:
  bool is_dirty_ = false;
  page_id_t page_id_; 
  vector<char> data_;
  auto GetData() -> const char * {return data_.data(); }
  auto GetDataMut() -> char * { return data_.data(); }
  void Reset() {is_dirty_ = false;}
 public:
  FrameInfo(page_id_t page_id) : page_id_(page_id), data_(DISK_PAGE_SIZE, 0) {} 
  ~FrameInfo() = default;
};

class BufferPoolManager {
 private:
  vector<shared_ptr<FrameInfo>> frame_info_;
  std::shared_ptr<Replacer> replacer_;
  size_t frame_num_;
  vector<DiskManager *> disk_manager_;
  void Evict(frame_id_t frame_id, page_id_t page_id);
 public:
  BufferPoolManager(size_t frame_num, vector<DiskManager *> &disk_manager);
  ~BufferPoolManager() = default;
  auto WritePage(page_id_t page_id) -> WritePageGuard;
  auto ReadPage(page_id_t page_id) -> ReadPageGuard;
  auto NewPage(size_t file_id) -> page_id_t;
  void DeletePage(page_id_t page_id);
};