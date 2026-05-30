#pragma once

#include "page/b_plus_tree_page.h"
#include "common/config.h"
#include "vector/vector.hpp"
#include <cstring>


using sjtu::vector;
using std::string;
using std::memmove;

#define B_PLUS_TREE_LEAF_PAGE_TYPE BPlusTreeLeafPage<KeyType, ValueType, Compare>

template <typename KeyType, typename ValueType, typename Compare>
class BPlusTreeLeafPage : public BPlusTreePage {
 static const size_t LEAF_PAGE_HEADER_SIZE = 20;
 static const size_t LEAF_PAGE_SLOT_CNT = ((DISK_PAGE_SIZE - LEAF_PAGE_HEADER_SIZE - sizeof(size_t)) / (sizeof(KeyType) + sizeof(ValueType)));
 public:
  BPlusTreeLeafPage() = delete;
  BPlusTreeLeafPage(const BPlusTreeLeafPage &other) = delete;

  void Init(int max_size = LEAF_PAGE_SLOT_CNT);
  auto GetNextPageId() const -> page_id_t;
  void SetNextPageId(page_id_t next_page_id);
  auto KeyAt(int index) const -> const KeyType &;
  auto ValueAt(int index) const -> const ValueType &;
  void SetValueAt(int index, const ValueType &value);
  auto Search(const KeyType &key, const Compare &compare) const -> int;
  auto Insert(const KeyType &key, const ValueType &value, const Compare &compare) -> bool;
  auto Delete(const KeyType &key, const Compare &compare) -> bool;
  void MoveHalf(B_PLUS_TREE_LEAF_PAGE_TYPE *other);
  auto MoveBack(B_PLUS_TREE_LEAF_PAGE_TYPE *other) -> KeyType;
  auto MoveFront(B_PLUS_TREE_LEAF_PAGE_TYPE *other) -> KeyType;
  void Merge(B_PLUS_TREE_LEAF_PAGE_TYPE *other);
  //void FlushTomb();
  
  void ToString() const {
    std::cerr << "(";
    bool first = true;

    for (int i = 0; i < GetSize(); i++) {
      KeyType key = KeyAt(i);
      if (first) {
        first = false;
      } else {
        std::cerr << ",";
      }

      std::cerr << key.GetKey() << " " << ValueAt(i);
    }
    std::cerr << ")" << std::endl;
  }

