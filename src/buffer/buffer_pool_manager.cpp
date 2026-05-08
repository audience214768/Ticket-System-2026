#include "buffer/buffer_pool_manager.h"
#include "buffer/replacer.h"
#include "common/config.h"
#include "disk/disk_manager.h"
#include "disk/disk_scheduler.h"
#include "disk/log_manager.h"
#include "page/b_plus_tree_leaf_page.h"
#include "page/page_guard.h"
#include <cassert>
#include <cstddef>
#include <mutex>

using std::unique_lock;
using sjtu::make_shared;

BufferPoolManager::BufferPoolManager(size_t frame_num, const vector<shared_ptr<DiskManager>> &disk_manager)
  : frame_num_(frame_num),
    replacer_(make_shared<Replacer>(frame_num)),
    disk_scheduler_(make_shared<DiskScheduler>(1, disk_manager)),
    bpm_mutex_(make_shared<mutex>())/*,
    log_manager_(make_shared<LogManager>(disk_manager))*/ {
  frame_info_.reserve(frame_num);
  free_list_.reserve(frame_num);
  for (int i = 0; i < frame_num; i++) {
    frame_info_.push_back(make_shared<FrameInfo>(INVALID_PAGE_ID, i));
    free_list_.push_back(i);
  }
}

auto BufferPoolManager::NewPage(size_t file_id) -> page_id_t {
  promise<page_id_t> promise;
  auto future = promise.get_future();
  disk_scheduler_->Scheduler(DiskRequest{
    .type = RequestType::kNew,
    .data = nullptr,
    .page_id = file_id << FILE_BIT,
    .callback = std::move(promise),
  });
  auto new_page_id = future.get();
  unique_lock<mutex> lock(*bpm_mutex_);
  //std::cerr << new_page_id << std::endl;
  return new_page_id;
}

inline auto hash(page_id_t page_id) -> int {
    return page_id & (HASH_SIZE - 1);
}

void BufferPoolManager::InsertHash(frame_id_t frame_id, page_id_t page_id) {
  int idx = hash(page_id);
  while (hash_table[idx].page_id != INVALID_PAGE_ID) {
    idx = (idx + 1) & (HASH_SIZE - 1);
  }
  hash_table[idx].page_id = page_id;
  hash_table[idx].frame_id = frame_id;
  hash_table[idx].used = true;
}

auto BufferPoolManager::FindFrame(page_id_t page_id) -> frame_id_t {
  int idx = hash(page_id);
  int tmp = idx;
  while (hash_table[idx].used) {
    if (hash_table[idx].page_id == page_id) {
      //std::cerr << "get " << (tmp - idx) << std::endl;
      return hash_table[idx].frame_id;
    }
    idx = (idx + 1) & (HASH_SIZE - 1);
    if (idx == tmp) {
      break;
    }
  }
  //std::cerr << "fail " << idx - tmp << std::endl;
  return INVALID_FRAME_ID;
}

void BufferPoolManager::UseFrame(frame_id_t frame_id, page_id_t page_id, bool is_write, unique_lock<mutex> &lock) {
  //std::cerr << page_id << std::endl;
  InsertHash(frame_id, page_id);
  frame_info_[frame_id]->page_id_ = page_id;
  frame_info_[frame_id]->is_dirty_ = is_write;
  frame_info_[frame_id]->is_loading_ = true;
  Access(frame_id);
  {
    promise<page_id_t> promise;
    auto future = promise.get_future();
    disk_scheduler_->Scheduler(DiskRequest{
      .type = RequestType::kRead,
      .data = frame_info_[frame_id]->GetDataMut(),
      .page_id = page_id,
      .callback = std::move(promise),
    });
    lock.unlock();
    future.get();
    lock.lock();
  }
  frame_info_[frame_id]->is_loading_ = false;
  frame_info_[frame_id]->cv_.notify_all();
}

void BufferPoolManager::Evict(frame_id_t frame_id, page_id_t page_id, bool is_write, unique_lock<mutex> &lock) {
  auto idx = hash(frame_info_[frame_id]->page_id_);
  while (hash_table[idx].used) {
    if (hash_table[idx].page_id == frame_info_[frame_id]->page_id_) {
      hash_table[idx].page_id = INVALID_PAGE_ID;
      hash_table[idx].frame_id = INVALID_FRAME_ID;
      break;
    }
    idx = (idx + 1) & (HASH_SIZE - 1);
  }
  InsertHash(frame_id, page_id);
  page_id_t old_page_id = frame_info_[frame_id]->page_id_;
  bool old_state = frame_info_[frame_id]->is_dirty_;
  frame_info_[frame_id]->page_id_ = page_id;
  frame_info_[frame_id]->is_dirty_ = is_write;
  Access(frame_id);
  frame_info_[frame_id]->is_loading_ = true;
  if (old_state) {
    promise<page_id_t> promise;
    auto future = promise.get_future();
    disk_scheduler_->Scheduler(DiskRequest{
      .type = RequestType::kWrite,
      .data = frame_info_[frame_id]->GetDataMut(),
      .page_id = old_page_id,
      .callback = std::move(promise),
    });
    lock.unlock();
    future.get();
    lock.lock();
    //disk_manager_[frame_info_[frame_id]->page_id_ >> FILE_BIT]->WritePage(frame_info_[frame_id]->page_id_, frame_info_[frame_id]->GetData());
  }
  {
    promise<page_id_t> promise;
    auto future = promise.get_future();
    disk_scheduler_->Scheduler(DiskRequest{
      .type = RequestType::kRead,
      .data = frame_info_[frame_id]->GetDataMut(),
      .page_id = page_id,
      .callback = std::move(promise),
    });
    lock.unlock();
    future.get();
    lock.lock();
  }
  frame_info_[frame_id]->is_loading_ = false;
  frame_info_[frame_id]->cv_.notify_all();
}

