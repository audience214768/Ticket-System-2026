

#pragma once

#include "vector/vector.hpp"
#include "common/config.h"
#include "page/b_plus_tree_page.h"

using sjtu::vector;
using std::cerr;
using std::endl;

#define B_PLUS_TREE_INTERNAL_PAGE_TYPE BPlusTreeInternalPage<KeyType, ValueType, Compare>
#define INTERNAL_PAGE_HEADER_SIZE 12
#define INTERNAL_PAGE_SLOT_CNT \
  ((DISK_PAGE_SIZE - INTERNAL_PAGE_HEADER_SIZE) / ((sizeof(KeyType) + sizeof(ValueType))))

INDEX_TEMPLATE_ARGUMENTS
class BPlusTreeInternalPage : public BPlusTreePage {
 public:
  BPlusTreeInternalPage() = delete;
  BPlusTreeInternalPage(const BPlusTreeInternalPage &other) = delete;

  void Init(int max_size = INTERNAL_PAGE_SLOT_CNT);

  auto KeyAt(int index) const -> KeyType;

  void SetKeyAt(int index, const KeyType &key);

  auto ValueIndex(const ValueType &value) const -> int;

  auto ValueAt(int index) const -> ValueType;
  void SetValueAt(int index, const ValueType &value);
  auto Search(const KeyType &key, const Compare &compare) const -> int;
  void Insert(const KeyType &key, page_id_t page_id, const Compare &compare);
  void Delete(const KeyType &key, const Compare &compare);
  auto MoveHalf(B_PLUS_TREE_INTERNAL_PAGE_TYPE *other, const KeyType &key, const ValueType &value, int index) -> KeyType;
  auto MoveBack(B_PLUS_TREE_INTERNAL_PAGE_TYPE *other, const KeyType &key) -> KeyType;
  auto MoveFront(B_PLUS_TREE_INTERNAL_PAGE_TYPE *other, const KeyType &key) -> KeyType;
  void Merge(B_PLUS_TREE_INTERNAL_PAGE_TYPE *other, const KeyType &key);

  void ToString() const {
    cerr << "(";
    bool first = true;

    // First key of internal page is always invalid
    for (int i = 1; i < GetSize(); i++) {
      KeyType key = KeyAt(i);
      if (first) {
        first = false;
      } else {
        cerr << ",";
      }

      cerr << key.GetKey() << " " << key.rid << " " << ValueAt(i);
    }
    cerr << ")" << endl;
  }

 private:
  KeyType key_array_[INTERNAL_PAGE_SLOT_CNT];
  ValueType page_id_array_[INTERNAL_PAGE_SLOT_CNT];
};

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::Init(int max_size) {
  if (max_size == 0) {
    max_size = INTERNAL_PAGE_SLOT_CNT;
  }
  SetMaxSize(max_size);
  SetSize(0);
  SetPageType(IndexPageType::INTERNAL_PAGE);
}


INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::KeyAt(int index) const -> KeyType {
  return key_array_[index];
}


INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::SetKeyAt(int index, const KeyType &key) {
  key_array_[index] = key;
}


INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::ValueAt(int index) const -> ValueType {
  return page_id_array_[index];
}

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::SetValueAt(int index, const ValueType &value) {
  page_id_array_[index] = value;
}

INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::Search(const KeyType &key, const Compare &compare) const -> int {
  int l = 1, r = GetSize();
  int ans = 0;
  while(l < r) {
    int m = (l + r) >> 1;
    //std::cerr << key << " " << KeyAt(m) << std::endl;
    if(compare(key, key_array_[m]) >= 0) {
      ans = m;
      //std::cerr << l << " " << r << " " << ans << std::endl;
      l = m + 1;
    } else {
      r = m;
    }
  }
  return ans;
}

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::Insert(const KeyType &key, page_id_t page_id, const Compare &compare) {
  int index = Search(key, compare);
  //std::cerr << GetSize() << " " << GetMaxSize() << " " << sizeof(KeyType) << " " << sizeof(ValueType) << " " << ((DISK_PAGE_SIZE - INTERNAL_PAGE_HEADER_SIZE) / ((sizeof(KeyType) + sizeof(ValueType)))) << " " << INTERNAL_PAGE_SLOT_CNT << std::endl;
  //std::cerr << key << " insert in the " << index << std::endl;
  for(int i = GetSize(); i > index + 1; i--) {
    key_array_[i] = key_array_[i - 1];
    page_id_array_[i] = page_id_array_[i - 1];
  }
  key_array_[index + 1] = key;
  page_id_array_[index + 1] = page_id;
  SetSize(GetSize() + 1);
}

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::Delete(const KeyType &key, const Compare &compare) {
  int index = Search(key, compare);
  //std::cerr << "delete index : " << index << std::endl;
  for(int i = index; i < GetSize() - 1; i++) {
    key_array_[i] = key_array_[i + 1];
    page_id_array_[i] = page_id_array_[i + 1];
    //std::cerr << page_id_array_[i] << std::endl;
  }
  SetSize(GetSize() - 1);
}

INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::MoveHalf(B_PLUS_TREE_INTERNAL_PAGE_TYPE *other, const KeyType &key, const ValueType &value, int index) -> KeyType {
  vector<KeyType> key_vector;
  vector<ValueType> value_vector;
  //std::cerr << other->GetSize() << std::endl;
  key_vector.reserve(other->GetSize() + 1);
  value_vector.reserve(other->GetSize() + 1);
  for(int i = 0; i < other->GetSize(); i++) {
    key_vector.push_back(other->key_array_[i]);
    value_vector.push_back(other->page_id_array_[i]);
    if(i == index) {
      key_vector.push_back(key);
      value_vector.push_back(value);
    }
  }
  for(int i = 0; i <= other->GetSize() / 2; i++) {
    other->key_array_[i] = key_vector[i];
    other->page_id_array_[i] = value_vector[i];
  }
  for(int i = other->GetSize() / 2 + 1; i <= other->GetSize(); i++) {
    key_array_[i - other->GetSize() / 2 - 1] = key_vector[i];
    page_id_array_[i - other->GetSize() / 2 - 1] = value_vector[i];
  }
  SetSize((other->GetSize() + 1) / 2);
  other->SetSize(other->GetSize() / 2 + 1);
  return key_vector[other->GetSize()];
}

INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::MoveBack(B_PLUS_TREE_INTERNAL_PAGE_TYPE *other, const KeyType &key) -> KeyType { // move the back of this to the other
  int new_size = (GetSize() + other->GetSize()) >> 1;
  for(int i = other->GetSize() - 1; i >= 0; i--) {
    other->key_array_[i + new_size - other->GetSize()] = i == 0 ? key : other->key_array_[i];
    other->page_id_array_[i + new_size - other->GetSize()] = other->page_id_array_[i];
  }
  for(int i = 0; i < new_size - other->GetSize(); i++) {
    other->key_array_[i] = key_array_[GetSize() - (new_size - other->GetSize()) + i];
    other->page_id_array_[i] = page_id_array_[GetSize() - (new_size - other->GetSize()) + i];
  } 
  SetSize(GetSize() - (new_size - other->GetSize()));
  other->SetSize(new_size);
  return other->key_array_[0];
}

INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::MoveFront(B_PLUS_TREE_INTERNAL_PAGE_TYPE *other, const KeyType &key) -> KeyType {
  int new_size = (GetSize() + other->GetSize()) >> 1;
  for(int i = other->GetSize(); i < new_size; i++) {
    other->key_array_[i] = i == other->GetSize() ? key : key_array_[i - other->GetSize()];
    other->page_id_array_[i] = page_id_array_[i - other->GetSize()];
    //std::cerr << other->page_id_array_[i] << " ";
  }
  int size = GetSize() + other->GetSize() - new_size;
  for(int i = 0; i < size; i++) {
    key_array_[i] = key_array_[i + new_size - other->GetSize()];
    page_id_array_[i] = page_id_array_[i + new_size - other->GetSize()];
    //std::cerr << page_id_array_[i] << " ";
  }
  //std::cerr << std::endl;
  other->SetSize(new_size);
  SetSize(size);
  return key_array_[0];
}

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::Merge(B_PLUS_TREE_INTERNAL_PAGE_TYPE *other, const KeyType &key) {
  for(int i = GetSize(); i < GetSize() + other->GetSize(); i++) {
    key_array_[i] = i == GetSize() ? key : other->key_array_[i - GetSize()];
    page_id_array_[i] = other->page_id_array_[i - GetSize()];
  }
  SetSize(GetSize() + other->GetSize());
}

