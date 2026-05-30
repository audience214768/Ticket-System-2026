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


struct WaitlistPage {
    static constexpr size_t Num = (DISK_PAGE_SIZE - sizeof(size_t)) / sizeof(WaitlistRecord);
    size_t last_index = 0;
    WaitlistRecord data[Num];
};

struct WaitlistPageHeader {
    size_t last_page;
    size_t magic_num;
};


class SeatManager {
  private:
    shared_ptr<BufferPoolManager> bpm_;

    // key: ComposedKey<26>, fixed_key = trainID(20) + date(6), rid = segIdx
    static constexpr size_t SEAT_KEY_LEN = TRAIN_ID_LEN + 6;
    page_id_t seat_index_header_page_id_;
    BPlusTree<ComposedKey<SEAT_KEY_LEN>, int, Compare> seat_index_;

    size_t waitlist_data_fid_;
    page_id_t waitlist_data_header_page_id_;
    page_id_t waitlist_data_last_page_id_;

    // waitlist index: (trainID+date, rid=timestamp) → record_id
    page_id_t waitlist_index_header_page_id_;
    BPlusTree<ComposedKey<SEAT_KEY_LEN>, size_t, Compare> waitlist_index_;

    void packSeatKey(char *key_buf, const char *trainID, const char *date) const;

  public:
    SeatManager(shared_ptr<BufferPoolManager> bpm,
                size_t seat_index_fid,
                size_t waitlist_data_fid,
                size_t waitlist_index_fid);

    void initSeats(const char *trainID, short saleBegin, short saleEnd,
                   int stationNum, int seatNum);

    auto getSeats(const char *trainID, const char *date) -> vector<int>;

    auto deductSeats(const char *trainID, const char *date, int from, int to, int num) -> bool;

    void refundSeats(const char *trainID, const char *date, int from, int to, int num);

    auto addToWaitlist(const char *trainID, const char *date, const WaitlistRecord &record) -> size_t;
    void removeFromWaitlist(const char *trainID, const char *date, long long timestamp);
    auto getWaitlist(const char *trainID, const char *date) -> vector<WaitlistRecord>;

    // process waitlist after refund (returns fulfilled records)
    auto processWaitlist(const char *trainID, const char *date) -> vector<WaitlistRecord>;

    void Reset();
};
