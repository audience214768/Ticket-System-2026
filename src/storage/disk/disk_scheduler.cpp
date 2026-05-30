#include "disk/disk_scheduler.h"
#include "common/config.h"
#include "disk/disk_manager.h"
#include <mutex>

using std::unique_lock;

DiskScheduler::DiskScheduler(
    size_t thread_num, const vector<shared_ptr<DiskManager>> &disk_manager)
    : disk_manager_(disk_manager), stop_(false) {
  for (int i = 0; i < thread_num; i++) {
    workers_.emplace_back([&] { StartFlushPage(); });
  }
}

DiskScheduler::~DiskScheduler() {
  {
    unique_lock<mutex> lock(mutex_);
    stop_ = true;
  }
  cv_consume_.notify_all();
  for (auto &worker : workers_) {
    if (worker.joinable()) {
      worker.join();
    }
  }
}

void DiskScheduler::Scheduler(DiskRequest request) {
  {
    unique_lock<mutex> lock(mutex_);
    cv_produce_.wait(lock, [this] { return channel_.size() < MAX_REQUEST_NUM; });
    channel_.emplace_back(std::move(request));
  }
  cv_consume_.notify_all();
}

void DiskScheduler::StartFlushPage() {
  while (true) {
    DiskRequest disk_request;
    {
      unique_lock<mutex> lock(mutex_);
      cv_consume_.wait(lock, [this] { return stop_ || !channel_.empty(); });
      if (stop_ && channel_.empty()) {
        return;
      }
      disk_request = std::move(channel_.front());
      channel_.pop_front();
      cv_produce_.notify_all();
    }
    switch (disk_request.type) {
      case RequestType::kDelete: {
        disk_manager_[disk_request.page_id >> FILE_BIT]->DeletePage(disk_request.page_id);
        disk_request.callback.set_value(1);
        break;
      }
      case RequestType::kRead: {
        disk_manager_[disk_request.page_id >> FILE_BIT]->ReadPage(disk_request.page_id, disk_request.data);
        disk_request.callback.set_value(1);
        break;
      }
      case RequestType::kWrite: {
        disk_manager_[disk_request.page_id >> FILE_BIT]->WritePage(disk_request.page_id, disk_request.data);
        disk_request.callback.set_value(1);
        break;
      }
      case RequestType::kNew: {
        disk_request.callback.set_value(disk_manager_[disk_request.page_id >> FILE_BIT]->NewPage());
        break;
      }
    }
  }
}