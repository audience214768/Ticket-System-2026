#include "manager/train_manager.h"
#include "buffer/buffer_pool_manager.h"
#include "common/config.h"
#include "utils/config.h"
#include "utils/err.h"
#include "utils/types.h"
#include "page/page_guard.h"
#include "type/type.hpp"

using sjtu::shared_ptr;

TrainManager::TrainManager(shared_ptr<BufferPoolManager> bpm,
                           size_t train_index_fid,
                           size_t station_data_fid,
                           size_t station_lookup_fid)
  : bpm_(std::move(bpm)),
    train_index_(train_index_fid, train_index_fid << FILE_BIT, bpm_),
    station_data_fid_(station_data_fid),
    station_lookup_index_(station_lookup_fid, station_lookup_fid << FILE_BIT, bpm_) {}

void TrainManager::Reset() {
    train_index_.Reset();
    station_lookup_index_.Reset();
}

auto TrainManager::addTrain(const char *trainID, const TrainRecord &meta,
                             const vector<StationRecord> &stations, long long timestamp) -> bool {
    //std::cerr << "add train" << std::endl;
    {
        ComposedKey<TRAIN_ID_LEN + 1> ck;
        strcpy(ck.fixed_key.key, trainID);
        ck.rid = RID_MIN;
        vector<page_id_t> tmp;
        train_index_.GetValue(ck, &tmp);
        if (!tmp.empty()) return false;
    }

    int n = stations.size();
    page_id_t page0 = bpm_->NewPage(station_data_fid_);
    page_id_t page1 = INVALID_PAGE_ID;
    if (n > STATIONS_PER_PAGE0) {
        page1 = bpm_->NewPage(station_data_fid_);
    }

    {
        WritePageGuard guard = bpm_->WritePage(page0);
        auto *raw = guard.AsMut<char>();
        memcpy(raw, &meta, sizeof(TrainRecord));
        memcpy(raw + TRAIN_PAGE1_OFFSET, &page1, sizeof(page_id_t));

        int n0 = n < STATIONS_PER_PAGE0 ? n : STATIONS_PER_PAGE0;
        auto *dst = reinterpret_cast<StationRecord *>(raw + TRAIN_STATIONS_OFFSET);
        for (int i = 0; i < n0; i++) {
            StationRecord rec = stations[i];
            rec.lookup_rid = timestamp;

            ComposedKey<STATION_NAME_LEN * 5> sk;
            strcpy(sk.fixed_key.key, rec.stationName);
            sk.rid = timestamp;
            StationLookupValue val;
            strcpy(val.trainID, trainID);
            val.seq = i;
            station_lookup_index_.Insert(sk, val);

            dst[i] = rec;
        }
    }

    if (page1 != INVALID_PAGE_ID) {
        WritePageGuard guard = bpm_->WritePage(page1);
        auto *raw = guard.AsMut<char>();
        int n1 = n - STATIONS_PER_PAGE0;
        auto *dst = reinterpret_cast<StationRecord *>(raw);
        for (int i = 0; i < n1; i++) {
            StationRecord rec = stations[STATIONS_PER_PAGE0 + i];
            rec.lookup_rid = timestamp;

            ComposedKey<STATION_NAME_LEN * 5> sk;
            strcpy(sk.fixed_key.key, rec.stationName);
            sk.rid = timestamp;
            StationLookupValue val;
            strcpy(val.trainID, trainID);
            val.seq = STATIONS_PER_PAGE0 + i;
            station_lookup_index_.Insert(sk, val);

            dst[i] = rec;
        }
    }

    {
        ComposedKey<TRAIN_ID_LEN + 1> tk;
        strcpy(tk.fixed_key.key, trainID);
        tk.rid = 0;
        train_index_.Insert(tk, page0);
    }

    return true;
}

