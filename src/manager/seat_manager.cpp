#include "manager/seat_manager.h"
#include "buffer/buffer_pool_manager.h"
#include "common/config.h"
#include "parser/parser.h"
#include "utils/types.h"
#include "page/page_guard.h"
#include "type/type.hpp"
#include <csignal>

using sjtu::shared_ptr;

SeatManager::SeatManager(shared_ptr<BufferPoolManager> bpm,
                         size_t seat_index_fid,
                         size_t waitlist_data_fid,
                         size_t waitlist_index_fid)
  : bpm_(std::move(bpm)),
    seat_index_header_page_id_(seat_index_fid << FILE_BIT),
    seat_index_(seat_index_fid, seat_index_fid << FILE_BIT, bpm_),
    waitlist_data_fid_(waitlist_data_fid),
    waitlist_data_header_page_id_(waitlist_data_fid << FILE_BIT),
    waitlist_data_last_page_id_(),
    waitlist_index_header_page_id_(waitlist_index_fid << FILE_BIT),
    waitlist_index_(waitlist_index_fid, waitlist_index_fid << FILE_BIT, bpm_) {
    WritePageGuard guard = bpm_->WritePage(waitlist_data_header_page_id_);
    auto *header = guard.AsMut<WaitlistPageHeader>();
    if (header->magic_num != 0xDEADBEEF) {
        header->magic_num = 0xDEADBEEF;
        header->last_page = waitlist_data_fid_ << FILE_BIT | 1;
        waitlist_data_last_page_id_ = header->last_page;
    } else {
        waitlist_data_last_page_id_ = header->last_page;
    }
}

void SeatManager::Reset() {
    seat_index_.Reset();
    waitlist_index_.Reset();
    WritePageGuard guard = bpm_->WritePage(waitlist_data_header_page_id_);
    auto *header = guard.AsMut<WaitlistPageHeader>();
    header->magic_num = 0xDEADBEEF;
    header->last_page = waitlist_data_fid_ << FILE_BIT | 1;
    waitlist_data_last_page_id_ = header->last_page;
}

void SeatManager::packSeatKey(char *key_buf, const char *trainID, const char *date) const {
    memset(key_buf, 0, SEAT_KEY_LEN);
    strcpy(key_buf, trainID);
    strcpy(key_buf + strlen(trainID), date);
}

void SeatManager::initSeats(const char *trainID, short saleBegin, short saleEnd,
                             int stationNum, int seatNum) {

    char key_buf[SEAT_KEY_LEN];
    //std::cerr << "begin to init" << std::endl;
    for (short i = saleBegin; i <= saleEnd; i++) {
        char date_str[6];
        dayOffsetToDate(i, date_str);
        packSeatKey(key_buf, trainID, date_str);
        ComposedKey<SEAT_KEY_LEN> ck;
        memcpy(ck.fixed_key.key, key_buf, SEAT_KEY_LEN);
        //std::cerr << i << std::endl;
        for (int seg = 0; seg < stationNum - 1; seg++) {
            //std::cerr << seg << std::endl;
            ck.rid = seg;
            seat_index_.Insert(ck, seatNum);
            //std::cerr << "finish insert" << std::endl;
        } 
        //std::cerr << "check" << std::endl;
        //std::cerr << trainID << " " << date_str << " " << std::endl;
    }
    // WritePageGuard guard = bpm_->WritePage(seat_index_header_page_id_);
    // std::cerr << "root " << guard.As<BPlusTreeHeaderPage>()->root_page_id_ << std::endl;
}

auto SeatManager::getSeats(const char *trainID, const char *date) -> vector<int> {
    char key_buf[SEAT_KEY_LEN];
    packSeatKey(key_buf, trainID, date);
    ComposedKey<SEAT_KEY_LEN> ck;
    memcpy(ck.fixed_key.key, key_buf, SEAT_KEY_LEN);
    ck.rid = RID_MIN;
    vector<int> result;
    seat_index_.GetValue(ck, &result);
    //WritePageGuard guard = bpm_->WritePage(seat_index_header_page_id_);
    //std::cerr << "root " << guard.As<BPlusTreeHeaderPage>()->root_page_id_ << std::endl;
    return result;
}

auto SeatManager::deductSeats(const char *trainID, const char *date,
                               int from, int to, int num) -> bool {
    char key_buf[SEAT_KEY_LEN];
    packSeatKey(key_buf, trainID, date);

    ComposedKey<SEAT_KEY_LEN> ck;
    memcpy(ck.fixed_key.key, key_buf, SEAT_KEY_LEN);
    ck.rid = RID_MIN;
    vector<int> seats;
    seat_index_.GetValue(ck, &seats);

    for (int seg = from; seg < to; seg++) {
        if (seats[seg] < num) return false;
    }

    seat_index_.ScanAndUpdate(ck, [from, to, num](const ComposedKey<SEAT_KEY_LEN>& key, int old_val) -> int {
        if (key.rid >= from && key.rid < to) {
            return old_val - num;
        }
        return old_val;
    });
    return true;
}

