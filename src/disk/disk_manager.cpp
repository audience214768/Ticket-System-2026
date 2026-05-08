#include "disk/disk_manager.h"
#include "common/config.h"
#include <cstring>
#include <iostream>
#include <mutex>

using std::unique_lock;
using std::ios;

DiskManager::DiskManager(size_t file_id, const string &db_file_name):fileID_(file_id), db_file_name_(db_file_name) {
  db_file_io_.open(db_file_name_, ios::in | ios::out | ios::binary);
  if (!db_file_io_.is_open()) {
    db_file_io_.clear();
    db_file_io_.open(db_file_name_, ios::out | ios::binary | ios::trunc | ios::in);
  } else {
    db_file_io_.clear();
    db_file_io_.seekg(0);
    db_file_io_.read(reinterpret_cast<char *>(&next_free_page_), sizeof(page_id_t));
    db_file_io_.seekg(sizeof(page_id_t));
    db_file_io_.read(reinterpret_cast<char *>(&next_page_id_), sizeof(page_id_t));
  }
}

DiskManager::~DiskManager() {
  db_file_io_.clear();
  db_file_io_.seekp(0);
  db_file_io_.write(reinterpret_cast<const char *>(&next_free_page_), sizeof(page_id_t));
  db_file_io_.seekp(sizeof(page_id_t));
  db_file_io_.write(reinterpret_cast<const char *>(&next_page_id_), sizeof(page_id_t));
  db_file_io_.flush();
  db_file_io_.close();
}

void DiskManager::ReadPage(page_id_t page_id, char *data) {
  unique_lock<mutex> lock(io_mutex_);
  auto offset = OFFSET(page_id % DISK_FILE_SIZE);
  if (offset >= GetFileSize()) {
    memset(data, 0, DISK_PAGE_SIZE);
    return;
  }
  db_file_io_.clear();
  db_file_io_.seekg(offset);
  db_file_io_.read(data, DISK_PAGE_SIZE);
}

void DiskManager::WritePage(page_id_t page_id, const char *data) {
  unique_lock<mutex> lock(io_mutex_);
  db_file_io_.clear();
  db_file_io_.seekp(OFFSET(page_id % DISK_FILE_SIZE));
  db_file_io_.write(data, DISK_PAGE_SIZE);
  db_file_io_.flush();
}

auto DiskManager::NewPage() -> page_id_t{
  unique_lock<mutex> lock(io_mutex_);
  if(next_free_page_ != INVALID_PAGE_ID) {
    page_id_t last_free_page = next_free_page_;
    db_file_io_.clear();
    db_file_io_.seekg(OFFSET(next_free_page_ % DISK_FILE_SIZE));
    db_file_io_.read(reinterpret_cast<char *>(&next_free_page_), sizeof(page_id_t));
    //std::cerr << "new " << last_free_page << " " << next_free_page_ << std::endl;
    return (fileID_ << FILE_BIT) | last_free_page;
  }
  return (fileID_ << FILE_BIT) | next_page_id_++;
}

void DiskManager::DeletePage(page_id_t page_id) {
  //std::cerr << "delete " << page_id << " " << next_free_page_ << std::endl;
  unique_lock<mutex> lock(io_mutex_);
  db_file_io_.clear();
  db_file_io_.seekp(OFFSET(page_id % DISK_FILE_SIZE));
  db_file_io_.write(reinterpret_cast<const char *>(&next_free_page_), sizeof(page_id_t));
  db_file_io_.flush();
  next_free_page_ = page_id;
}

auto DiskManager::GetFileSize() -> size_t {
  db_file_io_.clear();
  auto current_pos = db_file_io_.tellg();
  db_file_io_.seekg(0, ios::end);
  size_t size = db_file_io_.tellg();
  db_file_io_.seekg(current_pos);
  return size;
}