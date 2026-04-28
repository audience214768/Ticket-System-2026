#include "disk/disk_manager.h"
#include "common/config.h"
#include <cstring>
#include <iostream>
using std::ios;

DiskManager::DiskManager(size_t file_id, const string &db_file_name):fileID_(file_id), db_file_name_(db_file_name) {
  db_file_io_.open(db_file_name_, ios::in | ios::out | ios::binary);
  if (!db_file_io_.is_open()) {
    db_file_io_.clear();
    db_file_io_.open(db_file_name_, ios::out | ios::binary | ios::trunc | ios::in);
  } else {
    db_file_io_.seekg(0);
    db_file_io_.read(reinterpret_cast<char *>(&next_free_page_), sizeof(page_id_t));
    db_file_io_.seekg(sizeof(page_id_t));
    db_file_io_.read(reinterpret_cast<char *>(&next_page_id_), sizeof(page_id_t));
  }

  log_file_name_ = db_file_name_ + ".log"; 
  log_file_io_.open(log_file_name_, ios::in | ios::out | ios::binary | ios::app);
  if(!log_file_io_.is_open()) {
    log_file_io_.open(log_file_name_, ios::out | ios::binary);
    log_file_io_.close();
    log_file_io_.open(log_file_name_, ios::in | ios::out | ios::binary | ios::app);
  }
}

DiskManager::~DiskManager() {
  db_file_io_.seekp(0);
  db_file_io_.write(reinterpret_cast<const char *>(&next_free_page_), sizeof(page_id_t));
  db_file_io_.seekp(sizeof(page_id_t));
  db_file_io_.write(reinterpret_cast<const char *>(&next_page_id_), sizeof(page_id_t));
  db_file_io_.close();
  log_file_io_.close();
}

void DiskManager::ReadPage(page_id_t page_id, char *data) {
  auto offset = OFFSET(page_id % DISK_FILE_SIZE);
  if (offset >= GetGileSize()) {
    memset(data, 0, DISK_PAGE_SIZE);
    return;
  }
  db_file_io_.seekg(offset);
  db_file_io_.read(data, DISK_PAGE_SIZE);
}

void DiskManager::WritePage(page_id_t page_id, const char *data) {
  db_file_io_.seekp(OFFSET(page_id % DISK_FILE_SIZE));
  db_file_io_.write(data, DISK_PAGE_SIZE);
  db_file_io_.flush();
}

auto DiskManager::NewPage() -> page_id_t{
  if(next_free_page_ != INVALID_PAGE_ID) {
    page_id_t last_free_page = next_free_page_;
    db_file_io_.seekg(OFFSET(next_free_page_));
    db_file_io_.read(reinterpret_cast<char *>(&next_free_page_), sizeof(page_id_t));
    return (fileID_ << 10) | last_free_page;
  }
  return (fileID_ << 10) | next_page_id_++;
}

void DiskManager::DeletePage(page_id_t page_id) {
  db_file_io_.seekp(OFFSET(page_id % DISK_FILE_SIZE));
  db_file_io_.write(reinterpret_cast<const char *>(&next_free_page_), sizeof(page_id_t));
  next_free_page_ = page_id;
}

void DiskManager::WriteLog(const char *log, int size) {
  log_file_io_.write(log, size);
}

auto DiskManager::GetGileSize() -> size_t {
  auto current_pos = db_file_io_.tellg();
  db_file_io_.seekg(0, ios::end);
  size_t size = db_file_io_.tellg();
  db_file_io_.seekg(current_pos);
  return size;
}