void SeatManager::refundSeats(const char *trainID, const char *date,
                               int from, int to, int num) {
    char key_buf[SEAT_KEY_LEN];
    packSeatKey(key_buf, trainID, date);

    ComposedKey<SEAT_KEY_LEN> ck;
    memcpy(ck.fixed_key.key, key_buf, SEAT_KEY_LEN);
    ck.rid = RID_MIN;
    vector<int> seats;
    seat_index_.GetValue(ck, &seats);

    seat_index_.ScanAndUpdate(ck, [from, to, num](const ComposedKey<SEAT_KEY_LEN>& key, int old_val) -> int {
        if (key.rid >= from && key.rid < to) {
            return old_val + num;
        }
        return old_val;
    });
}

auto SeatManager::addToWaitlist(const char *trainID, const char *date,
                                 const WaitlistRecord &record) -> size_t {
    WritePageGuard page_guard = bpm_->WritePage(waitlist_data_last_page_id_);
    auto *page = page_guard.AsMut<WaitlistPage>();

    size_t record_id = waitlist_data_last_page_id_ * WaitlistPage::Num + page->last_index;

    char key_buf[SEAT_KEY_LEN];
    packSeatKey(key_buf, trainID, date);
    ComposedKey<SEAT_KEY_LEN> key;
    memcpy(key.fixed_key.key, key_buf, SEAT_KEY_LEN);
    key.rid = static_cast<int>(record.timestamp);
    waitlist_index_.Insert(key, record_id);

    page->data[page->last_index++] = record;

    if (page->last_index == WaitlistPage::Num) {
        WritePageGuard header_guard = bpm_->WritePage(waitlist_data_header_page_id_);
        auto *header = header_guard.AsMut<WaitlistPageHeader>();
        header->last_page++;
        waitlist_data_last_page_id_++;
        WritePageGuard new_page_guard = bpm_->WritePage(waitlist_data_last_page_id_);
        page = new_page_guard.AsMut<WaitlistPage>();
        page->last_index = 0;
    }
    // WritePageGuard guard = bpm_->WritePage(seat_index_header_page_id_);
    // std::cerr << "root " << guard.As<BPlusTreeHeaderPage>()->root_page_id_ << std::endl;
    return record_id;
}

void SeatManager::removeFromWaitlist(const char *trainID, const char *date,
                                      long long timestamp) {
    char key_buf[SEAT_KEY_LEN];
    packSeatKey(key_buf, trainID, date);
    ComposedKey<SEAT_KEY_LEN> key;
    memcpy(key.fixed_key.key, key_buf, SEAT_KEY_LEN);
    key.rid = static_cast<int>(timestamp);
    waitlist_index_.Remove(key);
    // WritePageGuard guard = bpm_->WritePage(seat_index_header_page_id_);
    // std::cerr << "root " << guard.As<BPlusTreeHeaderPage>()->root_page_id_ << std::endl;
}

auto SeatManager::getWaitlist(const char *trainID, const char *date) -> vector<WaitlistRecord> {
    char key_buf[SEAT_KEY_LEN];
    packSeatKey(key_buf, trainID, date);
    ComposedKey<SEAT_KEY_LEN> key;
    memcpy(key.fixed_key.key, key_buf, SEAT_KEY_LEN);
    key.rid = RID_MIN;

    vector<size_t> record_ids;
    waitlist_index_.GetValue(key, &record_ids);

    vector<WaitlistRecord> result;
    page_id_t current_page_id = INVALID_PAGE_ID;
    ReadPageGuard guard;
    for (size_t i = 0; i < record_ids.size(); i++) {
        page_id_t page_id = record_ids[i] / WaitlistPage::Num;
        size_t offset = record_ids[i] % WaitlistPage::Num;
        if (page_id != current_page_id) {
            current_page_id = page_id;
            guard = bpm_->ReadPage(page_id);
        }
        result.push_back(guard.As<WaitlistPage>()->data[offset]);
    }
    return result;
}

auto SeatManager::processWaitlist(const char *trainID, const char *date) -> vector<WaitlistRecord> {
    vector<WaitlistRecord> wait_list = getWaitlist(trainID, date);
    vector<WaitlistRecord> fulfilled;

    char key_buf[SEAT_KEY_LEN];
    packSeatKey(key_buf, trainID, date);
    ComposedKey<SEAT_KEY_LEN> ck;
    memcpy(ck.fixed_key.key, key_buf, SEAT_KEY_LEN);
    ck.rid = RID_MIN;
    vector<int> seats;
    seat_index_.GetValue(ck, &seats);

    for (size_t i = 0; i < wait_list.size(); i++) {
        int from = wait_list[i].fromSeq;
        int to = wait_list[i].toSeq;
        int num = wait_list[i].num;
        bool enough = true;
        for (int seg = from; seg < to; seg++) {
            if (seats[seg] < num) {
                enough = false;
                break;
            }
        }
        if (!enough) continue;

        seat_index_.ScanAndUpdate(ck, [from, to, num, &seats](const ComposedKey<SEAT_KEY_LEN>& key, int old_val) -> int {
            if (key.rid >= from && key.rid < to) {
                int new_val = old_val - num;
                seats[key.rid] = new_val;
                return new_val;
            }
            return old_val;
        });
        fulfilled.push_back(wait_list[i]);
        removeFromWaitlist(trainID, date, wait_list[i].timestamp);
    }
    return fulfilled;
}
