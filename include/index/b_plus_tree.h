
#pragma once

#include <deque>
#include <optional>
#include <string>
#include <vector>

#include "common/config.h"
#include "page/b_plus_tree_internal_page.h"
#include "page/b_plus_tree_leaf_page.h"
#include "page/page_guard.h"


class Context {
 public:
  std::optional<WritePageGuard> write_header_page_{std::nullopt};
  std::optional<ReadPageGuard> read_header_page_{std::nullopt};
  page_id_t root_page_id_{INVALID_PAGE_ID};

  std::deque<WritePageGuard> write_set_;

  std::deque<ReadPageGuard> read_set_;

  auto IsRootPage(page_id_t page_id) -> bool { return page_id == root_page_id_; }
};

#define BPLUSTREE_TYPE BPlusTree<KeyType, ValueType, NumTombs>

FULL_INDEX_TEMPLATE_ARGUMENTS_DEFN
class BPlusTree {
  using InternalPage = BPlusTreeInternalPage<KeyType, page_id_t>;
  using LeafPage = BPlusTreeLeafPage<KeyType, ValueType, NumTombs>;

 public:
  explicit BPlusTree(size_t file_id, page_id_t header_page_id,BufferPoolManager *buffer_pool_manager, 
  int leaf_max_size = LEAF_PAGE_SLOT_CNT, 
  int internal_max_size = INTERNAL_PAGE_SLOT_CNT);

  // Returns true if this B+ tree has no keys and values.
  auto IsEmpty() const -> bool;

  // Insert a key-value pair into this B+ tree.
  auto Insert(const KeyType &key, const ValueType &value) -> bool;

  // Remove a key and its value from this B+ tree.
  void Remove(const KeyType &key);

  // Return the value associated with a given key
  auto GetValue(const KeyType &key, std::vector<ValueType> *result) -> bool;

  // Return the page id of the root node
  auto GetRootPageId() -> page_id_t;
  /*
  // Index iterator
  auto Begin() -> INDEXITERATOR_TYPE;

  auto End() -> INDEXITERATOR_TYPE;

  auto Begin(const KeyType &key) -> INDEXITERATOR_TYPE;*/

  std::shared_ptr<BufferPoolManager> bpm_;

 private:

  auto FindLeaf(const KeyType &key, Context &ctx, bool is_opt) -> page_id_t;

  // member variable
  size_t file_id_;
  std::vector<std::string> log;  // NOLINT
  int leaf_max_size_;
  int internal_max_size_;
  page_id_t header_page_id_;
};