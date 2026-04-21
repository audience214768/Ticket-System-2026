

#include "index/b_plus_tree.h"
#include <netdb.h>
#include <optional>
#include "buffer/buffer_pool_manager.h"
#include "common/config.h"
#include "page/b_plus_tree_page.h"
#include "page/b_plus_tree_page.h"


FULL_INDEX_TEMPLATE_ARGUMENTS
BPLUSTREE_TYPE::BPlusTree(size_t file_id, page_id_t header_page_id, BufferPoolManager *buffer_pool_manager, int leaf_max_size, 
int internal_max_size)
    : bpm_(std::make_shared<BufferPoolManager>(buffer_pool_manager)),
      file_id_(file_id),
      leaf_max_size_(leaf_max_size),
      internal_max_size_(internal_max_size),
      header_page_id_(header_page_id) {
  WritePageGuard guard = bpm_->WritePage(header_page_id_);
  auto root_page = guard.AsMut<BPlusTreeHeaderPage>();
  root_page->root_page_id_ = INVALID_PAGE_ID;
}

FULL_INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::IsEmpty() const -> bool {
  ReadPageGuard guard = bpm_->ReadPage(header_page_id_);
  auto root_page = guard.As<BPlusTreeHeaderPage>();
  return root_page->root_page_id_ == INVALID_PAGE_ID;
}

FULL_INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::FindLeaf(const KeyType &key, Context &ctx, bool is_opt) -> page_id_t {
  //std::cerr << "begin to find the leaf" << std::endl;
  if (is_opt) {
    ctx.read_header_page_ = bpm_->ReadPage(header_page_id_);
    auto root_page = ctx.read_header_page_->As<BPlusTreeHeaderPage>();
    ctx.root_page_id_ = root_page->root_page_id_;
    // std::cerr << "check" << std::endl;
    ReadPageGuard page = bpm_->ReadPage(root_page->root_page_id_);
    // std::cerr << "check out" << std::endl;
    // std::cerr << page.AsMut<BPlusTreePage>()->IsLeafPage() << std::endl;
    while (!page.As<BPlusTreePage>()->IsLeafPage()) {
      // std::cerr << "check" << std::endl;
      const InternalPage *internal_page = page.As<InternalPage>();
      if (internal_page->GetSize() > internal_page->GetMinSize() + 1 &&
          internal_page->GetSize() < internal_page->GetMaxSize()) {
        ctx.read_set_.clear();
      }
      int index = internal_page->Search(key);
      // std::cerr << internal_page->ToString() << " " << key << " " << index << std::endl;
      ReadPageGuard child_page = bpm_->ReadPage(internal_page->ValueAt(index));
      ctx.read_set_.push_back(std::move(page));
      page = std::move(child_page);
    }
    return page.GetPageId();
  } else {
    ctx.write_header_page_ = bpm_->WritePage(header_page_id_);
    auto root_page = ctx.write_header_page_->AsMut<BPlusTreeHeaderPage>();
    ctx.root_page_id_ = root_page->root_page_id_;
    //std::cerr << "check" << std::endl;
    WritePageGuard page = bpm_->WritePage(root_page->root_page_id_);
    //std::cerr << "check out" << std::endl;
    // std::cerr << page.AsMut<BPlusTreePage>()->IsLeafPage() << std::endl;
    while (!page.As<BPlusTreePage>()->IsLeafPage()) {
      // std::cerr << "check" << std::endl;
      const InternalPage *internal_page = page.As<InternalPage>();
      if (internal_page->GetSize() > internal_page->GetMinSize() + 1 &&
          internal_page->GetSize() < internal_page->GetMaxSize()) {
        ctx.write_set_.clear();
      }
      int index = internal_page->Search(key);
      // std::cerr << internal_page->ToString() << " " << key << " " << index << std::endl;
      WritePageGuard child_page = bpm_->WritePage(internal_page->ValueAt(index));
      ctx.write_set_.push_back(std::move(page));
      page = std::move(child_page);
    }
    return page.GetPageId();
  }
}


FULL_INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::GetValue(const KeyType &key, std::vector<ValueType> *result) -> bool {
  // Declaration of context instance. Using the Context is not necessary but advised.
  Context ctx;
  if(IsEmpty()) {
    return false;
  }
  auto leaf_page_id = FindLeaf(key, ctx, true);
  ReadPageGuard leaf_page_guard = bpm_->ReadPage(leaf_page_id);
  const LeafPage *leaf_page = leaf_page_guard.As<LeafPage>();
  auto index = leaf_page->Search(key);
  //std::cerr << index << std::endl;
  if(index != -1 && key == leaf_page->KeyAt(index) && !leaf_page->isTomb(index)) {
    result->push_back(leaf_page->ValueAt(index));
    return true;
  } else {
    return false;
  }
}


