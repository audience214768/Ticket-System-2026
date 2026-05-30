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

//   Page layout for train data (2 pages per train):
//   Page 0: TrainRecord header (16B) + page_id_t page1 (8B) + StationRecord[STATIONS_PER_PAGE0]
//   Page 1: StationRecord[STATIONS_PER_PAGE1]
static constexpr size_t TRAIN_HEADER_SIZE = sizeof(TrainRecord);           // 16
static constexpr size_t TRAIN_PAGE1_OFFSET = TRAIN_HEADER_SIZE;            // 16
static constexpr size_t TRAIN_STATIONS_OFFSET = TRAIN_PAGE1_OFFSET + sizeof(page_id_t);  // 24
static constexpr size_t STATIONS_PER_PAGE0 = (DISK_PAGE_SIZE - TRAIN_STATIONS_OFFSET) / sizeof(StationRecord);
static constexpr size_t STATIONS_PER_PAGE1 = DISK_PAGE_SIZE / sizeof(StationRecord);

class TrainManager {
  private:
    shared_ptr<BufferPoolManager> bpm_;

    // train_index_: key=trainID, value=page_id_t (first page of train data)
    BPlusTree<ComposedKey<TRAIN_ID_LEN + 1>, page_id_t, Compare> train_index_;

    // station data pages (2 pages per train)
    size_t station_data_fid_;

    // station_lookup: (stationName, rid=timestamp) → StationLookupValue
    BPlusTree<ComposedKey<STATION_NAME_LEN * 5>, StationLookupValue, Compare> station_lookup_index_;

  public:
    TrainManager(shared_ptr<BufferPoolManager> bpm,
                 size_t train_index_fid,
                 size_t station_data_fid,
                 size_t station_lookup_fid);

    auto addTrain(const char *trainID, const TrainRecord &meta,
                  const vector<StationRecord> &stations, long long timestamp) -> bool;

    auto deleteTrain(const char *trainID) -> bool;

    auto releaseTrain(const char *trainID) -> bool;

    auto getTrain(const char *trainID) -> TrainRecord;

    auto getStations(const char *trainID) -> vector<StationRecord>;

    struct TrainData {
        TrainRecord meta;
        vector<StationRecord> stations;
    };
    auto getTrainData(const char *trainID) -> TrainData;

    auto getStation(const char *trainID, int seq) -> StationRecord;

    auto getTrainsByStation(const char *stationName) -> vector<StationLookupValue>;

    void Reset();
};
