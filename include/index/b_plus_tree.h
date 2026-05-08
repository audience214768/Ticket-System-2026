
#pragma once


#include <cassert>

#include "common/config.h"
#include "page/b_plus_tree_internal_page.h"
#include "page/b_plus_tree_leaf_page.h"
#include "page/page_guard.h"
#include "buffer/buffer_pool_manager.h"
#include "page/b_plus_tree_page.h"
#include "deque/deque.hpp"
#include "shared_ptr/shared_ptr.hpp"

using sjtu::shared_ptr;
using sjtu::make_shared;
using sjtu::deque;


class Context {
 public:
  WritePageGuard write_header_page_;
  ReadPageGuard read_header_page_;
  page_id_t root_page_id_{INVALID_PAGE_ID};

  deque<WritePageGuard> write_set_;
  deque<ReadPageGuard> read_set_;

  WritePageGuard wirte_leaf_page_;
  ReadPageGuard read_leaf_page_;

  auto IsRootPage(page_id_t page_id) -> bool { return page_id == root_page_id_; }
};

#define BPLUSTREE_TYPE BPlusTree<KeyType, ValueType, Compare>

template <typename KeyType, typename ValueType, typename Compare>
class BPlusTree {
  using InternalPage = BPlusTreeInternalPage<KeyType, page_id_t, Compare>;
  using LeafPage = BPlusTreeLeafPage<KeyType, ValueType, Compare>;

 public:
  explicit BPlusTree(size_t file_id, page_id_t header_page_id,shared_ptr<BufferPoolManager> buffer_pool_manager, 
  int leaf_max_size = 0, 
  int internal_max_size = 0);
  
  auto IsEmpty() const -> bool;

  auto Insert(const KeyType &key, const ValueType &value) -> bool;

  auto Remove(const KeyType &key) -> bool;

  void GetValue(const KeyType &key, vector<ValueType> *result);

 private:

  auto FindLeaf(const KeyType &key, Context &ctx, bool is_opt) -> page_id_t;

  auto FindLeafOpt(const KeyType &key, Context &ctx, bool is_get) -> page_id_t;

  auto FindLeafPess(const KeyType &key, Context &ctx) -> page_id_t;

  // member variable
  const size_t file_id_;
  const int leaf_max_size_;
  const int internal_max_size_;
  const page_id_t header_page_id_;
  shared_ptr<BufferPoolManager> bpm_;
  const Compare compare_;
  mutex debug_mutex;
};


INDEX_TEMPLATE_ARGUMENTS
BPLUSTREE_TYPE::BPlusTree(size_t file_id, page_id_t header_page_id, shared_ptr<BufferPoolManager> buffer_pool_manager, int leaf_max_size, 
int internal_max_size)
    : bpm_(buffer_pool_manager),
      file_id_(file_id),
      leaf_max_size_(leaf_max_size),
      internal_max_size_(internal_max_size),
      header_page_id_(header_page_id),
      compare_() {
  //std::cerr << leaf_max_size << " " << internal_max_size << std::endl;
  WritePageGuard guard = bpm_->WritePage(header_page_id_);
  auto header_page = guard.AsMut<BPlusTreeHeaderPage>();
  if (header_page->magic_num_ != 0xDEADBEEF) {
    header_page->root_page_id_ = INVALID_PAGE_ID;
    header_page->magic_num_ = 0xDEADBEEF;
  }
  //std::cerr << LEAF_PAGE_SLOT_CNT << " " << INTERNAL_PAGE_SLOT_CNT << std::endl;
}

INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::IsEmpty() const -> bool {
  ReadPageGuard guard = bpm_->ReadPage(header_page_id_);
  auto root_page = guard.As<BPlusTreeHeaderPage>();
  return root_page->root_page_id_ == INVALID_PAGE_ID;
}

INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::FindLeafOpt(const KeyType &key, Context &ctx, bool is_get) -> page_id_t {
    ctx.read_header_page_ = bpm_->ReadPage(header_page_id_);
    auto header_page = ctx.read_header_page_.As<BPlusTreeHeaderPage>();
    if (header_page->root_page_id_ == INVALID_PAGE_ID) {
      return INVALID_PAGE_ID;
    }
    ctx.root_page_id_ = header_page->root_page_id_;
    ReadPageGuard page = bpm_->ReadPage(header_page->root_page_id_);
    while (!page.As<BPlusTreePage>()->IsLeafPage()) {
      const InternalPage *internal_page = page.As<InternalPage>();
      if (internal_page->GetSize() > internal_page->GetMinSize() &&
          internal_page->GetSize() < internal_page->GetMaxSize() && !is_get) {
        ctx.read_set_.clear();
        ctx.read_header_page_.Drop();
      }
      int index = internal_page->Search(key, compare_);
      ReadPageGuard child_page = bpm_->ReadPage(internal_page->ValueAt(index));
      ctx.read_set_.emplace_back(std::move(page));
      page = std::move(child_page);
    }
    return page.GetPageId();
}

INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::FindLeafPess(const KeyType &key, Context &ctx) -> page_id_t {
    ctx.write_header_page_ = bpm_->WritePage(header_page_id_);
    auto header_page = ctx.write_header_page_.AsMut<BPlusTreeHeaderPage>();
    if (header_page->root_page_id_ == INVALID_PAGE_ID) {
      return INVALID_PAGE_ID;
    }
    ctx.root_page_id_ = header_page->root_page_id_;
    //std::cerr << header_page->root_page_id_ << std::endl;
    WritePageGuard page = bpm_->WritePage(header_page->root_page_id_);
    while (!page.As<BPlusTreePage>()->IsLeafPage()) {
      const InternalPage *internal_page = page.As<InternalPage>();
      if (internal_page->GetSize() > internal_page->GetMinSize() &&
          internal_page->GetSize() < internal_page->GetMaxSize()) {
        ctx.write_set_.clear();
        ctx.write_header_page_.Drop();
      }
      int index = internal_page->Search(key, compare_);
      WritePageGuard child_page = bpm_->WritePage(internal_page->ValueAt(index));
      ctx.write_set_.emplace_back(std::move(page));
      page = std::move(child_page);
    }
    return page.GetPageId();
}

INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::FindLeaf(const KeyType &key, Context &ctx, bool is_opt) -> page_id_t {
  if (is_opt) {
    ctx.read_header_page_ = bpm_->ReadPage(header_page_id_);
    auto header_page = ctx.read_header_page_.As<BPlusTreeHeaderPage>();
    if (header_page->root_page_id_ == INVALID_PAGE_ID) {
      return INVALID_PAGE_ID;
    }
    ctx.root_page_id_ = header_page->root_page_id_;
    //std::cerr << "1 " << header_page->root_page_id_ << std::endl;
    ReadPageGuard page = bpm_->ReadPage(header_page->root_page_id_);
    while (!page.As<BPlusTreePage>()->IsLeafPage()) {
      const InternalPage *internal_page = page.As<InternalPage>();
      int index = internal_page->Search(key, compare_);
      ReadPageGuard child_page = bpm_->ReadPage(internal_page->ValueAt(index));
      ctx.read_set_.emplace_back(std::move(page));
      page = std::move(child_page);
    }
    return page.GetPageId();
  } else {
    ctx.write_header_page_ = bpm_->WritePage(header_page_id_);
    auto header_page = ctx.write_header_page_.AsMut<BPlusTreeHeaderPage>();
    if (header_page->root_page_id_ == INVALID_PAGE_ID) {
      return INVALID_PAGE_ID;
    }
    ctx.root_page_id_ = header_page->root_page_id_;
    //std::cerr << header_page->root_page_id_ << std::endl;
    WritePageGuard page = bpm_->WritePage(header_page->root_page_id_);
    while (!page.As<BPlusTreePage>()->IsLeafPage()) {
      const InternalPage *internal_page = page.As<InternalPage>();
      if (internal_page->GetSize() > internal_page->GetMinSize() &&
          internal_page->GetSize() < internal_page->GetMaxSize()) {
        ctx.write_set_.clear();
        ctx.write_header_page_.Drop();
      }
      int index = internal_page->Search(key, compare_);
      WritePageGuard child_page = bpm_->WritePage(internal_page->ValueAt(index));
      ctx.write_set_.emplace_back(std::move(page));
      page = std::move(child_page);
    }
    return page.GetPageId();
  }
}


INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::GetValue(const KeyType &key, vector<ValueType> *result) {
  Context ctx;
  auto leaf_page_id = FindLeafOpt(key, ctx, true);
  if (ctx.root_page_id_ == INVALID_PAGE_ID) {
    return ;
  }
  //std::cerr << "3 " << leaf_page_id << std::endl;
  ReadPageGuard leaf_page_guard = bpm_->ReadPage(leaf_page_id);
  const LeafPage *leaf_page = leaf_page_guard.As<LeafPage>();
  auto index = leaf_page->Search(key, compare_);
  int i = index + 1;
  while (true) {
    for (; i < leaf_page->GetSize() && leaf_page->KeyAt(i).GetKey() == key.GetKey(); i++) {
      result->push_back(leaf_page->ValueAt(i));
    }
    if (i == leaf_page->GetSize() && leaf_page->GetNextPageId() != INVALID_PAGE_ID) {
      //std::cerr << "next " << leaf_page->GetNextPageId() << std::endl;
      leaf_page_guard = bpm_->ReadPage(leaf_page->GetNextPageId());
      leaf_page = leaf_page_guard.As<LeafPage>();
      //leaf_page->ToString();
      i = 0;
    } else {
      break;
    }
  }
  if (result->empty()) {
    for (auto &guard : ctx.read_set_) {
        const InternalPage *page = guard.As<InternalPage>();
        std::cerr << guard.GetPageId() << " " ;
        page->ToString();
    }
    std::cerr << leaf_page_id << " ";
    leaf_page->ToString();
  }
}


INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Insert(const KeyType &key, const ValueType &value) -> bool {
  //std::cerr << "insert" << std::endl;
  {
    Context ctx;
    auto leaf_page_id = FindLeafOpt(key, ctx, false);
    if (ctx.root_page_id_ != INVALID_PAGE_ID) {
      WritePageGuard leaf_page_guard = bpm_->WritePage(leaf_page_id);
      LeafPage *leaf_page = leaf_page_guard.AsMut<LeafPage>();
      if(leaf_page->GetSize() < leaf_page->GetMaxSize() - 1) {
        return leaf_page->Insert(key, value, compare_);
      }
    }
  }
  //std::cerr << "check1" << std::endl;
  Context ctx;
  auto leaf_page_id = FindLeafPess(key, ctx);
  if (ctx.root_page_id_ == INVALID_PAGE_ID) {
    page_id_t new_page_id = bpm_->NewPage(file_id_);
    WritePageGuard guard = bpm_->WritePage(new_page_id);
    LeafPage *new_page = guard.AsMut<LeafPage>();
    new_page->Init(leaf_max_size_);
    new_page->SetPageType(IndexPageType::LEAF_PAGE);
    if (!new_page->Insert(key, value, compare_)) {
      return false;
    }
    ctx.write_header_page_.AsMut<BPlusTreeHeaderPage>()->root_page_id_ = new_page_id;
    return true;
  }
  WritePageGuard leaf_page_guard = bpm_->WritePage(leaf_page_id);
  LeafPage *leaf_page = leaf_page_guard.AsMut<LeafPage>();
  if(!leaf_page->Insert(key, value, compare_)) {
    return false;
  }
  if (leaf_page->GetSize() < leaf_page->GetMaxSize()) {
    return true;
  }
  auto new_page_id = bpm_->NewPage(file_id_);
  WritePageGuard guard = bpm_->WritePage(new_page_id);
  LeafPage* new_leaf_page = guard.AsMut<LeafPage>();
  new_leaf_page->Init(leaf_max_size_);
  new_leaf_page->SetPageType(IndexPageType::LEAF_PAGE);
  new_leaf_page->MoveHalf(leaf_page);
  new_leaf_page->SetNextPageId(leaf_page->GetNextPageId());
  leaf_page->SetNextPageId(new_page_id);
  KeyType insert_key = new_leaf_page->KeyAt(0);
  page_id_t right_page_id = new_page_id;
  page_id_t left_page_id = leaf_page_id;
  while (!ctx.write_set_.empty()) {
    InternalPage *father_page = ctx.write_set_.back().AsMut<InternalPage>();
    if (father_page->GetSize() < father_page->GetMaxSize()) {
      break;
    }
    new_page_id = bpm_->NewPage(file_id_);
    guard = bpm_->WritePage(new_page_id);
    InternalPage *new_internal_page = guard.AsMut<InternalPage>();
    new_internal_page->Init(internal_max_size_);
    new_internal_page->SetPageType(IndexPageType::INTERNAL_PAGE);
    auto index = father_page->Search(insert_key, compare_);
    KeyType new_key = new_internal_page->MoveHalf(father_page, insert_key, right_page_id, index);
    insert_key = new_key;
    right_page_id = new_page_id;
    left_page_id = ctx.write_set_.back().GetPageId();
    ctx.write_set_.pop_back();
  }
  if (left_page_id == ctx.root_page_id_) {
    auto new_page_id = bpm_->NewPage(file_id_);
    WritePageGuard guard = bpm_->WritePage(new_page_id);
    InternalPage *new_page = guard.AsMut<InternalPage>();
    new_page->Init(internal_max_size_);
    new_page->SetPageType(IndexPageType::INTERNAL_PAGE);
    ctx.write_header_page_.AsMut<BPlusTreeHeaderPage>()->root_page_id_ = new_page_id;
    new_page->SetValueAt(0, left_page_id);
    new_page->SetSize(1);
    new_page->Insert(insert_key, right_page_id, compare_);
    return true;
  }
  InternalPage *father_page = ctx.write_set_.back().AsMut<InternalPage>();
  //unique_lock<mutex> lock(debug_mutex);
  father_page->Insert(insert_key, right_page_id, compare_);
  return true;
}



INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Remove(const KeyType &key) -> bool {
  {
    //std::cerr << "remove" << std::endl;
    Context ctx;
    auto leaf_page_id = FindLeafOpt(key, ctx, false);
    if (ctx.root_page_id_ == INVALID_PAGE_ID) {
      return false;
    }
    //std::cerr << "check" << std::endl;
    WritePageGuard guard = bpm_->WritePage(leaf_page_id);
    LeafPage *leaf_page = guard.AsMut<LeafPage>();
    if (leaf_page->GetSize() > leaf_page->GetMinSize()) {
      return leaf_page->Delete(key, compare_);
    }
  }
  Context ctx;
  auto leaf_page_id = FindLeafPess(key, ctx);
  if (ctx.root_page_id_ == INVALID_PAGE_ID) {
    return false;
  }
  //assert(leaf_page_id != INVALID_PAGE_ID);
  WritePageGuard guard = bpm_->WritePage(leaf_page_id);
  LeafPage *leaf_page = guard.AsMut<LeafPage>();
  if (!leaf_page->Delete(key, compare_)) {
    return false;
  }
  if (leaf_page->GetSize() > leaf_page->GetMinSize()) {
    return true;
  }
  KeyType remove_key;
  if (!ctx.write_set_.empty()) {
    InternalPage *father_page = ctx.write_set_.back().AsMut<InternalPage>();
    auto index = father_page->Search(key, compare_);
    if (index != father_page->GetSize() - 1) {
      WritePageGuard next_page_guard = bpm_->WritePage(father_page->ValueAt(index + 1));
      LeafPage *next_page = next_page_guard.AsMut<LeafPage>();
      if (next_page->GetSize() > next_page->GetMinSize()) {
        father_page->SetKeyAt(index + 1, next_page->MoveFront(leaf_page));
        return true;
      }
    }
    if (index != 0) {
      guard.Drop(); // only right lock
      WritePageGuard next_page_guard = bpm_->WritePage(father_page->ValueAt(index - 1));
      guard = bpm_->WritePage(father_page->ValueAt(index));
      leaf_page = guard.AsMut<LeafPage>();
      LeafPage *next_page = next_page_guard.AsMut<LeafPage>();
      if (next_page->GetSize() > next_page->GetMinSize()) {
        //std::cerr << "borrow last" << std::endl;
        father_page->SetKeyAt(index, next_page->MoveBack(leaf_page));
        return true;
      } else {
        //std::cerr << "merge last" << std::endl;
        remove_key = father_page->KeyAt(index);
        next_page->Merge(leaf_page);
        next_page->SetNextPageId(leaf_page->GetNextPageId());
        guard.Drop();
        bpm_->DeletePage(leaf_page_id);
      }
    } else {
      //std::cerr << "merge next" << std::endl;
      auto next_page_id = father_page->ValueAt(index + 1);
      WritePageGuard next_page_guard = bpm_->WritePage(next_page_id);
      LeafPage *next_page = next_page_guard.AsMut<LeafPage>();
      remove_key = father_page->KeyAt(index + 1);
      leaf_page->Merge(next_page);
      leaf_page->SetNextPageId(next_page->GetNextPageId());
      next_page_guard.Drop();
      bpm_->DeletePage(next_page_id);
    }
  }
  while (!ctx.write_set_.empty()) {
    guard = std::move(ctx.write_set_.back());
    ctx.write_set_.pop_back();
    InternalPage *internal_page = guard.AsMut<InternalPage>();
    auto page_id = guard.GetPageId();
    internal_page->Delete(remove_key, compare_);
    if (internal_page->GetSize() > internal_page->GetMinSize() || ctx.write_set_.empty()) {
      break;
    }
    InternalPage *father_page = ctx.write_set_.back().AsMut<InternalPage>();
    auto index = father_page->Search(remove_key, compare_);
    if (index != father_page->GetSize() - 1) {
      WritePageGuard next_page_guard = bpm_->WritePage(father_page->ValueAt(index + 1));
      InternalPage *next_page = next_page_guard.AsMut<InternalPage>();
      if (next_page->GetSize() > next_page->GetMinSize()) {
        //std::cerr << "borrow next" << std::endl;
        father_page->SetKeyAt(index + 1, next_page->MoveFront(internal_page, father_page->KeyAt(index + 1)));
        break;
      }
    }
    if (index != 0) {
      guard.Drop();
      WritePageGuard next_page_guard = bpm_->WritePage(father_page->ValueAt(index - 1));
      guard = bpm_->WritePage(father_page->ValueAt(index));
      internal_page = guard.AsMut<InternalPage>();
      InternalPage *next_page = next_page_guard.AsMut<InternalPage>();
      if (next_page->GetSize() > next_page->GetMinSize()) {
        //std::cerr << "borrow last" << std::endl;
        father_page->SetKeyAt(index, next_page->MoveBack(internal_page, father_page->KeyAt(index)));
        break;
      } else {
        //std::cerr << "merge last" << std::endl;
        remove_key = father_page->KeyAt(index);
        next_page->Merge(internal_page, remove_key);
        guard.Drop();
        bpm_->DeletePage(page_id);
      }
    } else {
      //std::cerr << "merge next" << std::endl;
      auto next_page_id = father_page->ValueAt(index + 1);
      WritePageGuard next_page_guard = bpm_->WritePage(next_page_id);
      InternalPage *next_page = next_page_guard.AsMut<InternalPage>();
      remove_key = father_page->KeyAt(index + 1);
      internal_page->Merge(next_page, remove_key);
      //internal_page->ToString();
      next_page_guard.Drop();
      bpm_->DeletePage(next_page_id);
    }
  }
  if(guard.GetPageId() == ctx.root_page_id_) {
    if(guard.As<BPlusTreePage>()->IsLeafPage()) {
      LeafPage *root_page = guard.AsMut<LeafPage>();
      if(root_page->GetSize() == 0) {
        BPlusTreeHeaderPage *header_page = ctx.write_header_page_.AsMut<BPlusTreeHeaderPage>();
        header_page->root_page_id_ = INVALID_PAGE_ID;
        guard.Drop();
        bpm_->DeletePage(ctx.root_page_id_);
      }
    } else {
      InternalPage *root_page = guard.AsMut<InternalPage>();
      if(root_page->GetSize() == 1) {
        auto header_page = ctx.write_header_page_.AsMut<BPlusTreeHeaderPage>();
        header_page->root_page_id_ = root_page->ValueAt(0);
        guard.Drop();
        bpm_->DeletePage(ctx.root_page_id_);
      }
    }
  }
  return true;
}



