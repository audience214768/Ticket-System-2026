#pragma once
#include "page/b_plus_tree_leaf_page.h"

FULL_INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::Init(int max_size) {
  SetMaxSize(max_size);
  SetSize(0);
  num_tombstones_ = 0;
  SetPageType(IndexPageType::LEAF_PAGE);
  SetNextPageId(INVALID_PAGE_ID);
}

FULL_INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::GetTombstones() const -> std::vector<KeyType> {
  std::vector<KeyType> delete_key;
  delete_key.reserve(num_tombstones_);
  for(size_t i = 0; i < num_tombstones_; i++) {
    delete_key.push_back(key_array_[tombstones_[i]]);
  }
  return delete_key;
}

FULL_INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::GetTombCount() const -> int {
  return num_tombstones_;
}

FULL_INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::GetNextPageId() const -> page_id_t {
  return next_page_id_;
}

FULL_INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::SetNextPageId(page_id_t next_page_id) {
  next_page_id_ = next_page_id;
}

FULL_INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::KeyAt(int index) const -> const KeyType &{
  return key_array_[index];
}

FULL_INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::isTomb(int index) const -> bool {
  //std::cerr << num_tombstones_ << " " << NumTombs << std::endl;
  for(size_t i = 0; i < num_tombstones_; i++) {
    if(tombstones_[i] == static_cast<size_t>(index)) {
      return true;
    }
  }
  return false;
}

FULL_INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::setTomb(int index) {
  //std::cerr << num_tombstones_ << " " << NumTombs << std::endl;
  tombstones_[num_tombstones_++] = index;
  if(num_tombstones_ == static_cast<size_t>(NumTombs) || num_tombstones_ > static_cast<size_t>(GetSize() - GetMinSize())) {
    FlushTomb();
  }
}

FULL_INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::ValueAt(int index) const -> const ValueType &{
  return rid_array_[index];
}

FULL_INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::DeleteTomb(int index) {
  //std::cerr << num_tombstones_ << std::endl;
  size_t i;
  for(i = 0; i < num_tombstones_; i++) {
    if(tombstones_[i] == static_cast<size_t>(index)) {
      break;
    }
  }
  for(;i < num_tombstones_; i++) {
    tombstones_[i] = tombstones_[i + 1];
  }
  num_tombstones_--;
}

FULL_INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::Search(const KeyType &key) const -> int {
  int l = 0, r = GetSize();
  int ans = l - 1;
  while(l < r) {
    int m = (l + r) >> 1;
    //std::cerr << key << " " << KeyAt(m) << std::endl;
    if(key >= key_array_[m]) {
      ans = m;
      //std::cerr << l << " " << r << " " << ans << std::endl;
      l = m + 1;
    } else {
      r = m;
    }
  }
  return ans;
}

FULL_INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::Insert( const KeyType &key, const ValueType &value) -> bool{ //true mean the key exist
  //std::cerr << "begin insert" << std::endl;
  int index = Search(key);
  //std::cerr << index << " " << GetSize() << std::endl;
  if(index != -1 && key == key_array_[index] && !isTomb(index)) {
    return true;
  }
  for(size_t i = 0; i < num_tombstones_; i++) {
    if(tombstones_[i] >= static_cast<size_t>(index + 1)) {
      tombstones_[i]++;
    }
  }
  for(int i = GetSize(); i > index + 1; i--) {
    key_array_[i] = key_array_[i - 1];
    rid_array_[i] = rid_array_[i - 1];
  }
  key_array_[index + 1] = key;
  rid_array_[index + 1] = value;
  SetSize(GetSize() + 1);
  if(GetSize() == GetMaxSize()) {
    FlushTomb();
  }
  return false;
} 

FULL_INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::Delete(const KeyType &key) -> bool {
  auto index = Search(key);
  if(index == -1 || key_array_[index] != key || isTomb(index) == true) {
    return false;
  }
  if(NumTombs != 0) {
    setTomb(index);
  } else {
    for(int i = index; i < GetSize() - 1; i++) {
      key_array_[i] = key_array_[i + 1];
      rid_array_[i] = rid_array_[i + 1];
    }
    SetSize(GetSize() - 1);
  }
  if(GetSize() < GetMinSize()) {
    return true;
  }
  return false;
}

FULL_INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::MoveHalf(B_PLUS_TREE_LEAF_PAGE_TYPE *other) {
  for(int i = other->GetSize() / 2; i < other->GetSize(); i++) {
    key_array_[i - other->GetSize() / 2] = other->key_array_[i];
    rid_array_[i - other->GetSize() / 2] = other->rid_array_[i];
  }
  SetSize(other->GetSize() - other->GetSize() / 2);
  other->SetSize(other->GetSize() / 2);
}

FULL_INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::FlushTomb() {
  if(num_tombstones_ == 0) {
    return ;
  }
  std::vector<KeyType> key;
  std::vector<ValueType> value;
  key.reserve(GetSize() - num_tombstones_);
  value.reserve(GetSize() - num_tombstones_);
  for(int i = 0; i < GetSize(); i++) {
    if(!isTomb(i)) {
      key.push_back(key_array_[i]);
      value.push_back(rid_array_[i]);
    }
  }
  SetSize(GetSize() - num_tombstones_);
  num_tombstones_ = 0;
  for(int i = 0; i < GetSize(); i++) {
    key_array_[i] = key[i];
    rid_array_[i] = value[i];
  }
}
FULL_INDEX_TEMPLATE_ARGUMENTS
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

FULL_INDEX_TEMPLATE_ARGUMENTS
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

FULL_INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::Merge(B_PLUS_TREE_LEAF_PAGE_TYPE *other) {
  for(int i = GetSize(); i < GetSize() + other->GetSize(); i++) {
    key_array_[i] = other->key_array_[i - GetSize()];
    rid_array_[i] = other->rid_array_[i - GetSize()];
  }
  SetSize(GetSize() + other->GetSize());
}