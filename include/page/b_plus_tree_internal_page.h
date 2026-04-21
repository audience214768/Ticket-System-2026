

#pragma once

#include <string>
#include <vector>

#include "common/config.h"
#include "page/b_plus_tree_page.h"

using std::vector;

#define B_PLUS_TREE_INTERNAL_PAGE_TYPE BPlusTreeInternalPage<KeyType, ValueType>
#define INTERNAL_PAGE_HEADER_SIZE 12
#define INTERNAL_PAGE_SLOT_CNT \
  ((DISK_PAGE_SIZE - INTERNAL_PAGE_HEADER_SIZE) / ((int)(sizeof(KeyType) + sizeof(ValueType))))

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
  auto Search(const KeyType &key) const -> int;
  void Insert(const KeyType &key, page_id_t page_id);
  void Delete(const KeyType &key);
  auto MoveHalf(B_PLUS_TREE_INTERNAL_PAGE_TYPE *other, const KeyType &key, const ValueType &value, int index) -> KeyType;
  auto MoveBack(B_PLUS_TREE_INTERNAL_PAGE_TYPE *other, const KeyType &key) -> KeyType;
  auto MoveFront(B_PLUS_TREE_INTERNAL_PAGE_TYPE *other, const KeyType &key) -> KeyType;
  void Merge(B_PLUS_TREE_INTERNAL_PAGE_TYPE *other, const KeyType &key);

  auto ToString() const -> std::string {
    std::string kstr = "(";
    bool first = true;

    // First key of internal page is always invalid
    for (int i = 1; i < GetSize(); i++) {
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
  KeyType key_array_[INTERNAL_PAGE_SLOT_CNT];
  ValueType page_id_array_[INTERNAL_PAGE_SLOT_CNT];
};

