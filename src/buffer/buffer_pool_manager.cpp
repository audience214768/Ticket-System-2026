#include "buffer/buffer_pool_manager.h"
#include "buffer/replacer.h"
#include "common/config.h"
#include "disk/disk_manager.h"
#include "page/page_guard.h"


BufferPoolManager::BufferPoolManager(size_t frame_num, vector<DiskManager *> &disk_manager)
    : frame_num_(frame_num),
      replacer_(std::make_shared<Replacer>(frame_num, 3)),
      disk_manager_(disk_manager) {
  frame_info_.reserve(frame_num);
  for (size_t i = 0; i < frame_num; i++) {
    frame_info_.push_back(std::make_shared<FrameInfo>(INVALID_PAGE_ID));
  }
}

auto BufferPoolManager::NewPage(size_t file_id) -> page_id_t { return disk_manager_[file_id]->NewPage(); }


auto BufferPoolManager::WritePage(page_id_t page_id) -> WritePageGuard {
  size_t file_id = page_id >> 10;
  frame_id_t frame_id = INVALID_FRAME_ID;
  frame_id_t free_frame = INVALID_FRAME_ID;
  for(int i = 0; i < frame_num_; i++) {
    if(frame_info_[i]->page_id_ == page_id) {
      frame_id = i;
    }
    if(frame_info_[i]->page_id_ == INVALID_PAGE_ID) {
      free_frame = i;
    }
  }
  if(frame_id == INVALID_FRAME_ID) {
    if(free_frame == INVALID_FRAME_ID) {
      auto evict_frame_id = replacer_->Evict();
      Evict(evict_frame_id, page_id);
      return WritePageGuard(frame_info_[evict_frame_id]);
    }
    frame_info_[free_frame]->page_id_ = page_id;
    frame_info_[free_frame]->is_dirty_ = true;
    disk_manager_[page_id >> 10]->ReadPage(page_id, frame_info_[free_frame]->GetDataMut());
    replacer_->Access(free_frame);
    return WritePageGuard(frame_info_[free_frame]);
  }
  frame_info_[frame_id]->is_dirty_ = true;
  replacer_->Access(frame_id);
  return WritePageGuard(frame_info_[free_frame]);
}

auto BufferPoolManager::ReadPage(page_id_t page_id) -> ReadPageGuard {
  size_t file_id = page_id >> 10;
  frame_id_t frame_id = INVALID_FRAME_ID;
  frame_id_t free_frame = INVALID_FRAME_ID;
  for(int i = 0; i < frame_num_; i++) {
    if(frame_info_[i]->page_id_ == page_id) {
      frame_id = i;
    }
    if(frame_info_[i]->page_id_ == INVALID_PAGE_ID) {
      free_frame = i;
    }
  }
  if(frame_id == INVALID_FRAME_ID) {
    if(free_frame == INVALID_FRAME_ID) {
      auto evict_frame_id = replacer_->Evict();
      Evict(evict_frame_id, page_id);
      return ReadPageGuard(frame_info_[evict_frame_id]);
    }
    frame_info_[free_frame]->page_id_ = page_id;
    frame_info_[free_frame]->is_dirty_ = false;
    disk_manager_[page_id >> 10]->ReadPage(page_id, frame_info_[free_frame]->GetDataMut());
    replacer_->Access(free_frame);
    return ReadPageGuard(frame_info_[free_frame]);
  }
  replacer_->Access(frame_id);
  return ReadPageGuard(frame_info_[free_frame]);
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