FULL_INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Insert(const KeyType &key, const ValueType &value) -> bool {
  // Declaration of context instance. Using the Context is not necessary but advised.
  //std::cerr << "begin to insert" << std::endl;
  //LOG_DEBUG("Insert %d", static_cast<int>(key.GetAsInteger()));
  {
    WritePageGuard header_page_guard = bpm_->WritePage(header_page_id_);
    auto header_page = header_page_guard.AsMut<BPlusTreeHeaderPage>();
    if (header_page->root_page_id_ == INVALID_PAGE_ID) {
      page_id_t new_page_id = bpm_->NewPage(file_id_);
      WritePageGuard guard = bpm_->WritePage(new_page_id);
      LeafPage *new_page = guard.AsMut<LeafPage>();
      new_page->Init(leaf_max_size_);
      new_page->SetPageType(IndexPageType::LEAF_PAGE);
      //std::cerr << "first : " << key << std::endl;
      new_page->Insert(key, value);
      // std::cerr << new_page->ToString() << std::endl;
      header_page->root_page_id_ = new_page_id;
      return true;
    }
  }
  //std::cerr << "not empty" << std::endl;
  {
    Context ctx;
    auto leaf_page_id = FindLeaf(key, ctx, true);
    WritePageGuard leaf_page_guard = bpm_->WritePage(leaf_page_id);
    LeafPage *leaf_page = leaf_page_guard.AsMut<LeafPage>();
    if(leaf_page->GetSize() - leaf_page->GetTombCount() < leaf_page->GetMaxSize() - 1) {
      //std::cerr << "no full insert : " << key << " " << leaf_page->ToString() << " " << ctx.read_set_.size() << std::endl;
      if(leaf_page->Insert(key, value)) {
        return false;
      } else {
        //std::cerr << leaf_page->ToString() << std::endl;
        //std::cerr << "insert " << key << std::endl;
        return true;
      }
    }
  }
  Context ctx;
  auto leaf_page_id = FindLeaf(key, ctx, false);
  WritePageGuard leaf_page_guard = bpm_->WritePage(leaf_page_id);
  LeafPage *leaf_page = leaf_page_guard.AsMut<LeafPage>();
  //std::cerr << "before full insert : " << key << " " << leaf_page->ToString() << std::endl;
  if(leaf_page->Insert(key, value)) {
    return false;
  }
  //std::cerr << leaf_page->ToString() << std::endl;
  //std::cerr << "insert " << key << std::endl;
  auto new_page_id = bpm_->NewPage(file_id_);
  //std::cerr << "creat " << new_page_id << std::endl;
  WritePageGuard guard = bpm_->WritePage(new_page_id);
  LeafPage* new_leaf_page = guard.AsMut<LeafPage>();
  //std::cerr << leaf_page->ToString() << std::endl;
  new_leaf_page->Init(leaf_max_size_);
  new_leaf_page->SetPageType(IndexPageType::LEAF_PAGE);
  new_leaf_page->MoveHalf(leaf_page);
  //std::cerr << "after full insert : " << key << " " << leaf_page->ToString() << " " << new_leaf_page->ToString() << std::endl;
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
    // std::cerr << "creat" << new_page_id << std::endl;
    guard = bpm_->WritePage(new_page_id);
    InternalPage *new_internal_page = guard.AsMut<InternalPage>();
    new_internal_page->Init(internal_max_size_);
    new_internal_page->SetPageType(IndexPageType::INTERNAL_PAGE);
    auto index = father_page->Search(insert_key);
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
    ctx.write_header_page_->AsMut<BPlusTreeHeaderPage>()->root_page_id_ = new_page_id;
    new_page->SetValueAt(0, left_page_id);
    new_page->SetSize(1);
    new_page->Insert(insert_key, right_page_id);
    return true;
  }
  InternalPage *father_page = ctx.write_set_.back().AsMut<InternalPage>();
  father_page->Insert(insert_key, right_page_id);
  //std::cerr << "end insert" << std::endl;
  return true;
}