 private:
  page_id_t next_page_id_;
  KeyType key_array_[LEAF_PAGE_SLOT_CNT];
  ValueType rid_array_[LEAF_PAGE_SLOT_CNT];
};

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::Init(int max_size) {
  if (max_size == 0) {
    max_size = LEAF_PAGE_SLOT_CNT;
  }
  SetMaxSize(max_size);
  SetSize(0);
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
void B_PLUS_TREE_LEAF_PAGE_TYPE::SetValueAt(int index, const ValueType &value) {
  rid_array_[index] = value;
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
  // for(int i = GetSize(); i > index + 1; i--) {
  //   key_array_[i] = key_array_[i - 1];
  //   rid_array_[i] = rid_array_[i - 1];
  // }
  memmove(key_array_ + index + 2, key_array_ + index + 1, (GetSize() - index - 1) * sizeof(KeyType));
  memmove(rid_array_ + index + 2, rid_array_ + index + 1, (GetSize() - index - 1) * sizeof(ValueType));
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
  // for(int i = index; i < GetSize() - 1; i++) {
  //   key_array_[i] = key_array_[i + 1];
  //   rid_array_[i] = rid_array_[i + 1];
  // }
  memmove(key_array_ + index, key_array_ + index + 1, (GetSize() - 1 - index) * sizeof(KeyType));
  memmove(rid_array_ + index, rid_array_ + index + 1, (GetSize() - 1 - index) * sizeof(ValueType));
  SetSize(GetSize() - 1);
  return true;
}

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::MoveHalf(B_PLUS_TREE_LEAF_PAGE_TYPE *other) {
  // for(int i = other->GetSize() / 2; i < other->GetSize(); i++) {
  //   key_array_[i - other->GetSize() / 2] = other->key_array_[i];
  //   rid_array_[i - other->GetSize() / 2] = other->rid_array_[i];
  // }
  memmove(key_array_, other->key_array_ + other->GetSize() / 2, (other->GetSize() - other->GetSize() / 2) * sizeof(KeyType));
  memmove(rid_array_, other->rid_array_ + other->GetSize() / 2, (other->GetSize() - other->GetSize() / 2) * sizeof(ValueType));
  SetSize(other->GetSize() - other->GetSize() / 2);
  other->SetSize(other->GetSize() / 2);
}

INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::MoveBack(B_PLUS_TREE_LEAF_PAGE_TYPE *other) -> KeyType { // move the back of this to the other
  int new_size = (GetSize() + other->GetSize()) >> 1;
  // for(int i = other->GetSize() - 1; i >= 0; i--) {
  //   other->key_array_[i + new_size - other->GetSize()] = other->key_array_[i];
  //   other->rid_array_[i + new_size - other->GetSize()] = other->rid_array_[i];
  // }
  memmove(other->key_array_ + new_size - other->GetSize(), other->key_array_, other->GetSize() * sizeof(KeyType));
  memmove(other->rid_array_ + new_size - other->GetSize(), other->rid_array_, other->GetSize() * sizeof(ValueType));
  // for(int i = 0; i < new_size - other->GetSize(); i++) {
  //   other->key_array_[i] = key_array_[GetSize() - (new_size - other->GetSize()) + i];
  //   other->rid_array_[i] = rid_array_[GetSize() - (new_size - other->GetSize()) + i];
  // }
  memmove(other->key_array_, key_array_ + GetSize() - (new_size - other->GetSize()), (new_size - other->GetSize()) * sizeof(KeyType));
  memmove(other->rid_array_, rid_array_ + GetSize() - (new_size - other->GetSize()), (new_size - other->GetSize()) * sizeof(ValueType));
  SetSize(GetSize() - (new_size - other->GetSize()));
  other->SetSize(new_size);
  return other->key_array_[0];
}

INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::MoveFront(B_PLUS_TREE_LEAF_PAGE_TYPE *other) -> KeyType {
  int new_size = (GetSize() + other->GetSize()) >> 1;
  // for(int i = other->GetSize(); i < new_size; i++) {
  //   other->key_array_[i] = key_array_[i - other->GetSize()];
  //   other->rid_array_[i] = rid_array_[i - other->GetSize()];
  // }
  memmove(other->key_array_ + other->GetSize(), key_array_, (new_size - other->GetSize()) * sizeof(KeyType));
  memmove(other->rid_array_ + other->GetSize(), rid_array_, (new_size - other->GetSize()) * sizeof(ValueType));
  int size = GetSize() + other->GetSize() - new_size;
  // for(int i = 0; i < size; i++) {
  //   key_array_[i] = key_array_[i + new_size - other->GetSize()];
  //   rid_array_[i] = rid_array_[i + new_size - other->GetSize()];
  // }
  memmove(key_array_, key_array_ + new_size - other->GetSize(), size * sizeof(KeyType));
  memmove(rid_array_, rid_array_ + new_size - other->GetSize(), size * sizeof(ValueType));
  other->SetSize(new_size);
  SetSize(size);
  return key_array_[0];
}

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::Merge(B_PLUS_TREE_LEAF_PAGE_TYPE *other) {
  // for(int i = GetSize(); i < GetSize() + other->GetSize(); i++) {
  //   key_array_[i] = other->key_array_[i - GetSize()];
  //   rid_array_[i] = other->rid_array_[i - GetSize()];
  // }
  memmove(key_array_ + GetSize(), other->key_array_, other->GetSize() * sizeof(KeyType));
  memmove(rid_array_ + GetSize(), other->rid_array_, other->GetSize() * sizeof(ValueType));
  SetSize(GetSize() + other->GetSize());
  other->SetSize(0);
}