auto TrainManager::deleteTrain(const char *trainID) -> bool {
    ComposedKey<TRAIN_ID_LEN + 1> tk;
    strcpy(tk.fixed_key.key, trainID);
    tk.rid = RID_MIN;
    vector<page_id_t> results;
    train_index_.GetValue(tk, &results);
    if (results.empty()) {
        return false;
    }

    page_id_t page0 = results[0];

    TrainRecord meta;
    page_id_t page1;
    {
        ReadPageGuard guard = bpm_->ReadPage(page0);
        const auto *raw = guard.As<char>();
        memcpy(&meta, raw, sizeof(TrainRecord));
        memcpy(&page1, raw + TRAIN_PAGE1_OFFSET, sizeof(page_id_t));
    }
    if (meta.released) {
        return false;
    }

    {
        ReadPageGuard guard = bpm_->ReadPage(page0);
        const auto *raw = guard.As<char>();
        int n0 = meta.stationNum < STATIONS_PER_PAGE0 ? meta.stationNum : STATIONS_PER_PAGE0;
        const auto *src = reinterpret_cast<const StationRecord *>(raw + TRAIN_STATIONS_OFFSET);
        for (int i = 0; i < n0; i++) {
            ComposedKey<STATION_NAME_LEN * 5> sk;
            strcpy(sk.fixed_key.key, src[i].stationName);
            sk.rid = src[i].lookup_rid;
            station_lookup_index_.Remove(sk);
        }
    }

    if (page1 != INVALID_PAGE_ID) {
        ReadPageGuard guard = bpm_->ReadPage(page1);
        const auto *raw = guard.As<char>();
        int n1 = meta.stationNum - STATIONS_PER_PAGE0;
        const auto *src = reinterpret_cast<const StationRecord *>(raw);
        for (int i = 0; i < n1; i++) {
            ComposedKey<STATION_NAME_LEN * 5> sk;
            strcpy(sk.fixed_key.key, src[i].stationName);
            sk.rid = src[i].lookup_rid;
            station_lookup_index_.Remove(sk);
        }
    }

    bpm_->DeletePage(page0);
    if (page1 != INVALID_PAGE_ID) {
        bpm_->DeletePage(page1);
    }

    tk.rid = 0;
    train_index_.Remove(tk);

    return true;
}

auto TrainManager::releaseTrain(const char *trainID) -> bool {
    ComposedKey<TRAIN_ID_LEN + 1> tk;
    strcpy(tk.fixed_key.key, trainID);
    tk.rid = RID_MIN;
    vector<page_id_t> results;
    train_index_.GetValue(tk, &results);
    if (results.empty()) {
        return false;
    }

    page_id_t page0 = results[0];

    WritePageGuard guard = bpm_->WritePage(page0);
    auto *meta = guard.AsMut<TrainRecord>();
    if (meta->released) {
        return false;
    }
    meta->released = true;

    return true;
}

auto TrainManager::getTrain(const char *trainID) -> TrainRecord {
    ComposedKey<TRAIN_ID_LEN + 1> tk;
    strcpy(tk.fixed_key.key, trainID);
    tk.rid = RID_MIN;
    vector<page_id_t> results;
    train_index_.GetValue(tk, &results);
    if (results.empty()) {
        throw Exception("train not found");
    }

    ReadPageGuard guard = bpm_->ReadPage(results[0]);
    const auto *raw = guard.As<char>();
    TrainRecord meta;
    memcpy(&meta, raw, sizeof(TrainRecord));
    return meta;
}

auto TrainManager::getStations(const char *trainID) -> vector<StationRecord> {
    vector<StationRecord> result;

    ComposedKey<TRAIN_ID_LEN + 1> tk;
    strcpy(tk.fixed_key.key, trainID);
    tk.rid = RID_MIN;
    vector<page_id_t> results;
    train_index_.GetValue(tk, &results);
    if (results.empty()) return result;

    page_id_t page0 = results[0];

    TrainRecord meta;
    page_id_t page1;
    {
        ReadPageGuard guard = bpm_->ReadPage(page0);
        const auto *raw = guard.As<char>();
        memcpy(&meta, raw, sizeof(TrainRecord));
        memcpy(&page1, raw + TRAIN_PAGE1_OFFSET, sizeof(page_id_t));

        int n0 = meta.stationNum < STATIONS_PER_PAGE0 ? meta.stationNum : STATIONS_PER_PAGE0;
        const auto *src = reinterpret_cast<const StationRecord *>(raw + TRAIN_STATIONS_OFFSET);
        for (int i = 0; i < n0; i++) {
            result.push_back(src[i]);
        }
    }

    if (page1 != INVALID_PAGE_ID) {
        ReadPageGuard guard = bpm_->ReadPage(page1);
        const auto *raw = guard.As<char>();
        int n1 = meta.stationNum - STATIONS_PER_PAGE0;
        const auto *src = reinterpret_cast<const StationRecord *>(raw);
        for (int i = 0; i < n1; i++) {
            result.push_back(src[i]);
        }
    }

    return result;
}

