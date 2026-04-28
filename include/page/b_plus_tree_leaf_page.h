#pragma once

#include "page/b_plus_tree_page.h"
#include "common/config.h"
#include "vector/vector.hpp"


using sjtu::vector;
using std::string;

#define B_PLUS_TREE_LEAF_PAGE_TYPE BPlusTreeLeafPage<KeyType, ValueType, Compare>
#define LEAF_PAGE_HEADER_SIZE 16
#define LEAF_PAGE_SLOT_CNT                                                                               \
  ((DISK_PAGE_SIZE - LEAF_PAGE_HEADER_SIZE - sizeof(size_t)) / \
   (sizeof(KeyType) + sizeof(ValueType)))

INDEX_TEMPLATE_ARGUMENTS
class BPlusTreeLeafPage : public BPlusTreePage {
 public:
  BPlusTreeLeafPage() = delete;
  BPlusTreeLeafPage(const BPlusTreeLeafPage &other) = delete;

  void Init(int max_size = LEAF_PAGE_SLOT_CNT);
  auto GetNextPageId() const -> page_id_t;
  void SetNextPageId(page_id_t next_page_id);
  auto KeyAt(int index) const -> const KeyType &;
  auto ValueAt(int index) const -> const ValueType &;
  auto Search(const KeyType &key, const Compare &compare) const -> int;
  auto Insert(const KeyType &key, const ValueType &value, const Compare &compare) -> bool;
  auto Delete(const KeyType &key, const Compare &compare) -> bool;
  void MoveHalf(B_PLUS_TREE_LEAF_PAGE_TYPE *other);
  auto MoveBack(B_PLUS_TREE_LEAF_PAGE_TYPE *other) -> KeyType;
  auto MoveFront(B_PLUS_TREE_LEAF_PAGE_TYPE *other) -> KeyType;
  void Merge(B_PLUS_TREE_LEAF_PAGE_TYPE *other);
  //void FlushTomb();
  
  auto ToString() const -> string {
    string kstr = "(";
    bool first = true;

    for (int i = 0; i < GetSize(); i++) {
      KeyType key = KeyAt(i);
      if (first) {
        first = false;
      } else {
        kstr.append(",");
      }

      kstr.append(to_string(key.ToString()));
    }
    kstr.append(")");

    return kstr;
  }

 private:
  page_id_t next_page_id_;
  KeyType key_array_[LEAF_PAGE_SLOT_CNT];
  ValueType rid_array_[LEAF_PAGE_SLOT_CNT];
};

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::Init(int max_size) {
  SetMaxSize(max_size);
  SetSize(0);
  //num_tombstones_ = 0;
  SetPageType(IndexPageType::LEAF_PAGE);
  SetNextPageId(INVALID_PAGE_ID);
}

INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::GetNextPageId() const -> page_id_t {
  return next_page_id_;
}

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::SetNextPageId(page_id_t next_page_id) {
  next_page_id_ = next_page_id;
}

INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::KeyAt(int index) const -> const KeyType &{
  return key_array_[index];
}

INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::ValueAt(int index) const -> const ValueType &{
  return rid_array_[index];
}

INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::Search(const KeyType &key, const Compare &compare) const -> int {
  int l = 0, r = GetSize();
  int ans = l - 1;
  while(l < r) {
    int m = (l + r) >> 1;
    if(compare(key, key_array_[m]) >= 0) {
      ans = m;
      l = m + 1;
    } else {
      r = m;
    }
  }
  return ans;
}

INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::Insert(const KeyType &key, const ValueType &value, const Compare &compare) -> bool{ //false mean the key exist
  int index = Search(key, compare);
  if(index != -1 && compare(key, key_array_[index]) == 0) {
    return false;
  }
  for(int i = GetSize(); i > index + 1; i--) {
    key_array_[i] = key_array_[i - 1];
    rid_array_[i] = rid_array_[i - 1];
  }
  key_array_[index + 1] = key;
  rid_array_[index + 1] = value;
  SetSize(GetSize() + 1);
  return true;
} 

INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::Delete(const KeyType &key, const Compare &compare) -> bool {
  auto index = Search(key, compare);
  if(index == -1 || compare(key, key_array_[index]) != 0) {
    return false;
  }
  for(int i = index; i < GetSize() - 1; i++) {
    key_array_[i] = key_array_[i + 1];
    rid_array_[i] = rid_array_[i + 1];
  }
  SetSize(GetSize() - 1);
  return true;
}

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::MoveHalf(B_PLUS_TREE_LEAF_PAGE_TYPE *other) {
  for(int i = other->GetSize() / 2; i < other->GetSize(); i++) {
    key_array_[i - other->GetSize() / 2] = other->key_array_[i];
    rid_array_[i - other->GetSize() / 2] = other->rid_array_[i];
  }
  SetSize(other->GetSize() - other->GetSize() / 2);
  other->SetSize(other->GetSize() / 2);
}

INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::MoveBack(B_PLUS_TREE_LEAF_PAGE_TYPE *other) -> KeyType { // move the back of this to the other
  int new_size = (GetSize() + other->GetSize()) >> 1;
  for(int i = other->GetSize() - 1; i >= 0; i--) {
    other->key_array_[i + new_size - other->GetSize()] = other->key_array_[i];
    other->rid_array_[i + new_size - other->GetSize()] = other->rid_array_[i];
  }
  for(int i = 0; i < new_size - other->GetSize(); i++) {
    other->key_array_[i] = key_array_[GetSize() - (new_size - other->GetSize()) + i];
    other->rid_array_[i] = rid_array_[GetSize() - (new_size - other->GetSize()) + i];
  }
  SetSize(GetSize() - (new_size - other->GetSize()));
  other->SetSize(new_size);
  return other->key_array_[0];
}

INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::MoveFront(B_PLUS_TREE_LEAF_PAGE_TYPE *other) -> KeyType {
  int new_size = (GetSize() + other->GetSize()) >> 1;
  for(int i = other->GetSize(); i < new_size; i++) {
    other->key_array_[i] = key_array_[i - other->GetSize()];
    other->rid_array_[i] = rid_array_[i - other->GetSize()];
  }
  int size = GetSize() + other->GetSize() - new_size;
  for(int i = 0; i < size; i++) {
    key_array_[i] = key_array_[i + new_size - other->GetSize()];
    rid_array_[i] = rid_array_[i + new_size - other->GetSize()];
  }
  other->SetSize(new_size);
  SetSize(size);
  return key_array_[0];
}

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::Merge(B_PLUS_TREE_LEAF_PAGE_TYPE *other) {
  for(int i = GetSize(); i < GetSize() + other->GetSize(); i++) {
    key_array_[i] = other->key_array_[i - GetSize()];
    rid_array_[i] = other->rid_array_[i - GetSize()];
  }
  SetSize(GetSize() + other->GetSize());
}

