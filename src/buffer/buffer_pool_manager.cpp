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
  for (int i = 0; i < frame_num; i++) {
    frame_info_.push_back(make_shared<FrameInfo>(INVALID_PAGE_ID, i));
  }
}

auto BufferPoolManager::NewPage(size_t file_id) -> page_id_t { 
  return disk_manager_[file_id]->NewPage(); 
}


auto BufferPoolManager::WritePage(page_id_t page_id) -> WritePageGuard {
  frame_id_t frame_id = INVALID_FRAME_ID;
  frame_id_t free_frame = INVALID_FRAME_ID;
  for(int i = 0; i < frame_num_; i++) {
    if(frame_info_[i]->page_id_ == page_id) {
      frame_id = i;
      break;
    }
    if(frame_info_[i]->page_id_ == INVALID_PAGE_ID) {
      free_frame = i;
    }
  }
  if(frame_id == INVALID_FRAME_ID) {
    if(free_frame == INVALID_FRAME_ID) {
      auto evict_frame_id = replacer_->Evict();
      Evict(evict_frame_id, page_id);
      frame_info_[evict_frame_id]->is_dirty_ = true;
      Access(evict_frame_id);
      return WritePageGuard(frame_info_[evict_frame_id], replacer_);
    }
    frame_info_[free_frame]->page_id_ = page_id;
    frame_info_[free_frame]->is_dirty_ = true;
    disk_manager_[page_id >> 10]->ReadPage(page_id, frame_info_[free_frame]->GetDataMut());
    Access(free_frame);
    return WritePageGuard(frame_info_[free_frame], replacer_);
  }
  frame_info_[frame_id]->is_dirty_ = true;
  Access(frame_id);
  return WritePageGuard(frame_info_[frame_id], replacer_);
}

auto BufferPoolManager::ReadPage(page_id_t page_id) -> ReadPageGuard {
  frame_id_t frame_id = INVALID_FRAME_ID;
  frame_id_t free_frame = INVALID_FRAME_ID;
  for(int i = 0; i < frame_num_; i++) {
    if(frame_info_[i]->page_id_ == page_id) {
      frame_id = i;
      break;
    }
    if(frame_info_[i]->page_id_ == INVALID_PAGE_ID) {
      free_frame = i;
    }
  }
  if(frame_id == INVALID_FRAME_ID) {
    if(free_frame == INVALID_FRAME_ID) {
      auto evict_frame_id = replacer_->Evict();
      Evict(evict_frame_id, page_id);
      Access(evict_frame_id);
      return ReadPageGuard(frame_info_[evict_frame_id], replacer_);
    }
    frame_info_[free_frame]->page_id_ = page_id;
    frame_info_[free_frame]->is_dirty_ = false;
    disk_manager_[page_id >> 10]->ReadPage(page_id, frame_info_[free_frame]->GetDataMut());
    Access(free_frame);
    return ReadPageGuard(frame_info_[free_frame], replacer_);
  }
  Access(frame_id);
  return ReadPageGuard(frame_info_[frame_id], replacer_);
}

void BufferPoolManager::DeletePage(page_id_t page_id) {
  frame_id_t frame_id = INVALID_FRAME_ID;
  frame_id_t free_frame = INVALID_FRAME_ID;
  for(int i = 0; i < frame_num_; i++) {
    if(frame_info_[i]->page_id_ == page_id) {
      frame_id = i;
    }
  }
  if(frame_id == INVALID_FRAME_ID) {
    return ;
  }
  disk_manager_[page_id >> 10]->DeletePage(page_id);
  frame_info_[frame_id]->Reset();
}

void BufferPoolManager::Evict(frame_id_t frame_id, page_id_t page_id) {
  if (frame_info_[frame_id]->is_dirty_) {
    disk_manager_[frame_info_[frame_id]->page_id_ >> 10]->WritePage(frame_info_[frame_id]->page_id_, frame_info_[frame_id]->GetData());
  }
  disk_manager_[page_id >> 10]->ReadPage(page_id, frame_info_[frame_id]->GetDataMut());
  frame_info_[frame_id]->page_id_ = page_id;
  frame_info_[frame_id]->is_dirty_ = false;
}

void BufferPoolManager::Access(frame_id_t frame_id) {
  replacer_->Access(frame_id);
  frame_info_[frame_id]->pin_count_++;
  replacer_->SetEvictable(frame_id, false);
}

BufferPoolManager::~BufferPoolManager() {
  for (int i = 0; i < frame_num_; i++) {
    if(frame_info_[i]->page_id_ != INVALID_PAGE_ID && frame_info_[i]->is_dirty_) {
      disk_manager_[frame_info_[i]->page_id_ >> 10]->WritePage(frame_info_[i]->page_id_, frame_info_[i]->GetData());
    }
  }
}


