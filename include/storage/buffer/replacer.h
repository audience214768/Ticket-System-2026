#pragma once

#include "common/config.h"
#include "vector/vector.hpp"
#include "deque/deque.hpp"

using sjtu::vector;
using sjtu::deque;

class LRUKReplacer;

class Node {
  friend LRUKReplacer;
 private:
  deque<size_t> history_;
  size_t k_;
  bool is_evictable_ = true;
  Node() = delete;
  Node(size_t k):k_(k) {}
  void Visit(size_t current_time_stamp) { 
    if(history_.size() < k_) {
      history_.push_back(current_time_stamp);
    } else {
      history_.pop_front();
      history_.push_back(current_time_stamp);
    }
  }
  auto KTime() -> size_t { return history_.size() == k_ ? history_.back() : INF_TIME_STAMP;}
  auto LastTime() -> size_t {return history_.back(); }
  void Clear() { history_.clear(); }
 public:
  
};

class LRUKReplacer {
 private:
  vector<Node> node_store_;
  size_t frame_num_;
  size_t k_;
  size_t curr_time = 0;
 public:
  LRUKReplacer(size_t frame_num, size_t k = 2);
  ~LRUKReplacer() = default;
  auto Evict() -> frame_id_t;
  void Access(frame_id_t frame_id);
  void SetEvictable(frame_id_t frame_id, bool set_evictable);
};

class ClockReplacer {
 private:
  vector<bool> ref_;
  vector<bool> evictable_;
  int hand_ = 0;   
 public:
  ClockReplacer(size_t frame);
  auto Evict() -> frame_id_t;
  void Access(frame_id_t frame_id);
  void SetEvictable(frame_id_t frame_id, bool set_evictable);
};