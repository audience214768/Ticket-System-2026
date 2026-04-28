#include "buffer/buffer_pool_manager.h"
#include "buffer/replacer.h"
#include "common/config.h"
#include "disk/disk_manager.h"
#include "page/page_guard.h"

using sjtu::make_shared;

BufferPoolManager::BufferPoolManager(size_t frame_num, vector<shared_ptr<DiskManager>> &disk_manager)
  : frame_num_(frame_num),
    replacer_(make_shared<Replacer>(frame_num, 3)),
    disk_manager_(disk_manager) {
  frame_info_.reserve(frame_num);
  free_list_.reserve(frame_num);
  for (int i = 0; i < frame_num; i++) {
    frame_info_.push_back(make_shared<FrameInfo>(INVALID_PAGE_ID, i));
    free_list_.push_back(i);
  }
}

auto BufferPoolManager::NewPage(size_t file_id) -> page_id_t { 
  return disk_manager_[file_id]->NewPage(); 
}

inline auto hash(page_id_t page_id) -> int {
    return page_id & (HASH_SIZE - 1);
}

auto BufferPoolManager::FindFrame(page_id_t page_id) -> frame_id_t {
  int idx = hash(page_id);
  int tmp = idx;
  while (hash_table[idx].used) {
    if (hash_table[idx].page_id == page_id) {
      return hash_table[idx].frame_id;
    }
    idx = (idx + 1) & (HASH_SIZE - 1);
    if (idx == tmp) {
      break;
    }
  }
  return INVALID_FRAME_ID;
}

void BufferPoolManager::UseFrame(frame_id_t frame_id, page_id_t page_id, bool is_write) {
  int idx = hash(page_id);
  int tmp = idx;
  while (hash_table[idx].page_id != INVALID_PAGE_ID) {
    idx = (idx + 1) & (HASH_SIZE - 1);
  }
  hash_table[idx].page_id = page_id;
  hash_table[idx].frame_id = frame_id;
  hash_table[idx].used = true;
  frame_info_[frame_id]->page_id_ = page_id;
  frame_info_[frame_id]->is_dirty_ = is_write;
  disk_manager_[page_id >> FILE_BIT]->ReadPage(page_id, frame_info_[frame_id]->GetDataMut());
  Access(frame_id);
}

auto BufferPoolManager::WritePage(page_id_t page_id) -> WritePageGuard {
  frame_id_t frame_id = FindFrame(page_id);
  if(frame_id == INVALID_FRAME_ID) {
    if(free_list_.empty()) {
      auto evict_frame_id = replacer_->Evict();
      Evict(evict_frame_id, page_id);
      frame_info_[evict_frame_id]->is_dirty_ = true;
      return WritePageGuard(frame_info_[evict_frame_id], replacer_);
    }
    auto free_frame = free_list_.back();
    free_list_.pop_back();
    UseFrame(free_frame, page_id, true);
    return WritePageGuard(frame_info_[free_frame], replacer_);
  }
  frame_info_[frame_id]->is_dirty_ = true;
  Access(frame_id);
  return WritePageGuard(frame_info_[frame_id], replacer_);
}

auto BufferPoolManager::ReadPage(page_id_t page_id) -> ReadPageGuard {
  frame_id_t frame_id = FindFrame(page_id);
  if(frame_id == INVALID_FRAME_ID) {
    if(free_list_.empty()) {
      auto evict_frame_id = replacer_->Evict();
      Evict(evict_frame_id, page_id);
      Access(evict_frame_id);
      return ReadPageGuard(frame_info_[evict_frame_id], replacer_);
    }
    auto free_frame = free_list_.back();
    free_list_.pop_back();
    UseFrame(free_frame, page_id, false);
    return ReadPageGuard(frame_info_[free_frame], replacer_);
  }
  Access(frame_id);
  return ReadPageGuard(frame_info_[frame_id], replacer_);
}

void BufferPoolManager::DeletePage(page_id_t page_id) {
  frame_id_t frame_id = FindFrame(page_id);
  if(frame_id == INVALID_FRAME_ID) {
    return ;
  }
  disk_manager_[page_id >> FILE_BIT]->DeletePage(page_id);
  frame_info_[frame_id]->Reset();
  free_list_.push_back(frame_id);
  auto idx = hash(page_id);
  while (hash_table[idx].used) {
    if (hash_table[idx].page_id == page_id) {
      hash_table[idx].page_id = INVALID_PAGE_ID;
      hash_table[idx].frame_id = INVALID_FRAME_ID;
      break;
    }
    idx = (idx + 1) & (HASH_SIZE - 1);
  }
}

void BufferPoolManager::Evict(frame_id_t frame_id, page_id_t page_id) {
  if (frame_info_[frame_id]->is_dirty_) {
    disk_manager_[frame_info_[frame_id]->page_id_ >> FILE_BIT]->WritePage(frame_info_[frame_id]->page_id_, frame_info_[frame_id]->GetData());
  }
  auto idx = hash(frame_info_[frame_id]->page_id_);
  while (hash_table[idx].used) {
    if (hash_table[idx].page_id == frame_info_[frame_id]->page_id_) {
      hash_table[idx].page_id = INVALID_PAGE_ID;
      hash_table[idx].frame_id = INVALID_FRAME_ID;
      break;
    }
    idx = (idx + 1) & (HASH_SIZE - 1);
  }
  UseFrame(frame_id, page_id, false);
}

void BufferPoolManager::Access(frame_id_t frame_id) {
  replacer_->Access(frame_id);
  frame_info_[frame_id]->pin_count_++;
  replacer_->SetEvictable(frame_id, false);
}

BufferPoolManager::~BufferPoolManager() {
  for (int i = 0; i < frame_num_; i++) {
    if(frame_info_[i]->page_id_ != INVALID_PAGE_ID && frame_info_[i]->is_dirty_) {
      disk_manager_[frame_info_[i]->page_id_ >> FILE_BIT]->WritePage(frame_info_[i]->page_id_, frame_info_[i]->GetData());
    }
  }
}


