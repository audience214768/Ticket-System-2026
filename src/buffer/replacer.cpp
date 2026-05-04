#include "buffer/replacer.h"
#include "common/config.h"

LRUKReplacer::LRUKReplacer(size_t frame_num, size_t k)
    : frame_num_(frame_num), k_(k), node_store_(frame_num, Node(k)) {}

void LRUKReplacer::Access(frame_id_t frame_id) {
  node_store_[frame_id].Visit(curr_time++);
}

auto LRUKReplacer::Evict() -> frame_id_t {
  frame_id_t evict_id = INVALID_FRAME_ID;
  for (int i = 0; i < frame_num_; i++) {
    if (node_store_[i].history_.empty() ||
        node_store_[i].is_evictable_ == false) {
      continue;
    }
    if (evict_id == INVALID_FRAME_ID) {
      evict_id = i;
    } else if (node_store_[evict_id].KTime() > node_store_[i].KTime() ||
               (node_store_[evict_id].KTime() == node_store_[i].KTime() &&
                node_store_[evict_id].LastTime() > node_store_[i].LastTime())) {
      evict_id = i;
    }
  }
  node_store_[evict_id].Clear();
  return evict_id;
}

void LRUKReplacer::SetEvictable(frame_id_t frame_id, bool set_evictable) {
  node_store_[frame_id].is_evictable_ = set_evictable;
}

ClockReplacer::ClockReplacer(size_t frame_num) : ref_(frame_num, false), evictable_(frame_num, true){}

void ClockReplacer::Access(frame_id_t frame_id) {
  ref_[frame_id] = true;
}

void ClockReplacer::SetEvictable(frame_id_t frame_id, bool enable) {
  evictable_[frame_id] = enable;
}
auto ClockReplacer::Evict() -> frame_id_t {
  while (true) {
    //std::cerr << "try" << std::endl;
    if (!evictable_[hand_]) {
      hand_ = (hand_ + 1) % ref_.size();
      continue;
    }
    if (!ref_[hand_]) {
      int victim = hand_;
      hand_ = (hand_ + 1) % ref_.size();
      return victim;
    }
    ref_[hand_] = false;
    hand_ = (hand_ + 1) % ref_.size();
  }
}