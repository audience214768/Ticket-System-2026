#pragma once

#include "common/config.h"
#include "shared_ptr/shared_ptr.hpp"
#include "disk/disk_manager.h"
#include "vector/vector.hpp"
#include "deque/deque.hpp"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <thread>

using sjtu::shared_ptr;
using sjtu::vector;
using sjtu::deque;

using std::atomic;
using std::thread;
using std::condition_variable;
using std::mutex;
using std::fstream;
using std::ios;


class LogManager {
 static const size_t LOG_SIZE = DISK_PAGE_SIZE + sizeof(size_t) * 2;
 static const size_t BUFFER_SIZE = LOG_SIZE * 10;
 private:
  vector<shared_ptr<DiskManager>> disk_manager_;
  char buffer[BUFFER_SIZE];
  size_t offset_;
  atomic<size_t> lsn_ = 0;
  atomic<size_t> flushed_lsn_ = 0;
  thread flusher_;
  condition_variable cv_produce_;
  condition_variable cv_consume_;
  bool stop_ = true;
  mutex log_mutex_;
  fstream file_;

  void StartFlushLog();
  void Recover();

 public:
  LogManager(const vector<shared_ptr<DiskManager>> &disk_manager);
  ~LogManager();
  size_t AppendLog(page_id_t page_id, const char *data);
  void FlushToLsn(size_t lsn);  
};