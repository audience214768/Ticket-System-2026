#pragma once

#include "common/config.h"
#include "buffer/buffer_pool_manager.h"
#include "utils/config.h"
#include "shared_ptr/shared_ptr.hpp"
#include "utils/types.h"
#include "index/b_plus_tree.h"
#include "type/type.hpp"

using sjtu::shared_ptr;

struct UserListPage {
    static constexpr size_t Num = (DISK_PAGE_SIZE - sizeof(size_t)) / sizeof(UserRecord);
    size_t last_index = 0;
    UserRecord data[Num];
};

struct UserListHeaderPage {
    size_t last_page;
    bool have_user;
    size_t magic_num;
};


class UserManager {
  private:
    bool have_user_;
    shared_ptr<BufferPoolManager> bpm_;
    size_t user_list_fid_;
    page_id_t last_page_id_;
    page_id_t header_page_id_;
    BPlusTree<ComposedKey<USER_NAME_LEN + 1>, size_t, Compare> user_name_index_;
    static constexpr int LOG_HASH_SIZE = 1 << 10;
    UserNode *log_table[1024] = {nullptr};
    auto hash_djb2(const char *str) -> unsigned long {
        unsigned long hash = 5381;
        int c;
        while ((c = *str++)) {
            hash = ((hash << 5) + hash) + c;
        }
        return hash;
    }
  public:
    UserManager(shared_ptr<BufferPoolManager> bpm, size_t user_list_fid, size_t user_name_index_fid);
    auto haveUser() -> bool {
      return have_user_;
    }
    auto getUserIndex(const char *user_name) -> size_t;
    auto addUser(const char *user_name, const UserRecord &record) -> bool;
    void queryProfile(const char *user_namae, UserRecord &record);
    void modifyProfile(const char *user_namae, const UserRecord &record);
    void login(const char *user_name, const char *pwd, UserNode **ptr);
    void logout(const char *user_name, UserNode **ptr);
    auto getLoggedUser(const char *user_name) -> UserNode **;
    void clearLoginTable();
    void Reset();
};