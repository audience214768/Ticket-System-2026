#pragma once

#include "common/config.h"
#include "disk/disk_manager.h"
#include "vector/vector.hpp"
#include "shared_ptr/shared_ptr.hpp"
#include "deque/deque.hpp"
#include <future>
#include <thread>
#include <condition_variable>
#include <mutex>

using sjtu::vector;
using sjtu::shared_ptr;
using sjtu::deque;
using std::thread;
using std::mutex;
using std::condition_variable;
using std::promise;

enum RequestType { kWrite = 0, kRead, kDelete, kNew};

struct DiskRequest {
  RequestType type;
  char *data;
  page_id_t page_id;
  promise<page_id_t> callback;
};

class DiskScheduler {
 private:
  vector<shared_ptr<DiskManager>> disk_manager_;
  deque<DiskRequest> channel_;
  vector<thread> workers_;
  mutex mutex_;
  condition_variable cv_;
  bool stop_ = true;
 public:
  DiskScheduler(size_t thread_num, const vector<shared_ptr<DiskManager>> &disk_manager);
  ~DiskScheduler();
  void Scheduler(DiskRequest request);
 private:
  void StartWorkerThread();
};