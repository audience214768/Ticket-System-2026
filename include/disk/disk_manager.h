#pragma once

#include "common/config.h"
#include <fstream>
#include <string>

using std::string;
using std::fstream;
using std::ios;

enum OpType {READY, WRITE, FINISH};

struct LogHeader {
  OpType type;
  int offset;
  int data_size;
  unsigned int checksum;
  unsigned int magic = 0xDEADBEEF;
};


class DiskManager {
 private:
  size_t fileID_;

  string db_file_name_;
  fstream db_file_io_;
  
  string log_file_name_;
  fstream log_file_io_;

  page_id_t next_free_page_ = INVALID_PAGE_ID;
  page_id_t next_page_id_ = 1;

  int info_len = 2;
  #define OFFSET(page_id) ((page_id) * DISK_PAGE_SIZE + info_len *sizeof(page_id_t))

 public:
  DiskManager() = delete;
  explicit DiskManager(size_t file_id, const string &db_file_name);
  ~DiskManager();

  void ReadPage(page_id_t page_id, char *data);
  void WritePage(page_id_t page_id, const char *data);
  auto NewPage() -> page_id_t;
  void DeletePage(page_id_t page_id);

  void WriteLog(const char *log, int size);

  auto GetFileSize() -> size_t;

};