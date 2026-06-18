#include "manager/order_manager.h"
#include "buffer/buffer_pool_manager.h"
#include "common/config.h"
#include "utils/types.h"
#include "page/page_guard.h"
#include "type/type.hpp"

using sjtu::shared_ptr;

OrderManager::OrderManager(shared_ptr<BufferPoolManager> bpm,
                           size_t order_data_fid,
                           size_t order_index_fid)
  : bpm_(std::move(bpm)),
    order_data_fid_(order_data_fid),
    order_data_header_page_id_(order_data_fid << FILE_BIT),
    order_data_last_page_id_(),
    order_index_header_page_id_(order_index_fid << FILE_BIT),
    order_index_(order_index_fid, order_index_fid << FILE_BIT, bpm_) {
    WritePageGuard guard = bpm_->WritePage(order_data_header_page_id_);
    auto *header = guard.AsMut<OrderPageHeader>();
    if (header->magic_num != 0xDEADBEEF) {
        header->magic_num = 0xDEADBEEF;
        header->last_page = order_data_fid_ << FILE_BIT | 1;
        order_data_last_page_id_ = header->last_page;
    } else {
        order_data_last_page_id_ = header->last_page;
    }
}

auto OrderManager::addOrder(const char *username, const OrderRecord &record) -> size_t {
    //std::cerr << "add order" << std::endl;
    WritePageGuard page_guard = bpm_->WritePage(order_data_last_page_id_);
    auto *page = page_guard.AsMut<OrderPage>();

    size_t record_id = order_data_last_page_id_ * OrderPage::Num + page->last_index;

    ComposedKey<USER_NAME_LEN + 1> key;
    strcpy(key.fixed_key.key, username);
    key.rid = record.timestamp;
    order_index_.Insert(key, record_id);

    page->data[page->last_index++] = record;

    if (page->last_index == OrderPage::Num) {
        WritePageGuard header_guard = bpm_->WritePage(order_data_header_page_id_);
        auto *header = header_guard.AsMut<OrderPageHeader>();
        header->last_page++;
        order_data_last_page_id_++;
        WritePageGuard new_page_guard = bpm_->WritePage(order_data_last_page_id_);
        page = new_page_guard.AsMut<OrderPage>();
        page->last_index = 0;
    }

    return record_id;
}

auto OrderManager::getOrders(const char *username) -> vector<OrderRecord> {
    //std::cerr << "start to get order" << std::endl;
    ComposedKey<USER_NAME_LEN + 1> key;
    strcpy(key.fixed_key.key, username);
    key.rid = RID_MIN;
    vector<size_t> record_ids;
    order_index_.GetValue(key, &record_ids);
    //std::cerr << record_ids.size() << std::endl;
    vector<OrderRecord> orders;
    page_id_t current_page_id = INVALID_PAGE_ID;
    ReadPageGuard guard;
    for (int i = record_ids.size() - 1; i >= 0; i--) {
        page_id_t page_id = record_ids[i] / OrderPage::Num;
        size_t offset = record_ids[i] % OrderPage::Num;
        if (page_id != current_page_id) {
            current_page_id = page_id;
            guard = bpm_->ReadPage(page_id);
        }
        orders.push_back(guard.As<OrderPage>()->data[offset]);
    }

    return orders;
}

void OrderManager::updateOrderStatus(const char *username, long long timestamp, OrderStatus newStatus) {
    ComposedKey<USER_NAME_LEN + 1> key;
    strcpy(key.fixed_key.key, username);
    key.rid = RID_MIN;
    vector<size_t> record_ids;
    order_index_.GetValue(key, &record_ids);

    for (size_t i = 0; i < record_ids.size(); i++) {
        page_id_t page_id = record_ids[i] / OrderPage::Num;
        size_t offset = record_ids[i] % OrderPage::Num;
        ReadPageGuard guard = bpm_->ReadPage(page_id);
        if (guard.As<OrderPage>()->data[offset].timestamp == timestamp) {
            guard.Drop();
            WritePageGuard wguard = bpm_->WritePage(page_id);
            wguard.AsMut<OrderPage>()->data[offset].status = newStatus;
            return;
        }
    }
}

auto OrderManager::getOrderByRecordId(size_t record_id) -> OrderRecord {
    page_id_t page_id = record_id / OrderPage::Num;
    size_t offset = record_id % OrderPage::Num;
    ReadPageGuard guard = bpm_->ReadPage(page_id);
    return guard.As<OrderPage>()->data[offset];
}

void OrderManager::Reset() {
    order_index_.Reset();
    WritePageGuard guard = bpm_->WritePage(order_data_header_page_id_);
    auto *header = guard.AsMut<OrderPageHeader>();
    header->magic_num = 0xDEADBEEF;
    header->last_page = order_data_fid_ << FILE_BIT | 1;
    order_data_last_page_id_ = header->last_page;
}
