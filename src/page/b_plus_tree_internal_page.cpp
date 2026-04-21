#include "common/config.h"
#include "page/b_plus_tree_page.h"
#include "page/b_plus_tree_internal_page.h"

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::Init(int max_size) {
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
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::Search(const KeyType &key) const -> int {
  int l = 1, r = GetSize();
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

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::Insert(const KeyType &key, page_id_t page_id) {
  int index = Search(key);
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
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::Delete(const KeyType &key) {
  int index = Search(key);
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

