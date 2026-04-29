#include "buffer/replacer.h"
#include "common/config.h"

Replacer::Replacer(size_t frame_num, size_t k):frame_num_(frame_num), k_(k), node_store_(frame_num, Node(k)) {}

void Replacer::Access(frame_id_t frame_id) {
  node_store_[frame_id].Visit(curr_time++);
}

auto Replacer::Evict() ->frame_id_t {
  frame_id_t evict_id = INVALID_FRAME_ID;
  // int count = 0;
  // for (int i = 0; i < frame_num_; i++) {
  //   if (node_store_[i].is_evictable_ == false) {
  //     count++;
  //   }
  // }
  // std::cerr << count << std::endl;
  for (int i = 0; i < frame_num_; i++) {
    if(node_store_[i].history_.empty() || node_store_[i].is_evictable_ == false) {
      continue;
    }
    if (evict_id == INVALID_FRAME_ID) {
      evict_id = i;
    } else if (node_store_[evict_id].KTime() > node_store_[i].KTime() ||
               (node_store_[evict_id].KTime() == node_store_[i].KTime() && node_store_[evict_id].LastTime() > node_store_[i].LastTime())) {
      evict_id = i;
    }
  }
  node_store_[evict_id].Clear();
  return evict_id;
}

void Replacer::SetEvictable(frame_id_t frame_id, bool set_evictable) {
  node_store_[frame_id].is_evictable_ = set_evictable;
}