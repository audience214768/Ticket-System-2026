#pragma once

#include "page/b_plus_tree_page.h"
#include <vector>

using std::vector;

#define B_PLUS_TREE_LEAF_PAGE_TYPE BPlusTreeLeafPage<KeyType, ValueType, NumTombs>
#define LEAF_PAGE_HEADER_SIZE 16
#define LEAF_PAGE_DEFAULT_TOMB_CNT 0
#define LEAF_PAGE_TOMB_CNT ((NumTombs < 0) ? LEAF_PAGE_DEFAULT_TOMB_CNT : NumTombs)
#define LEAF_PAGE_SLOT_CNT                                                                               \
  ((DISK_PAGE_SIZE - LEAF_PAGE_HEADER_SIZE - sizeof(size_t) - (LEAF_PAGE_TOMB_CNT * sizeof(size_t))) / \
   (sizeof(KeyType) + sizeof(ValueType)))

FULL_INDEX_TEMPLATE_ARGUMENTS_DEFN
class BPlusTreeLeafPage : public BPlusTreePage {
 public:
  BPlusTreeLeafPage() = delete;
  BPlusTreeLeafPage(const BPlusTreeLeafPage &other) = delete;

  void Init(int max_size = LEAF_PAGE_SLOT_CNT);

  auto GetTombstones() const -> vector<KeyType>;
  auto GetTombCount() const -> int;
  auto GetNextPageId() const -> page_id_t;
  void SetNextPageId(page_id_t next_page_id);
  auto KeyAt(int index) const -> const KeyType &;
  auto isTomb(int index) const ->bool;
  void setTomb(int index);
  auto ValueAt(int index) const -> const ValueType &;
  auto Search(const KeyType &key) const -> int;
  auto Insert(const KeyType &key, const ValueType &value) -> bool;
  auto Delete(const KeyType &key) -> bool;
  void MoveHalf(B_PLUS_TREE_LEAF_PAGE_TYPE *other);
  auto MoveBack(B_PLUS_TREE_LEAF_PAGE_TYPE *other) -> KeyType;
  auto MoveFront(B_PLUS_TREE_LEAF_PAGE_TYPE *other) -> KeyType;
  void Merge(B_PLUS_TREE_LEAF_PAGE_TYPE *other);
  void FlushTomb();
  
  auto ToString() const -> std::string {
    std::string kstr = "(";
    bool first = true;

    auto tombs = GetTombstones();
    for (size_t i = 0; i < tombs.size(); i++) {
      kstr.append(std::to_string(tombs[i].ToString()));
      if ((i + 1) < tombs.size()) {
        kstr.append(",");
      }
    }

    kstr.append("|");

    for (int i = 0; i < GetSize(); i++) {
      KeyType key = KeyAt(i);
      if (first) {
        first = false;
      } else {
        kstr.append(",");
      }

      kstr.append(std::to_string(key.ToString()));
    }
    kstr.append(")");

    return kstr;
  }

 private:
  page_id_t next_page_id_;
  size_t num_tombstones_;
  size_t tombstones_[LEAF_PAGE_TOMB_CNT];
  KeyType key_array_[LEAF_PAGE_SLOT_CNT];
  ValueType rid_array_[LEAF_PAGE_SLOT_CNT];
  void DeleteTomb(int index);
};