void BufferPoolManager::Access(frame_id_t frame_id) {
  replacer_->Access(frame_id);
  frame_info_[frame_id]->pin_count_++;
  replacer_->SetEvictable(frame_id, false);
}

auto BufferPoolManager::WritePage(page_id_t page_id) -> WritePageGuard {
  unique_lock<mutex> lock(*bpm_mutex_);
  //std::cerr << "write page " << page_id << std::endl;
  frame_id_t frame_id = FindFrame(page_id);
  if(frame_id == INVALID_FRAME_ID) {
    if(free_list_.empty()) {
      auto evict_frame_id = replacer_->Evict();
      Evict(evict_frame_id, page_id, true, lock);
      //UseFrame(evict_frame_id, page_id, true, lock);
      lock.unlock();
      return WritePageGuard(frame_info_[evict_frame_id], replacer_, bpm_mutex_);
    }
    auto free_frame = free_list_.back();
    free_list_.pop_back();
    UseFrame(free_frame, page_id, true, lock);
    lock.unlock();
    return WritePageGuard(frame_info_[free_frame], replacer_, bpm_mutex_);
  }
  //std::cerr << "cache hit" << std::endl;
  if (frame_info_[frame_id]->is_loading_) {
    frame_info_[frame_id]->cv_.wait(lock, [&]{ return frame_info_[frame_id]->is_loading_ == false; });
  }
  frame_info_[frame_id]->is_dirty_ = true;
  Access(frame_id);
  lock.unlock();
  return WritePageGuard(frame_info_[frame_id], replacer_, bpm_mutex_);
}

auto BufferPoolManager::ReadPage(page_id_t page_id) -> ReadPageGuard {
  unique_lock<mutex> lock(*bpm_mutex_);
  frame_id_t frame_id = FindFrame(page_id);
  if(frame_id == INVALID_FRAME_ID) {
    if(free_list_.empty()) {
      auto evict_frame_id = replacer_->Evict();
      Evict(evict_frame_id, page_id, false, lock);
      lock.unlock();
      return ReadPageGuard(frame_info_[evict_frame_id], replacer_, bpm_mutex_);
    }
    auto free_frame = free_list_.back();
    free_list_.pop_back();
    UseFrame(free_frame, page_id, false, lock);
    //std::cerr << "2 " << frame_info_[free_frame]->page_id_ << std::endl;
    lock.unlock();
    return ReadPageGuard(frame_info_[free_frame], replacer_, bpm_mutex_);
  }
  if (frame_info_[frame_id]->is_loading_) {
    frame_info_[frame_id]->cv_.wait(lock, [&]{ return frame_info_[frame_id]->is_loading_ == false; });
  }
  Access(frame_id);
  lock.unlock();
  return ReadPageGuard(frame_info_[frame_id], replacer_, bpm_mutex_);
}

void BufferPoolManager::DeletePage(page_id_t page_id) {
  unique_lock<mutex> lock(*bpm_mutex_);
  frame_id_t frame_id = FindFrame(page_id);
  if(frame_id != INVALID_FRAME_ID) {
    assert(frame_info_[frame_id]->pin_count_ == 0);
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
  {
    promise<page_id_t> promise;
    auto future = promise.get_future();
    disk_scheduler_->Scheduler(DiskRequest{
      .type = RequestType::kDelete,
      .data = nullptr,
      .page_id = page_id,
      .callback = std::move(promise),
    });
    lock.unlock();
    future.get();
    lock.lock();
  }
}

BufferPoolManager::~BufferPoolManager() {
  for (int i = 0; i < frame_num_; i++) {
    assert(frame_info_[i]->pin_count_ == 0);
    if(frame_info_[i]->page_id_ != INVALID_PAGE_ID && frame_info_[i]->is_dirty_) {
      promise<page_id_t> promise;
      auto future = promise.get_future();
      disk_scheduler_->Scheduler(DiskRequest{
        .type = RequestType::kWrite,
        .data = frame_info_[i]->GetDataMut(),
        .page_id = frame_info_[i]->page_id_,
        .callback = std::move(promise),
      });
      future.get();
    }
  }
}


