#include "manager/user_manager.h"
#include "buffer/buffer_pool_manager.h"
#include "common/config.h"
#include "utils/config.h"
#include "utils/err.h"
#include "utils/types.h"
#include "page/page_guard.h"
#include "shared_ptr/shared_ptr.hpp"
#include "type/type.hpp"
#include "utils/hash.h"

using sjtu::shared_ptr;

UserManager::UserManager(shared_ptr<BufferPoolManager> bpm, size_t user_list_fid, size_t user_name_index_fid)
  : bpm_(std::move(bpm)), 
    user_list_fid_(user_list_fid),
    header_page_id_(user_list_fid << FILE_BIT),
    user_name_index_(user_name_index_fid, user_name_index_fid << FILE_BIT, bpm_) {
    //std::cerr << "initialize user_manager" << std::endl;
    WritePageGuard header_page_guard = bpm_->WritePage(header_page_id_);
    //std::cerr << *header_page_guard.AsMut<size_t>() << std::endl;
    //size_t *tmp = header_page_guard.AsMut<size_t>();
    UserListHeaderPage *header_page = header_page_guard.AsMut<UserListHeaderPage>();
    //std::cerr << header_page->magic_num << " " << 0xDEADBEEF << std::endl;
    //header_page->magic_num = 0;
    //std::cerr << header_page->magic_num << std::endl;
    if (header_page->magic_num != 0xDEADBEEF) {
        header_page->magic_num = 0xDEADBEEF;
        header_page->last_page = user_list_fid_ << FILE_BIT | 1;
        last_page_id_ = header_page->last_page;
        have_user_ = false;
        //std::cerr << last_page_id << std::endl;
    } else {
        last_page_id_ = header_page->last_page;
        have_user_ = header_page->have_user;
        //std::cerr << last_page_id << std::endl;
    }
}

auto UserManager::addUser(const char *user_name, const UserRecord &record) -> bool {
    if (!have_user_) {
        WritePageGuard header_page_guard = bpm_->WritePage(header_page_id_);
        header_page_guard.AsMut<UserListHeaderPage>()->have_user = true;
        have_user_ = true;
    }
    //std::cerr << last_page_id << std::endl;
    WritePageGuard page_guard = bpm_->WritePage(last_page_id_);
    //std::cerr << std::endl;
    UserListPage *page = page_guard.AsMut<UserListPage>();
    ComposedKey<21> key;
    strcpy(key.fixed_key.key, user_name);
    key.rid = 0;
    if (!user_name_index_.Insert(key, page->last_index + last_page_id_ * UserListPage::Num)) {
        return false;
    }
    //std::cerr << page->last_index << std::endl;
    page->data[page->last_index++] = std::move(record);
    if (page->last_index == page->Num) {
        WritePageGuard header_page_guard = bpm_->WritePage(header_page_id_);
        UserListHeaderPage *header_page = header_page_guard.AsMut<UserListHeaderPage>();
        header_page->last_page++;
        last_page_id_++;
        WritePageGuard new_page_guard = bpm_->WritePage(last_page_id_);
        page = new_page_guard.AsMut<UserListPage>();
        page->last_index = 0;
    }
    
    //std::cerr << "check" << std::endl;
    return true;
    //std::cerr << "check" << std::endl;
}

auto UserManager::getUserIndex(const char *user_name) -> size_t {
    ComposedKey<USER_NAME_LEN + 1> key;
    strcpy(key.fixed_key.key, user_name);
    key.rid = RID_MIN;
    vector<size_t> result;
    user_name_index_.GetValue(key, &result);
    //std::cerr << user_name << " " << result.size() << std::endl;
    if (result.size() == 0) {
        throw Exception("the user is no exist");
    }
    if (result.size() > 1) {
        throw Exception("the user is duplicated");
    }
    return result[0];
}

auto UserManager::getLoggedUser(const char *user_name) -> UserNode ** {
    size_t hash = hash_djb2(user_name) & (LOG_HASH_SIZE - 1);
    UserNode **ptr = &log_table[hash];
    //std::cerr << &log_table[hash] << std::endl;
    bool find = false;
    while (*ptr != nullptr) {
        if (strcmp((*ptr)->user_name, user_name) == 0) {
            find = true;
            break;
        }
        ptr = &(*ptr)->next;
    }
    return ptr;
}

void UserManager::login(const char *user_name, const char *pwd, UserNode **ptr) {
    size_t index = getUserIndex(user_name);
    //std::cerr << index << std::endl;
    ReadPageGuard page_guard = bpm_->ReadPage(index / UserListPage::Num);
    UserRecord record = page_guard.As<UserListPage>()->data[index % UserListPage::Num];
    //std::cerr << index / UserListPage::Num << " " << index % UserListPage::Num << std::endl;
    //std::cerr << record.password << std::endl;
    if (strcmp(record.password, pwd) != 0) {
        throw Exception("login: the password is wrong");
    }
    UserNode *newnode = new UserNode;
    strcpy(newnode->user_name, user_name);
    newnode->next = nullptr;
    newnode->priv = record.privilege;
    *ptr = newnode;
}

void UserManager::logout(const char *user_name, UserNode **ptr) {
    //getUserIndex(user_name);
    *ptr = (*ptr)->next;
}

void UserManager::queryProfile(const char *user_name, UserRecord &record) {
    size_t index = getUserIndex(user_name);
    //std::cerr << index << std::endl;
    ReadPageGuard page_guard = bpm_->ReadPage(index / UserListPage::Num);
    record = page_guard.As<UserListPage>()->data[index % UserListPage::Num];
}

void UserManager::modifyProfile(const char *user_name, const UserRecord &record) {
    size_t index = getUserIndex(user_name);
    WritePageGuard page_guard = bpm_->WritePage(index / UserListPage::Num);
    page_guard.AsMut<UserListPage>()->data[index % UserListPage::Num] = record;
}

void UserManager::clearLoginTable() {
    for (int i = 0; i < 1024; i++) {
        UserNode *node = log_table[i];
        while (node) {
            UserNode *next = node->next;
            delete node;
            node = next;
        }
        log_table[i] = nullptr;
    }
}


void UserManager::Reset() {
    clearLoginTable();
    have_user_ = false;
    user_name_index_.Reset();
    WritePageGuard guard = bpm_->WritePage(header_page_id_);
    auto *header = guard.AsMut<UserListHeaderPage>();
    header->magic_num = 0xDEADBEEF;
    header->last_page = user_list_fid_ << FILE_BIT | 1;
    last_page_id_ = header->last_page;
}