#pragma once

#include "common/config.h"

#define MappingType std::pair<KeyType, ValueType>

#define FULL_INDEX_TEMPLATE_ARGUMENTS_DEFN \
  template <typename KeyType, typename ValueType, ssize_t NumTombs = 0>
#define FULL_INDEX_TEMPLATE_ARGUMENTS \
  template <typename KeyType, typename ValueType, ssize_t NumTombs>
#define INDEX_TEMPLATE_ARGUMENTS template <typename KeyType, typename ValueType>

enum class IndexPageType { INVALID_INDEX_PAGE = 0, LEAF_PAGE, INTERNAL_PAGE };

class BPlusTreePage {
 public:
  BPlusTreePage() = delete;
  BPlusTreePage(const BPlusTreePage &other) = delete;
  ~BPlusTreePage() = delete;

  auto IsLeafPage() const -> bool;
  void SetPageType(IndexPageType page_type);

  auto GetSize() const -> int;
  void SetSize(int size);

  auto GetMaxSize() const -> int;
  void SetMaxSize(int max_size);
  auto GetMinSize() const -> int;

 private:
  IndexPageType page_type_;
  int size_;
  int max_size_;
};

class BPlusTreeHeaderPage {
 public:
  BPlusTreeHeaderPage() = delete;
  BPlusTreeHeaderPage(const BPlusTreeHeaderPage &other) = delete;

  page_id_t root_page_id_;
};

