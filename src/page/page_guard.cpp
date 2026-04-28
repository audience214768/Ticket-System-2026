

#include "page/page_guard.h"
#include "buffer/buffer_pool_manager.h"
#include "common/config.h"

ReadPageGuard::ReadPageGuard(shared_ptr<FrameInfo> frame, shared_ptr<Replacer> replacer/*, shared_ptr<std::mutex> bpm_latch, shared_ptr<DiskScheduler> disk_scheduler*/)
    : frame_(std::move(frame)),
      replacer_(replacer),
      is_valid_(true) {}

ReadPageGuard::ReadPageGuard(ReadPageGuard &&that) noexcept {
  is_valid_ = that.is_valid_;
  frame_ = that.frame_;
  replacer_ = that.replacer_;
  /*
  bpm_latch_ = that.bpm_latch_;
  disk_scheduler_ = that.disk_scheduler_;
  lock_ = std::move(that.lock_);*/
  that.is_valid_ = false;
}

auto ReadPageGuard::operator=(ReadPageGuard &&that) noexcept -> ReadPageGuard & {
  if (this == &that) {
    return *this;
  }
  this->Drop();
  is_valid_ = that.is_valid_;
  frame_ = that.frame_;
  replacer_ = that.replacer_;
  /*
  bpm_latch_ = that.bpm_latch_;
  disk_scheduler_ = that.disk_scheduler_;
  lock_ = std::move(that.lock_);*/
  that.is_valid_ = false;
  return *this;
}

auto ReadPageGuard::GetPageId() const -> page_id_t {
  return frame_->page_id_;
}

/**
 * @brief Gets a `const` pointer to the page of data this guard is protecting.
 */
auto ReadPageGuard::GetData() const -> const char * {
  //std::cerr << frame_->GetData() << std::endl;
  return frame_->GetData();
}

void ReadPageGuard::Drop() {
  if (!is_valid_) {
    return;
  }
  frame_->pin_count_--;
  //assert(frame_->pin_count_ >= 0);
  /*if (lock_.owns_lock()) {
    lock_.unlock();
  }
  std::lock_guard<std::mutex> lock(*bpm_latch_);*/
  if (frame_->pin_count_ == 0) {
    replacer_->SetEvictable(frame_->frame_id_, true);
  }
  is_valid_ = false;
  //page_id_ = INVALID_PAGE_ID;
  frame_ = nullptr;
  replacer_ = nullptr;
  /*
  bpm_latch_ = nullptr;
  disk_scheduler_ = nullptr;*/
}

ReadPageGuard::~ReadPageGuard() { Drop(); }

WritePageGuard::WritePageGuard(shared_ptr<FrameInfo> frame, shared_ptr<Replacer> replacer)
    :frame_(std::move(frame)), 
    replacer_(replacer),
    is_valid_(true) {}


WritePageGuard::WritePageGuard(WritePageGuard &&that) noexcept {
  is_valid_ = that.is_valid_;
  //page_id_ = that.page_id_;
  frame_ = that.frame_;
  replacer_ = that.replacer_;
  /*
  bpm_latch_ = that.bpm_latch_;
  disk_scheduler_ = that.disk_scheduler_;
  lock_ = std::move(that.lock_);*/
  that.is_valid_ = false;
}

auto WritePageGuard::operator=(WritePageGuard &&that) noexcept -> WritePageGuard & {
  if (this == &that) {
    return *this;
  }
  this->Drop();
  is_valid_ = that.is_valid_;
  //page_id_ = that.page_id_;
  frame_ = that.frame_;
  replacer_ = that.replacer_;
  /*
  bpm_latch_ = that.bpm_latch_;
  disk_scheduler_ = that.disk_scheduler_;
  lock_ = std::move(that.lock_);*/
  that.is_valid_ = false;
  return *this;
}


auto WritePageGuard::GetPageId() const -> page_id_t {
  return frame_->page_id_;
}

auto WritePageGuard::GetData() const -> const char * {
  return frame_->GetData();
}

auto WritePageGuard::GetDataMut() -> char * {
  return frame_->GetDataMut();
}


void WritePageGuard::Drop() {
  if (!is_valid_) {
    return;
  }
  frame_->pin_count_--;
  //assert(frame_->pin_count_ >= 0);
  /*if (lock_.owns_lock()) {
    // std::cerr << "unlock" << std::endl;
    lock_.unlock();
  }
  std::lock_guard<std::mutex> lock(*bpm_latch_);*/
  if (frame_->pin_count_ == 0) {
    replacer_->SetEvictable(frame_->frame_id_, true);
  }
  is_valid_ = false;
  //page_id_ = INVALID_PAGE_ID;
  frame_ = nullptr;
  replacer_ = nullptr;
  /*
  bpm_latch_ = nullptr;
  disk_scheduler_ = nullptr;*/
}

/** @brief The destructor for `WritePageGuard`. This destructor simply calls `Drop()`. */
WritePageGuard::~WritePageGuard() { Drop(); }