auto TrainManager::getTrainData(const char *trainID) -> TrainData {
    TrainData data;

    ComposedKey<TRAIN_ID_LEN + 1> tk;
    strcpy(tk.fixed_key.key, trainID);
    tk.rid = RID_MIN;
    vector<page_id_t> results;
    train_index_.GetValue(tk, &results);
    if (results.empty()) {
        throw Exception("train not found");
    }

    page_id_t page0 = results[0];
    page_id_t page1;

    {
        ReadPageGuard guard = bpm_->ReadPage(page0);
        const auto *raw = guard.As<char>();
        memcpy(&data.meta, raw, sizeof(TrainRecord));
        memcpy(&page1, raw + TRAIN_PAGE1_OFFSET, sizeof(page_id_t));

        int n0 = data.meta.stationNum < STATIONS_PER_PAGE0 ? data.meta.stationNum : STATIONS_PER_PAGE0;
        data.stations.reserve(data.meta.stationNum);
        const auto *src = reinterpret_cast<const StationRecord *>(raw + TRAIN_STATIONS_OFFSET);
        for (int i = 0; i < n0; i++) {
            data.stations.push_back(src[i]);
        }
    }

    if (page1 != INVALID_PAGE_ID) {
        ReadPageGuard guard = bpm_->ReadPage(page1);
        const auto *raw = guard.As<char>();
        int n1 = data.meta.stationNum - STATIONS_PER_PAGE0;
        const auto *src = reinterpret_cast<const StationRecord *>(raw);
        for (int i = 0; i < n1; i++) {
            data.stations.push_back(src[i]);
        }
    }

    return data;
}

auto TrainManager::getStation(const char *trainID, int seq) -> StationRecord {
    ComposedKey<TRAIN_ID_LEN + 1> tk;
    strcpy(tk.fixed_key.key, trainID);
    tk.rid = RID_MIN;
    vector<page_id_t> results;
    train_index_.GetValue(tk, &results);
    if (results.empty()) {
        throw Exception("train not found");
    }

    if (seq < STATIONS_PER_PAGE0) {
        ReadPageGuard guard = bpm_->ReadPage(results[0]);
        const auto *raw = guard.As<char>();
        const auto *src = reinterpret_cast<const StationRecord *>(raw + TRAIN_STATIONS_OFFSET);
        return src[seq];
    } else {
        TrainRecord meta;
        page_id_t page1;
        {
            ReadPageGuard guard = bpm_->ReadPage(results[0]);
            const auto *raw = guard.As<char>();
            memcpy(&meta, raw, sizeof(TrainRecord));
            memcpy(&page1, raw + TRAIN_PAGE1_OFFSET, sizeof(page_id_t));
        }
        ReadPageGuard guard = bpm_->ReadPage(page1);
        const auto *raw = guard.As<char>();
        const auto *src = reinterpret_cast<const StationRecord *>(raw);
        return src[seq - STATIONS_PER_PAGE0];
    }
}

auto TrainManager::getTrainsByStation(const char *stationName) -> vector<StationLookupValue> {
    ComposedKey<STATION_NAME_LEN * 5> sk;
    strcpy(sk.fixed_key.key, stationName);
    sk.rid = RID_MIN;
    vector<StationLookupValue> result;
    station_lookup_index_.GetValue(sk, &result);
    return result;
}
