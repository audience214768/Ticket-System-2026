#pragma once

#include "buffer/buffer_pool_manager.h"
#include "common/config.h"
#include "utils/types.h"
#include "index/b_plus_tree.h"
#include "shared_ptr/shared_ptr.hpp"
#include "type/type.hpp"
#include "vector/vector.hpp"

using sjtu::shared_ptr;
using sjtu::vector;

struct OrderPage {
    static constexpr size_t Num = (DISK_PAGE_SIZE - sizeof(size_t)) / sizeof(OrderRecord);
    size_t last_index = 0;
    OrderRecord data[Num];
};

struct OrderPageHeader {
    size_t last_page;
    size_t magic_num;
};

class OrderManager {
  private:
    shared_ptr<BufferPoolManager> bpm_;

    size_t order_data_fid_;
    page_id_t order_data_header_page_id_;
    page_id_t order_data_last_page_id_;

    page_id_t order_index_header_page_id_;
    BPlusTree<ComposedKey<USER_NAME_LEN + 1>, size_t, Compare> order_index_;

  public:
    OrderManager(shared_ptr<BufferPoolManager> bpm,
                 size_t order_data_fid,
                 size_t order_index_fid);

    auto addOrder(const char *username, const OrderRecord &record) -> size_t;

    auto getOrders(const char *username) -> vector<OrderRecord>;

    void updateOrderStatus(size_t record_id, OrderStatus newStatus);

    void updateOrderStatus(const char *username, long long timestamp, OrderStatus newStatus);

    auto getOrderByRecordId(size_t record_id) -> OrderRecord;

    void Reset();
};
