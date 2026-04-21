#include "common/config.h"
#include <future>

struct DiskRequest {
  bool is_write_;
  char *data_;
  page_id_t page_id;
  std::promise<bool> callback;
};

class DiskScheduler {

};