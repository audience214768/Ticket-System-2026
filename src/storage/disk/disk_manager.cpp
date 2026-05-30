#include "disk/disk_manager.h"
#include "common/config.h"
#include "utils/err.h"
#include <cstring>
#include <fcntl.h>
#include <mutex>
#include <sys/stat.h>
#include <unistd.h>

using std::unique_lock;

DiskManager::DiskManager(size_t file_id, const string &db_file_name)
    : fileID_(file_id), db_file_name_(db_file_name) {
  fd_ = ::open(db_file_name_.c_str(), O_RDWR);
  if (fd_ < 0) {
    fd_ = ::open(db_file_name_.c_str(), O_RDWR | O_CREAT, 0644);
    if (fd_ < 0) return;
  } else {
    ::pread(fd_, &next_free_page_, sizeof(page_id_t), 0);
    ::pread(fd_, &next_page_id_, sizeof(page_id_t), sizeof(page_id_t));
  }
}

DiskManager::~DiskManager() {
  if (fd_ < 0) return;
  ::pwrite(fd_, &next_free_page_, sizeof(page_id_t), 0);
  ::pwrite(fd_, &next_page_id_, sizeof(page_id_t), sizeof(page_id_t));
  ::close(fd_);
}

void DiskManager::ReadPage(page_id_t page_id, char *data) {
  unique_lock<mutex> lock(io_mutex_);
  auto offset = OFFSET(page_id % DISK_FILE_SIZE);
  if (offset >= GetFileSize()) {
    memset(data, 0, DISK_PAGE_SIZE);
    return;
  }
  ::pread(fd_, data, DISK_PAGE_SIZE, offset);
}

void DiskManager::WritePage(page_id_t page_id, const char *data) {
  unique_lock<mutex> lock(io_mutex_);
  ::pwrite(fd_, data, DISK_PAGE_SIZE, OFFSET(page_id % DISK_FILE_SIZE));
}

auto DiskManager::NewPage() -> page_id_t {
  unique_lock<mutex> lock(io_mutex_);
  if (next_free_page_ != INVALID_PAGE_ID) {
    page_id_t last_free_page = next_free_page_;
    ::pread(fd_, &next_free_page_, sizeof(page_id_t), OFFSET(next_free_page_ % DISK_FILE_SIZE));
    //std::cerr << "new" << " " << last_free_page << " " << next_free_page_ << std::endl;
    return (fileID_ << FILE_BIT) | last_free_page;
  }
  if (next_page_id_ >= DISK_FILE_SIZE) {
    throw Exception("disk file page exhausted");
  }
  return (fileID_ << FILE_BIT) | next_page_id_++;
}

void DiskManager::DeletePage(page_id_t page_id) {
  unique_lock<mutex> lock(io_mutex_);
  int write = ::pwrite(fd_, &next_free_page_, sizeof(page_id_t), OFFSET(page_id % DISK_FILE_SIZE));
  next_free_page_ = page_id;
}

auto DiskManager::GetFileSize() -> size_t {
  struct stat st;
  if (::fstat(fd_, &st) != 0) return 0;
  return static_cast<size_t>(st.st_size);
}

void DiskManager::Clear() {
  ::ftruncate(fd_, 0);
  next_free_page_ = INVALID_PAGE_ID;
  next_page_id_ = 1;
}