FULL_INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::Remove(const KeyType &key) {
  //LOG_DEBUG("Remove %d", static_cast<int>(key.GetAsInteger()));
  // Declaration of context instance.
  if(IsEmpty()) {
    return ;
  }
  {
    Context ctx;
    auto leaf_page_id = FindLeaf(key, ctx, true);
    //std::cerr << "check" << std::endl;
    WritePageGuard guard = bpm_->WritePage(leaf_page_id);
    //std::cerr << "delete" << key << std::endl;
    //std::cerr << "check out" << std::endl;
    LeafPage *leaf_page = guard.AsMut<LeafPage>();
    if (!leaf_page->Delete(key)) {
      //std::cerr << leaf_page->ToString() << std::endl;
      //std::cerr << "delete but not merge " << key << std::endl; 
      return ;
    } else {
      //std::cerr << leaf_page->ToString() << std::endl;
    }
  }
  
  Context ctx;
  auto leaf_page_id = FindLeaf(key, ctx, false);
  //std::cerr << "delete" << key << " " << ctx.write_set_.empty() << std::endl;
  //std::cerr << "check" << std::endl;
  WritePageGuard guard = bpm_->WritePage(leaf_page_id);
  //std::cerr << "check out" << std::endl;
  LeafPage *leaf_page = guard.AsMut<LeafPage>();
  KeyType remove_key;
  if (!ctx.write_set_.empty()) {
    InternalPage *father_page = ctx.write_set_.back().AsMut<InternalPage>();
    auto index = father_page->Search(key);
    if (index != father_page->GetSize() - 1) {
      //std::cerr << "check1 " << father_page->ToString() << key << std::endl;
      WritePageGuard next_page_guard = bpm_->WritePage(father_page->ValueAt(index + 1));
      //std::cerr << "check out" << std::endl;
      LeafPage *next_page = next_page_guard.AsMut<LeafPage>();
      next_page->FlushTomb();
      if (next_page->GetSize() > next_page->GetMinSize()) {
        //std::cerr << "get one from right" << std::endl;
        //std::cerr << leaf_page->ToString() << " " << next_page->ToString() << std::endl;
        father_page->SetKeyAt(index + 1, next_page->MoveFront(leaf_page));
        //std::cerr << leaf_page->ToString() << " " << next_page->ToString() << std::endl;
        return;
      }
    }
    if (index != 0) {
      //std::cerr << "check2 " << father_page->ToString() << key << std::endl;
      WritePageGuard next_page_guard = bpm_->WritePage(father_page->ValueAt(index - 1));
      //std::cerr << "check out" << std::endl;
      LeafPage *next_page = next_page_guard.AsMut<LeafPage>();
      next_page->FlushTomb();
      if (next_page->GetSize() > next_page->GetMinSize()) {
        //std::cerr << "get one from left" << std::endl;
        //std::cerr << next_page->ToString() << " " << leaf_page->ToString() << std::endl;
        father_page->SetKeyAt(index, next_page->MoveBack(leaf_page));
        //std::cerr << next_page->ToString() << " " << leaf_page->ToString() << std::endl;
        return ;
      } else {
        //std::cerr << "merge with left " << std::endl;
        // std::cerr << father_page->ToString() << std::endl;
        remove_key = father_page->KeyAt(index);
        // std::cerr << remove_key << std::endl;
        //std::cerr << next_page->ToString() << " " << leaf_page->ToString() << std::endl;
        next_page->Merge(leaf_page);
        //std::cerr << next_page->ToString() << std::endl;
        next_page->SetNextPageId(leaf_page->GetNextPageId());
        bpm_->DeletePage(leaf_page_id);
        // std::cerr << "check" << std::endl;
      }
    } else {
      //std::cerr << "merge with right" << std::endl;
      auto next_page_id = father_page->ValueAt(index + 1);
      WritePageGuard next_page_guard = bpm_->WritePage(next_page_id);
      LeafPage *next_page = next_page_guard.AsMut<LeafPage>();
      remove_key = father_page->KeyAt(index + 1);
      // std::cerr << remove_key << std::endl;
      next_page->FlushTomb();
      //std::cerr << leaf_page->ToString() << " " << next_page->ToString() << std::endl;
      leaf_page->Merge(next_page);
      //std::cerr << leaf_page->ToString() << std::endl;
      leaf_page->SetNextPageId(next_page->GetNextPageId());
      bpm_->DeletePage(next_page_id);
    }
    bpm_->DeletePage(leaf_page_id);
  }
  while (!ctx.write_set_.empty()) {
    // std::cerr << "check" << std::endl;
    guard = std::move(ctx.write_set_.back());
    ctx.write_set_.pop_back();
    InternalPage *internal_page = guard.AsMut<InternalPage>();
    auto page_id = guard.GetPageId();
    internal_page->Delete(remove_key);
    if (internal_page->GetSize() > internal_page->GetMinSize() || ctx.write_set_.empty()) {
      break;
    }
    // std::cerr << page_id << " too small " << std::endl;
    InternalPage *father_page = ctx.write_set_.back().AsMut<InternalPage>();
    // std::cerr << father_page->ToString() << std::endl;
    auto index = father_page->Search(remove_key);
    // std::cerr << index << std::endl;
    if (index != father_page->GetSize() - 1) {
      //std::cerr << "check1 " << father_page->ToString() << std::endl;
      WritePageGuard next_page_guard = bpm_->WritePage(father_page->ValueAt(index + 1));
      //std::cerr << "check out" << std::endl;
      InternalPage *next_page = next_page_guard.AsMut<InternalPage>();
      if (next_page->GetSize() > next_page->GetMinSize() + 1) {
        father_page->SetKeyAt(index + 1, next_page->MoveFront(internal_page, father_page->KeyAt(index + 1)));
        // std::cerr << father_page->ToString() << std::endl;
        //std::cerr << internal_page->ToString() << " " << next_page->ToString() << std::endl;
        //std::cerr << "get one from right" << std::endl;
        break;
      }
    }
    if (index != 0) {
      //std::cerr << "check2 " << father_page->ToString() << std::endl;
      WritePageGuard next_page_guard = bpm_->WritePage(father_page->ValueAt(index - 1));
      //std::cerr << "check out" << std::endl;
      InternalPage *next_page = next_page_guard.AsMut<InternalPage>();
      // std::cerr << next_page->ToString() << std::endl;
      if (next_page->GetSize() > next_page->GetMinSize() + 1) {
        //std::cerr << next_page->ToString() << " " << internal_page->ToString() << std::endl;
        father_page->SetKeyAt(index, next_page->MoveBack(internal_page, father_page->KeyAt(index)));
        // std::cerr << father_page->ToString() << std::endl;
        //std::cerr << next_page->ToString() << " " << internal_page->ToString() << std::endl;
        //std::cerr << "get one from left" << std::endl;
        break;
      } else {
        //std::cerr << "merge with left" << std::endl;
        remove_key = father_page->KeyAt(index);
        next_page->Merge(internal_page, remove_key);
        bpm_->DeletePage(page_id);
        // std::cerr << "check" << std::endl;
      }
    } else {
      //std::cerr << "merge with right" << std::endl;
      auto next_page_id = father_page->ValueAt(index + 1);
      WritePageGuard next_page_guard = bpm_->WritePage(next_page_id);
      InternalPage *next_page = next_page_guard.AsMut<InternalPage>();
      remove_key = father_page->KeyAt(index + 1);
      internal_page->Merge(next_page, remove_key);
      bpm_->DeletePage(next_page_id);
    }
  }
  if(guard.GetPageId() == ctx.root_page_id_) {
    //std::cerr << "root" << std::endl;
    if(guard.As<BPlusTreePage>()->IsLeafPage()) {
      LeafPage *root_page = guard.AsMut<LeafPage>();
      if(root_page->GetSize() == 0) {
        BPlusTreeHeaderPage *header_page = ctx.write_header_page_->AsMut<BPlusTreeHeaderPage>();
        //std::cerr << "null tree" << std::endl;
        header_page->root_page_id_ = INVALID_PAGE_ID;
        bpm_->DeletePage(guard.GetPageId());
      }
    } else {
      InternalPage *root_page = guard.AsMut<InternalPage>();
      if(root_page->GetSize() == 1) {
        auto header_page = ctx.write_header_page_->AsMut<BPlusTreeHeaderPage>();
        header_page->root_page_id_ = root_page->ValueAt(0);
        bpm_->DeletePage(guard.GetPageId());
      }
    }
  }
}

/**
 * @return Page id of the root of this tree
 *
 * You may want to implement this while implementing Task #3.
 */
FULL_INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::GetRootPageId() -> page_id_t {
  WritePageGuard guard = bpm_->WritePage(header_page_id_);
  auto root_page = guard.AsMut<BPlusTreeHeaderPage>();
  return root_page->root_page_id_;
}

template class BPlusTree<int, page_id_t>;

