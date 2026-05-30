#include "core/ticket_system.h"
#include "parser/parser.h"
#include "utils/err.h"
#include "utils/types.h"
#include "manager/train_manager.h"
#include "manager/user_manager.h"
#include "manager/order_manager.h"
#include "manager/seat_manager.h"
#include "shared_ptr/shared_ptr.hpp"
#include "utils/hash.h"

#include <cstring>
#include <cstdio>
#include <cstdlib>

using std::strcmp;

extern long long current_timestamp;

AddUser::AddUser(int argc, char *argv[], shared_ptr<UserManager> user_manager)
  : user_manager_(std::move(user_manager)) {
    if (!user_manager_->haveUser()) {
        priv_.data = 10;
    }
    if (argc != 13 || argc % 2 == 0) {
        throw Exception("add_user : argc is should be 13");
    }
    for (int i = 1; i < argc; i += 2) {
        if (strcmp(argv[i], "-c") == 0) {
            if (user_manager_->haveUser()) {
                cur_user_name_.parser(argv[i + 1]);
            }
        } else if (strcmp(argv[i], "-u") == 0) {
            user_name_.parser(argv[i + 1]);
        } else if (strcmp(argv[i], "-p") == 0) {
            pwd_.parser(argv[i + 1]);
        } else if (strcmp(argv[i], "-n") == 0) {
            name_.parser(argv[i + 1]);
        } else if (strcmp(argv[i], "-m") == 0) {
            mail_addr_.parser(argv[i + 1]);
        } else if (strcmp(argv[i], "-g") == 0) {
            if (user_manager_->haveUser()) {
                priv_.parser(argv[i + 1]);
            }
        } else {
            throw Exception("add_user: invalid arg");
        }
    }
}

void AddUser::execute() {
    if (user_manager_->haveUser()) {
        UserNode **ptr = user_manager_->getLoggedUser(cur_user_name_.data);
        if (*ptr == nullptr) {
            throw Exception("add_user: the current user is not loggged");
        }
        if ((*ptr)->priv <= priv_.data) {
            throw Exception("add_user: the current user's privilege is lower than new user's privilege");
        }
    }
    UserRecord record;
    strcpy(record.password, pwd_.data);
    strcpy(record.name, name_.data);
    strcpy(record.mailAddr, mail_addr_.data);
    record.privilege = priv_.data;
    if (!user_manager_->addUser(user_name_.data, record)) {
        throw Exception("add_user : the user_name exist");
    }
    printf("[%lld] 0\n", current_timestamp);
}

LogIn::LogIn(int argc, char *argv[], shared_ptr<UserManager> user_manager)
  : user_manager_(std::move(user_manager)) {
  if (argc != 5 || argc % 2 == 0) {
    throw Exception("login: argc should be 5");
  }
  for (int i = 1; i < argc; i += 2) {
    if (strcmp(argv[i], "-u") == 0) {
        user_name_.parser(argv[i + 1]);
    } else if (strcmp(argv[i], "-p") == 0) {
        pwd_.parser(argv[i + 1]);
    } else {
        throw Exception("login: invalid arg");
    }
  }
}

void LogIn::execute() {
    UserNode **ptr = user_manager_->getLoggedUser(user_name_.data);
    if (*ptr != nullptr) {
        throw Exception("login: the user is already logged");
    }
    user_manager_->login(user_name_.data, pwd_.data, ptr);
    printf("[%lld] 0\n", current_timestamp);
}

LogOut::LogOut(int argc, char *argv[], shared_ptr<UserManager> user_manager)
  : user_manager_(std::move(user_manager)) {
    if (argc != 3 || argc % 2 == 0) {
        throw Exception("logout: the argc should be 3");
    }
    if (strcmp(argv[1], "-u") == 0) {
        user_name_.parser(argv[2]);
    } else {
        throw Exception("logout: invalid arg");
    }
}

void LogOut::execute() {
    UserNode **ptr = user_manager_->getLoggedUser(user_name_.data);
    if (*ptr == nullptr) {
        throw Exception("login: the user is not logged");
    }
    user_manager_->logout(user_name_.data, ptr);
    printf("[%lld] 0\n", current_timestamp);
}

QueryProfile::QueryProfile(int argc, char *argv[], shared_ptr<UserManager> user_manager)
  : user_manager_(std::move(user_manager)) {
    if (argc != 5 || argc % 2 == 0) {
        throw Exception("query_profile: argc should be 5");
    }
    for (int i = 1; i < argc; i += 2) {
        if (strcmp(argv[i], "-c") == 0) {
            cur_user_name_.parser(argv[i + 1]);
        } else if (strcmp(argv[i], "-u") == 0) {
            user_name_.parser(argv[i + 1]);
        } else {
            throw Exception("query_profile: invalid arg");
        }
    }
}

void QueryProfile::execute() {
    //std::cerr << "start to query profile " << current_timestamp << std::endl;
    UserNode **ptr = user_manager_->getLoggedUser(cur_user_name_.data);
    if (*ptr == nullptr) {
        throw Exception("query_profile: the current user is not loggged");
    }
    UserRecord record;
    //std::cerr << "check1" << std::endl;
    user_manager_->queryProfile(user_name_.data, record);
    //std::cerr << record.privilege << " " << (*ptr)->priv << std::endl;
    if (record.privilege >= (*ptr)->priv && strcmp(user_name_.data, cur_user_name_.data) != 0) {
        throw Exception("query_profile: the current user's privilege is lower than query user's");
    }
    printf("[%lld] %s %s %s %d\n", current_timestamp, user_name_.data, record.name, record.mailAddr, record.privilege);
    //std::cerr << "finish" << std::endl;
}

ModifyProfile::ModifyProfile(int argc, char *argv[], shared_ptr<UserManager> user_manager)
  : user_manager_(std::move(user_manager)) {
    if (argc < 5 || argc > 13 || argc % 2 == 0) {
        throw Exception("modify_profile: argc is not right");
    }
    for (int i = 1; i < argc; i += 2) {
        if (strcmp(argv[i], "-c") == 0) {
            cur_user_name_.parser(argv[i + 1]);
        } else if (strcmp(argv[i], "-u") == 0) {
            user_name_.parser(argv[i + 1]);
        } else if (strcmp(argv[i], "-p") == 0) {
            new_pwd_.parser(argv[i + 1]);
        } else if (strcmp(argv[i], "-n") == 0) {
            new_name_.parser(argv[i + 1]);
        } else if (strcmp(argv[i], "-m") == 0) {
            new_mail_addr_.parser(argv[i + 1]);
        } else if (strcmp(argv[i], "-g") == 0) {
            new_priv_.parser(argv[i + 1]);
        } else {
            throw Exception("modify_profile: invalid arg");
        }
    }
}

void ModifyProfile::execute() {
    //std::cerr << "start to modify " << current_timestamp << std::endl;
    UserNode **ptr = user_manager_->getLoggedUser(cur_user_name_.data);
    if (*ptr == nullptr) {
        throw Exception("modify_profile: current user is not logged");
    }
    UserRecord record;
    user_manager_->queryProfile(user_name_.data, record);
    //std::cerr << record.name << " " << record.mailAddr << " " << record.password << std::endl;
    if (record.privilege >= (*ptr)->priv && strcmp(user_name_.data, cur_user_name_.data) != 0) {
        //std::cerr << record.privilege << " " << (*ptr)->priv << std::endl;
        throw Exception("modify_profile: current user's privilege is lower than modify user's");
    }
    if (new_priv_.data >= (*ptr)->priv) {
        throw Exception("modify_profile: current user's privilege is lower than modified privilege");
    }
    if (new_pwd_.data[0] != '\0') {
        strcpy(record.password, new_pwd_.data);
    }
    if (new_name_.data[0] != '\0') {
        strcpy(record.name, new_name_.data);
    }
    if (new_mail_addr_.data[0] != '\0') {
        strcpy(record.mailAddr, new_mail_addr_.data);
    }
    if (new_priv_.data != -1) {
        record.privilege = new_priv_.data;
        UserNode **ptr1 = user_manager_->getLoggedUser(user_name_.data);
        if (*ptr1 != nullptr) {
            (*ptr1)->priv = new_priv_.data;
        }
    }
    user_manager_->modifyProfile(user_name_.data, record);
    printf("[%lld] %s %s %s %d\n", current_timestamp, user_name_.data, record.name, record.mailAddr, record.privilege);
    //std::cerr << "finish" << std::endl;
}

AddTrain::AddTrain(int argc, char *argv[], shared_ptr<TrainManager> train_manager)
  : train_manager_(std::move(train_manager)) {
    if (argc != 21) {
        throw Exception("add_train: argc is not right");
    }
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0) {
            train_id_.parser(argv[i + 1]);
        } else if (strcmp(argv[i], "-n") == 0) {
            station_num_.parser(argv[i + 1]);
        } else if (strcmp(argv[i], "-m") == 0) {
            seat_num_.parser(argv[i + 1]);
        } else if (strcmp(argv[i], "-s") == 0) {
            if (stations_name.size() != 0) {
                throw Exception("duplicate arg");
            }
            char *token = strtok(argv[i + 1], "|");
            while (token) {
                StationName name;
                name.parser(token);
                stations_name.emplace_back(std::move(name));
                token = strtok(nullptr, "|");
            }
        } else if (strcmp(argv[i], "-p") == 0) {
            if (prices_.size() != 0) {
                throw Exception("duplicate arg");
            }
            char *token = strtok(argv[i + 1], "|");
            while (token) {
                Price price;
                price.parser(token);
                prices_.emplace_back(std::move(price));
                token = strtok(nullptr, "|");
            }
        } else if (strcmp(argv[i], "-x") == 0) {
            start_time_.parser(argv[i + 1]);
        } else if (strcmp(argv[i], "-t") == 0) {
            if (travel_times_.size() != 0) {
                throw Exception("duplicate arg");
            }
            char *token = strtok(argv[i + 1], "|");
            while (token) {
                TravelTime time;
                time.parser(token);
                travel_times_.emplace_back(std::move(time));
                token = strtok(nullptr, "|");
            }
        } else if (strcmp(argv[i], "-o") == 0) {
            if (stop_over_times_.size() != 0) {
                throw Exception("duplicate arg");
            }
            if (argv[i + 1][0] == '_' && strlen(argv[i + 1]) == 1) {
                continue;
            }
            char *token = strtok(argv[i + 1], "|");
            while (token) {
                StopoverTime time;
                time.parser(token);
                stop_over_times_.emplace_back(std::move(time));
                token = strtok(nullptr, "|");
            }
        } else if (strcmp(argv[i], "-d") == 0) {
            start_sale_date_.parser(strtok(argv[i + 1], "|"));
            end_sale_date_.parser(strtok(nullptr, "|"));
            if (strtok(nullptr, "|")) {
                throw Exception("add_train: saledate is not right");
            }
        } else if (strcmp(argv[i], "-y") == 0) {
            type_.parser(argv[i + 1]);
        }
    }
    if (station_num_.data != stations_name.size()) {
        throw Exception("add_train: station_num is not right");
    }
    if (station_num_.data != stop_over_times_.size() + 2) {
        throw Exception("add_train: stop_over_time is not right");
    }
    if (station_num_.data != prices_.size() + 1) {
        throw Exception("add_train: prices is not right");
    }
    if (station_num_.data != travel_times_.size() + 1) {
        throw Exception("add_train: travel_time is not right");
    }
}

void AddTrain::execute() {
    vector<StationRecord> stations;
    TrainRecord train = TrainRecord {
        .stationNum = station_num_.data,
        .seatNum = seat_num_.data,
        .startTime = start_time_.data,
        .saleDateBegin = start_sale_date_.data,
        .saleDateEnd = end_sale_date_.data,
        .type = type_.data,
        .released = false,
    };
    int price = 0;
    int time = 0;
    int cur_stop = 0;
    for (int i = 0; i < station_num_.data; i++) {
        StationRecord station;
        strcpy(station.stationName, stations_name[i].data);
        if (i != 0) {
            price += prices_[i - 1].data;
        }
        station.price = price;
        if (i != 0) {
            time += travel_times_[i - 1].data;
        }
        station.travelTime = time;
        station.stopTime = cur_stop;
        if (i != 0 && i != station_num_.data - 1) {
            cur_stop += stop_over_times_[i - 1].data;
        }
        stations.push_back(station);
    }
    if (!train_manager_->addTrain(train_id_.data, train, stations, current_timestamp)) {
        throw Exception("add_train: the train_id is exist");
    }
    printf("[%lld] 0\n", current_timestamp);
}

DeleteTrain::DeleteTrain(int argc, char *argv[], shared_ptr<TrainManager> train_manager)
  : train_manager_(std::move(train_manager)) {
    if (argc != 3 || argc % 2 == 0) {
        throw Exception("delete_train: argc should be 3");
    }
    for (int i = 1; i < argc; i += 2) {
        if (strcmp(argv[i], "-i") == 0) {
            train_id_.parser(argv[i + 1]);
        } else {
            throw Exception("delete_train: invalid arg");
        }
    }
}

void DeleteTrain::execute() {
    if (!train_manager_->deleteTrain(train_id_.data)) {
        throw Exception("delete_train: train not found or already released");
    }
    printf("[%lld] 0\n", current_timestamp);
}

ReleaseTrain::ReleaseTrain(int argc, char *argv[], shared_ptr<TrainManager> train_manager,
                           shared_ptr<SeatManager> seat_manager)
  : train_manager_(std::move(train_manager)), 
    seat_manager_(std::move(seat_manager)) {
    if (argc != 3 || argc % 2 == 0) {
        throw Exception("release_train: argc should be 3");
    }
    for (int i = 1; i < argc; i += 2) {
        if (strcmp(argv[i], "-i") == 0) {
            train_id_.parser(argv[i + 1]);
        } else {
            throw Exception("release_train: invalid arg");
        }
    }
}

void ReleaseTrain::execute() {
    //std::cerr << "release train" << std::endl;
    if (!train_manager_->releaseTrain(train_id_.data)) {
        throw Exception("release_train: train not found or already released");
    }
    //std::cerr << "check" << std::endl;
    TrainRecord train = train_manager_->getTrain(train_id_.data);
    //std::cerr << "check1" << std::endl;
    seat_manager_->initSeats(train_id_.data, train.saleDateBegin, train.saleDateEnd,
                             train.stationNum, train.seatNum);
    //std::cerr << "check2" << std::endl;
    printf("[%lld] 0\n", current_timestamp);
    //std::cerr << "finish" << std::endl;
}

QueryTrain::QueryTrain(int argc, char *argv[], shared_ptr<TrainManager> train_manager,
                       shared_ptr<SeatManager> seat_manager)
  : train_manager_(std::move(train_manager)), 
    seat_manager_(std::move(seat_manager)) {
    if (argc != 5 || argc % 2 == 0) {
        throw Exception("query_train: argc should be 5");
    }
    for (int i = 1; i < argc; i += 2) {
        if (strcmp(argv[i], "-i") == 0) {
            train_id_.parser(argv[i + 1]);
        } else if (strcmp(argv[i], "-d") == 0) {
            date_.parser(argv[i + 1]);
        } else {
            throw Exception("query_train: invalid arg");
        }
    }
}

void QueryTrain::execute() {
    auto train_data = train_manager_->getTrainData(train_id_.data);
    TrainRecord &train = train_data.meta;
    vector<StationRecord> &stations = train_data.stations;
    int station_num = train.stationNum;

    vector<int> seats;
    if (train.released) {
        seats = seat_manager_->getSeats(train_id_.data, date_.data);
    } else {
        for (int i = 0; i < station_num - 1; i++) {
            seats.push_back(train.seatNum);
        }
    }
    int date = dateToDayOffset(date_.data);
    //std::cerr << date << " " << train.saleDateEnd << std::endl;
    if (!(date >= train.saleDateBegin && date <= train.saleDateEnd)) {
        throw Exception("query_train: train is not saled that dat");
    }

    printf("[%lld] %s %c\n", current_timestamp, train_id_.data, train.type);

    for (int i = 0; i < station_num; i++) {
        char arr_str[20], dep_str[20];

        if (i == 0) {
            strcpy(arr_str, "xx-xx xx:xx");
        } else {
            int arr_min = train.startTime + stations[i].travelTime + stations[i].stopTime;
            formatTime(date_.data, arr_min, arr_str);
        }

        if (i == station_num - 1) {
            strcpy(dep_str, "xx-xx xx:xx");
        } else {
            int dep_min = train.startTime + stations[i].travelTime + stations[i + 1].stopTime;
            formatTime(date_.data, dep_min, dep_str);
        }

        if (i == station_num - 1) {
            printf("%s %s -> %s %d x\n",
                   stations[i].stationName, arr_str, dep_str, stations[i].price);
        } else {
            printf("%s %s -> %s %d %d\n",
                   stations[i].stationName, arr_str, dep_str, stations[i].price, seats[i]);
        }
    }
}


QueryTicket::QueryTicket(int argc, char *argv[], shared_ptr<TrainManager> train_manager,
                         shared_ptr<SeatManager> seat_manager)
  : train_manager_(std::move(train_manager)), 
    seat_manager_(std::move(seat_manager)) {
    if (argc != 7 && argc != 9) {
        throw Exception("query_ticket: argc is not right");
    }
    for (int i = 1; i < argc; i += 2) {
        if (strcmp(argv[i], "-s") == 0) {
            start_station_.parser(argv[i + 1]);
        } else if (strcmp(argv[i], "-t") == 0) {
            end_station_.parser(argv[i + 1]);
        } else if (strcmp(argv[i], "-d") == 0) {
            date_.parser(argv[i + 1]);
        } else if (strcmp(argv[i], "-p") == 0) {
            sort_param_.parser(argv[i + 1]);
        } else {
            throw Exception("query_ticket: invalid arg");
        }
    }
    if (sort_param_.data[0] == '\0') {
        strcpy(sort_param_.data, "time");
    }
}

struct TicketResult {
    char trainID[TRAIN_ID_LEN + 1];
    char from[STATION_NAME_LEN * 5];
    char to[STATION_NAME_LEN * 5];
    char leave_time[20];
    char arrive_time[20];
    int price;
    int seat;
    int total_time;
};

void QueryTicket::execute() {
    int query_day;
    try {
        query_day = dateToDayOffset(date_.data);
    } catch (const Exception &) {
        printf("[%lld] 0\n", current_timestamp);
        return;
    }

    vector<StationLookupValue> trains_from = train_manager_->getTrainsByStation(start_station_.data);
    vector<StationLookupValue> trains_to = train_manager_->getTrainsByStation(end_station_.data);

    struct HashNode { 
        char trainID[TRAIN_ID_LEN + 1]; 
        int seq; 
        bool used; 
    };
    int to_size = 1;
    while (to_size < static_cast<int>(trains_to.size()) * 2) { 
        to_size <<= 1; 
    }
    int to_mask = to_size - 1;
    vector<HashNode> to_map;
    for (int k = 0; k < to_size; k++) { 
        to_map.push_back({}); 
    }

    for (size_t k = 0; k < trains_to.size(); k++) {
        int idx = hash_djb2(trains_to[k].trainID) & to_mask;
        while (to_map[idx].used) { 
            idx = (idx + 1) & to_mask; 
        }
        strcpy(to_map[idx].trainID, trains_to[k].trainID);
        to_map[idx].seq = trains_to[k].seq;
        to_map[idx].used = true;
    }

    vector<TicketResult> results;

    for (size_t i = 0; i < trains_from.size(); i++) {
        const char *tid = trains_from[i].trainID;
        int start_seq = trains_from[i].seq;

        int idx = hash_djb2(tid) & to_mask;
        int end_seq = -1;
        while (to_map[idx].used) {
            if (strcmp(to_map[idx].trainID, tid) == 0) {
                end_seq = to_map[idx].seq;
                break;
            }
            idx = (idx + 1) & to_mask;
        }
        if (end_seq == -1 || start_seq >= end_seq)  {
            continue;
        }

        auto train_data = train_manager_->getTrainData(tid);
        if (!train_data.meta.released) {
            continue;
        }

        int sale_begin = train_data.meta.saleDateBegin;
        int sale_end = train_data.meta.saleDateEnd;
        auto &stations = train_data.stations;

        int dep_s_min = train_data.meta.startTime + stations[start_seq].travelTime + stations[start_seq + 1].stopTime;
        int origin_day = query_day - dep_s_min / 1440;
        if (origin_day < sale_begin || origin_day > sale_end) {
            continue;
        }

        char origin_date[6];
        dayOffsetToDate(origin_day, origin_date);
        int arr_t_min = train_data.meta.startTime + stations[end_seq].travelTime + stations[end_seq].stopTime;
        int travel_time = arr_t_min - dep_s_min;
        int price = stations[end_seq].price - stations[start_seq].price;

        vector<int> seats = seat_manager_->getSeats(tid, origin_date);
        int min_seat = seats[start_seq];
        for (int s = start_seq; s < end_seq; s++) {
            if (seats[s] < min_seat) {
                min_seat = seats[s];
            }
        }

        TicketResult res;
        strcpy(res.trainID, tid);
        strcpy(res.from, start_station_.data);
        strcpy(res.to, end_station_.data);
        formatTime(origin_date, dep_s_min, res.leave_time);
        formatTime(origin_date, arr_t_min, res.arrive_time);
        res.price = price;
        res.seat = min_seat;
        res.total_time = travel_time;
        results.push_back(res);
    }

    bool sort_by_time = (strcmp(sort_param_.data, "time") == 0);
    for (size_t i = 1; i < results.size(); i++) {
        TicketResult tmp = results[i];
        int j = i - 1;
        while (j >= 0) {
            bool should_swap = false;
            if (sort_by_time) {
                if (results[j].total_time > tmp.total_time) should_swap = true;
                else if (results[j].total_time == tmp.total_time && strcmp(results[j].trainID, tmp.trainID) > 0) {
                    should_swap = true;
                }
            } else {
                if (results[j].price > tmp.price) should_swap = true;
                else if (results[j].price == tmp.price && strcmp(results[j].trainID, tmp.trainID) > 0) {
                    should_swap = true;
                }
            }
            if (!should_swap) {
                break;
            }
            results[j + 1] = results[j];
            j--;
        }
        results[j + 1] = tmp;
    }

    printf("[%lld] %zu\n", current_timestamp, results.size());
    for (size_t i = 0; i < results.size(); i++) {
        printf("%s %s %s -> %s %s %d %d\n", 
               results[i].trainID, results[i].from, results[i].leave_time,
               results[i].to, results[i].arrive_time, results[i].price, results[i].seat);
    }
}

QueryTransfer::QueryTransfer(int argc, char *argv[], shared_ptr<TrainManager> train_manager,
                             shared_ptr<SeatManager> seat_manager)
  : train_manager_(std::move(train_manager)), 
    seat_manager_(std::move(seat_manager)) {
    if (argc != 7 && argc != 9) {
        throw Exception("query_transfer: argc is not right");
    }
    for (int i = 1; i < argc; i += 2) {
        if (strcmp(argv[i], "-s") == 0) {
            start_station_.parser(argv[i + 1]);
        } else if (strcmp(argv[i], "-t") == 0) {
            end_station_.parser(argv[i + 1]);
        } else if (strcmp(argv[i], "-d") == 0) {
            date_.parser(argv[i + 1]);
        } else if (strcmp(argv[i], "-p") == 0) {
            sort_param_.parser(argv[i + 1]);
        } else {
            throw Exception("query_transfer: invalid arg");
        }
    }
    if (sort_param_.data[0] == '\0') {
        strcpy(sort_param_.data, "time");
    }
}

struct TransferResult {
    TicketResult first;
    TicketResult second;
    int total_time;
};

void QueryTransfer::execute() {
    vector<StationLookupValue> trains_from = train_manager_->getTrainsByStation(start_station_.data);
    vector<StationLookupValue> trains_to = train_manager_->getTrainsByStation(end_station_.data);
    bool sort_by_time = (strcmp(sort_param_.data, "time") == 0);

    TransferResult best;
    bool found = false;

    auto better = [sort_by_time](const TransferResult &a, const TransferResult &b) -> bool {
        int ta = a.total_time;
        int tb = b.total_time;
        int pa = a.first.price + a.second.price;
        int pb = b.first.price + b.second.price;
        if (sort_by_time) {
            if (ta != tb) return ta < tb;
            if (pa != pb) return pa < pb;
        } else {
            if (pa != pb) return pa < pb;
            if (ta != tb) return ta < tb;
        }
        int cmp = strcmp(a.first.trainID, b.first.trainID);
        if (cmp != 0) return cmp < 0;
        return strcmp(a.second.trainID, b.second.trainID) < 0;
    };

    int query_day;
    try {
        query_day = dateToDayOffset(date_.data);
    } catch (const Exception &) {
        printf("[%lld] 0\n", current_timestamp);
        return;
    }

    struct ToTrain {
        TrainRecord meta;
        vector<StationRecord> stations;
        int seq;
        int arr_t_min; // pre-computed
        char trainID[TRAIN_ID_LEN + 1];
    };

    vector<ToTrain> to_trains;

    struct HashBucket { 
        char stationName[STATION_NAME_LEN * 5 + 1]; 
        int head; 
        bool used; 
    };
    struct MatchEntry { 
        int train_idx; 
        int m2; 
        int next; 
    };

    int hs = 1;
    while (hs < static_cast<int>(trains_to.size()) * 200) {
        hs <<= 1;
    }
    int hmask = hs - 1;
    vector<HashBucket> htable;
    for (int k = 0; k < hs; k++) {
        HashBucket b = {};
        b.head = -1;
        htable.push_back(b);
    }
    vector<MatchEntry> match_entries;

    for (size_t j = 0; j < trains_to.size(); j++) {
        const char *tid2 = trains_to[j].trainID;
        int seq2 = trains_to[j].seq;

        auto train_data2 = train_manager_->getTrainData(tid2);
        if (!train_data2.meta.released) continue;

        ToTrain tt;
        tt.meta = train_data2.meta;
        tt.stations = train_data2.stations;
        tt.seq = seq2;
        tt.arr_t_min = train_data2.meta.startTime + train_data2.stations[seq2].travelTime + train_data2.stations[seq2].stopTime;
        strcpy(tt.trainID, tid2);
        int ti = to_trains.size();
        to_trains.push_back(tt);

        for (int m2 = 0; m2 < seq2; m2++) {
            const char *sname = train_data2.stations[m2].stationName;

            int idx = hash_djb2(sname) & hmask;
            int ins_start = idx;
            while (htable[idx].used && strcmp(htable[idx].stationName, sname) != 0) {
                idx = (idx + 1) & hmask;
                if (idx == ins_start) {
                    break;
                }
            }
            if (!htable[idx].used) {
                strcpy(htable[idx].stationName, sname);
                htable[idx].head = -1;
                htable[idx].used = true;
            }
            MatchEntry me;
            me.train_idx = ti;
            me.m2 = m2;
            me.next = htable[idx].head;
            htable[idx].head = match_entries.size();
            match_entries.push_back(me);
        }
    }

    for (size_t i = 0; i < trains_from.size(); i++) {
        const char *tid1 = trains_from[i].trainID;
        int seq1 = trains_from[i].seq;

        auto train_data1 = train_manager_->getTrainData(tid1);
        if (!train_data1.meta.released) {
            continue;
        }

        TrainRecord &train1 = train_data1.meta;
        vector<StationRecord> &st1 = train_data1.stations;
        if (st1.size() - 1 == seq1) {
            continue;
        }

        int dep_s_min = train1.startTime + st1[seq1].travelTime + st1[seq1 + 1].stopTime;
        int origin_day1 = query_day - dep_s_min / 1440;
        int sale_begin1 = train1.saleDateBegin;
        int sale_end1 = train1.saleDateEnd;
        if (origin_day1 < sale_begin1 || origin_day1 > sale_end1) {
            continue;
        }

        char origin_date1[6];
        dayOffsetToDate(origin_day1, origin_date1);

        vector<int> seats1_v = seat_manager_->getSeats(tid1, origin_date1);
        char leave1_formatted[20];
        formatTime(origin_date1, dep_s_min, leave1_formatted);

        for (int m1 = seq1 + 1; m1 < train1.stationNum; m1++) {
            int arr_m_min = train1.startTime + st1[m1].travelTime + st1[m1].stopTime;
            int travel1 = arr_m_min - dep_s_min;
            int price1 = st1[m1].price - st1[seq1].price;
            char arrive1_formatted[20];
            formatTime(origin_date1, arr_m_min, arrive1_formatted);

            int min_seat1 = seats1_v[seq1];
            for (int s = seq1; s < m1; s++) {
                if (seats1_v[s] < min_seat1) { 
                    min_seat1 = seats1_v[s]; 
                }
            }

            int idx = hash_djb2(st1[m1].stationName) & hmask;
            int start_idx = idx;
            while (htable[idx].used) {
                if (strcmp(htable[idx].stationName, st1[m1].stationName) == 0) {
                    for (int e = htable[idx].head; e != -1; e = match_entries[e].next) {
                        int ti = match_entries[e].train_idx;
                        int m2 = match_entries[e].m2;

                        ToTrain &tt = to_trains[ti];
                        if (strcmp(tid1, tt.trainID) == 0) {
                            continue;
                        }

                        int dep_m_min = tt.meta.startTime + tt.stations[m2].travelTime + tt.stations[m2 + 1].stopTime;
                        int diff = origin_day1 * 1440 + arr_m_min - dep_m_min;
                        int origin_day2 = (diff + 1439) / 1440;

                        int sale_begin2 = tt.meta.saleDateBegin;
                        int sale_end2 = tt.meta.saleDateEnd;
                        if (origin_day2 > sale_end2) continue;
                        origin_day2 = std::max(origin_day2, sale_begin2);
                        char origin_date2[6];
                        dayOffsetToDate(origin_day2, origin_date2);

                        int travel2 = tt.arr_t_min - dep_m_min;
                        int price2 = tt.stations[tt.seq].price - tt.stations[m2].price;

                        vector<int> seats2_v = seat_manager_->getSeats(tt.trainID, origin_date2);
                        int min_seat2 = seats2_v[m2];
                        for (int s = m2; s < tt.seq; s++) {
                            if (seats2_v[s] < min_seat2) { 
                                min_seat2 = seats2_v[s]; 
                            }
                        }

                        TransferResult cur;
                        strcpy(cur.first.trainID, tid1);
                        strcpy(cur.first.from, start_station_.data);
                        strcpy(cur.first.to, st1[m1].stationName);
                        strcpy(cur.first.leave_time, leave1_formatted);
                        strcpy(cur.first.arrive_time, arrive1_formatted);
                        cur.first.price = price1;
                        cur.first.seat = min_seat1;
                        cur.first.total_time = travel1;

                        strcpy(cur.second.trainID, tt.trainID);
                        strcpy(cur.second.from, tt.stations[m2].stationName);
                        strcpy(cur.second.to, end_station_.data);
                        formatTime(origin_date2, dep_m_min, cur.second.leave_time);
                        formatTime(origin_date2, tt.arr_t_min, cur.second.arrive_time);
                        cur.second.price = price2;
                        cur.second.seat = min_seat2;
                        cur.second.total_time = travel2;
                        cur.total_time = origin_day2 * 1440 + tt.arr_t_min - origin_day1 * 1440 - dep_s_min;
                        if (!found || better(cur, best)) {
                            best = cur;
                            found = true;
                        }
                    }
                    break;
                }
                idx = (idx + 1) & hmask;
                if (idx == start_idx) {
                    break;
                }
            }
        }
    }

    if (!found) {
        printf("[%lld] 0\n", current_timestamp);
    } else {
        printf("[%lld] %s %s %s -> %s %s %d %d\n",
               current_timestamp,
               best.first.trainID, best.first.from, best.first.leave_time,
               best.first.to, best.first.arrive_time, best.first.price, best.first.seat);
        printf("%s %s %s -> %s %s %d %d\n",
               best.second.trainID, best.second.from, best.second.leave_time,
               best.second.to, best.second.arrive_time, best.second.price, best.second.seat);
    }
}

BuyTicket::BuyTicket(int argc, char *argv[], shared_ptr<UserManager> user_manager,
                     shared_ptr<TrainManager> train_manager,
                     shared_ptr<SeatManager> seat_manager,
                     shared_ptr<OrderManager> order_manager)
  : user_manager_(std::move(user_manager)),
    train_manager_(std::move(train_manager)),
    seat_manager_(std::move(seat_manager)),
    order_manager_(std::move(order_manager)),
    queue_(false) {
    if (argc < 13 || argc > 15 || argc % 2 == 0) {
        throw Exception("buy_ticket: argc is not right");
    }
    for (int i = 1; i < argc; i += 2) {
        if (strcmp(argv[i], "-u") == 0) {
            user_name_.parser(argv[i + 1]);
        } else if (strcmp(argv[i], "-i") == 0) {
            train_id_.parser(argv[i + 1]);
        } else if (strcmp(argv[i], "-d") == 0) {
            date_.parser(argv[i + 1]);
        } else if (strcmp(argv[i], "-n") == 0) {
            num_.parser(argv[i + 1]);
        } else if (strcmp(argv[i], "-f") == 0) {
            from_station_.parser(argv[i + 1]);
        } else if (strcmp(argv[i], "-t") == 0) {
            to_station_.parser(argv[i + 1]);
        } else if (strcmp(argv[i], "-q") == 0) {
            if (strcmp(argv[i + 1], "true") == 0) {
                queue_ = true;
            } else if (strcmp(argv[i + 1], "false") == 0) {
                queue_ = false;
            } else {
                throw Exception("buy_ticket: -q should be true or false");
            }
        } else {
            throw Exception("buy_ticket: invalid arg");
        }
    }
}

void BuyTicket::execute() {
    //std::cerr << "strat buy ticket " << current_timestamp << std::endl;
    UserNode **ptr = user_manager_->getLoggedUser(user_name_.data);
    if (*ptr == nullptr) {
        throw Exception("buy_ticket: user is not logged");
    }

    auto train_data = train_manager_->getTrainData(train_id_.data);
    TrainRecord &train = train_data.meta;
    if (!train.released) {
        throw Exception("buy_ticket: train is not released");
    }
    if (train.seatNum < num_.data) {
        throw Exception("buy ticket: not enough seats");
    }

    vector<StationRecord> &stations = train_data.stations;
    int sn = train.stationNum;

    int from_seq = -1, to_seq = -1;
    for (int i = 0; i < sn; i++) {
        if (from_seq == -1 && strcmp(stations[i].stationName, from_station_.data) == 0) {
            from_seq = i;
        }
        if (from_seq != -1 && strcmp(stations[i].stationName, to_station_.data) == 0) {
            to_seq = i;
            break;
        }
    }
    if (from_seq == -1 || to_seq == -1 || from_seq >= to_seq) {
        throw Exception("buy_ticket: invalid station pair");
    }

    int arr_s_min = train.startTime + stations[from_seq].travelTime + stations[from_seq].stopTime;
    int dep_s_min = train.startTime + stations[from_seq].travelTime + stations[from_seq + 1].stopTime;
    int arr_t_min = train.startTime + stations[to_seq].travelTime + stations[to_seq].stopTime;
    int price = stations[to_seq].price - stations[from_seq].price;

    int query_day = dateToDayOffset(date_.data);
    int origin_day = query_day - dep_s_min / 1440;

    char origin_date[6];
    dayOffsetToDate(origin_day, origin_date);

    int sale_begin = train.saleDateBegin;
    int sale_end = train.saleDateEnd;
    if (origin_day < sale_begin || origin_day > sale_end) {
        throw Exception("buy_ticket: train not operating on this date");
    }
    //std::cerr << "check" << std::endl;
    bool deducted = seat_manager_->deductSeats(train_id_.data, origin_date,
                                                from_seq, to_seq, num_.data);
    //std::cerr << "check1" << " " << deducted << std::endl;
    long long timestamp = current_timestamp;

    if (deducted) {
        OrderRecord order;
        memset(&order, 0, sizeof(order));
        strcpy(order.trainID, train_id_.data);
        strcpy(order.from, from_station_.data);
        strcpy(order.to, to_station_.data);
        strcpy(order.date, origin_date);
        formatTime(origin_date, dep_s_min, order.leave_time);
        formatTime(origin_date, arr_t_min, order.arrive_time);
        order.fromSeq = from_seq;
        order.toSeq = to_seq;
        order.price = price;
        order.num = num_.data;
        order.status = kSuccess;
        order.timestamp = timestamp;
        order_manager_->addOrder(user_name_.data, order);
        printf("[%lld] %d\n", current_timestamp, price * num_.data);
    } else if (queue_) {
        OrderRecord order;
        memset(&order, 0, sizeof(order));
        strcpy(order.trainID, train_id_.data);
        strcpy(order.from, from_station_.data);
        strcpy(order.to, to_station_.data);
        strcpy(order.date, origin_date);
        formatTime(origin_date, dep_s_min, order.leave_time);
        formatTime(origin_date, arr_t_min, order.arrive_time);
        order.fromSeq = from_seq;
        order.toSeq = to_seq;
        order.price = price;
        order.num = num_.data;
        order.status = kPending;
        order.timestamp = timestamp;
        order_manager_->addOrder(user_name_.data, order);

        WaitlistRecord wl;
        memset(&wl, 0, sizeof(wl));
        strcpy(wl.username, user_name_.data);
        wl.fromSeq = from_seq;
        wl.toSeq = to_seq;
        wl.num = num_.data;
        wl.timestamp = timestamp;
        seat_manager_->addToWaitlist(train_id_.data, origin_date, wl);
        printf("[%lld] queue\n", current_timestamp);
    } else {
        throw Exception("buy_ticket: not enough seats");
    }
    //std::cerr << "finish" << std::endl;
}

QueryOrder::QueryOrder(int argc, char *argv[], shared_ptr<UserManager> user_manager,
                       shared_ptr<OrderManager> order_manager)
  : user_manager_(std::move(user_manager)), 
    order_manager_(std::move(order_manager)) {
    if (argc != 3 || argc % 2 == 0) {
        throw Exception("query_order: argc should be 3");
    }
    for (int i = 1; i < argc; i += 2) {
        if (strcmp(argv[i], "-u") == 0) {
            user_name_.parser(argv[i + 1]);
        } else {
            throw Exception("query_order: invalid arg");
        }
    }
}

void QueryOrder::execute() {
    //std::cerr << "query order" << " " << current_timestamp << std::endl;
    UserNode **ptr = user_manager_->getLoggedUser(user_name_.data);
    if (*ptr == nullptr) {
        throw Exception("query_order: user is not logged");
    }

    vector<OrderRecord> orders = order_manager_->getOrders(user_name_.data);
    printf("[%lld] %zu\n", current_timestamp, orders.size());
    ///std::cerr << "check" << std::endl;
    for (size_t i = 0; i < orders.size(); i++) {
        const char *status_str;
        if (orders[i].status == kSuccess) {
            status_str = "success";
        } else if (orders[i].status == kPending) {
            status_str = "pending";
        } else {
            status_str = "refunded";
        }

        printf("[%s] %s %s %s -> %s %s %d %d\n",
               status_str, orders[i].trainID,
               orders[i].from, orders[i].leave_time,
               orders[i].to, orders[i].arrive_time,
               orders[i].price, orders[i].num);
    }
    //std::cerr << "finish" << std::endl;
}

RefundTicket::RefundTicket(int argc, char *argv[], shared_ptr<UserManager> user_manager,
                           shared_ptr<OrderManager> order_manager,
                           shared_ptr<SeatManager> seat_manager,
                           shared_ptr<TrainManager> train_manager)
  : user_manager_(std::move(user_manager)),
    order_manager_(std::move(order_manager)),
    seat_manager_(std::move(seat_manager)),
    train_manager_(std::move(train_manager)) {
    if (argc != 3 && argc != 5) {
        throw Exception("refund_ticket: argc is not right");
    }
    for (int i = 1; i < argc; i += 2) {
        if (strcmp(argv[i], "-u") == 0) {
            user_name_.parser(argv[i + 1]);
        } else if (strcmp(argv[i], "-n") == 0) {
            n_.parser(argv[i + 1]);
        } else {
            throw Exception("refund_ticket: invalid arg");
        }
    }
    if (n_.data == 0) n_.data = 1;
}

void RefundTicket::execute() {
    //std::cerr << "start to runfund" << " " << current_timestamp << std::endl;
    UserNode **ptr = user_manager_->getLoggedUser(user_name_.data);
    if (*ptr == nullptr) {
        throw Exception("refund_ticket: user is not logged");
    }

    vector<OrderRecord> orders = order_manager_->getOrders(user_name_.data);
    int idx = n_.data - 1;
    if (idx < 0 || idx >= static_cast<int>(orders.size())) {
        throw Exception("refund_ticket: invalid order index");
    }
    //std::cerr << "check" << " " << orders[idx].status << std::endl;
    OrderRecord &order = orders[idx];
    if (order.status == kRefunded) {
        throw Exception("refund_ticket: order already refunded");
    }

    if (order.status == kSuccess) {
        seat_manager_->refundSeats(order.trainID, order.date, order.fromSeq, order.toSeq, order.num);
        auto fulfilled = seat_manager_->processWaitlist(order.trainID, order.date);
        //std::cerr << "check3" << std::endl;
        for (size_t i = 0; i < fulfilled.size(); i++) {
            order_manager_->updateOrderStatus(fulfilled[i].username, fulfilled[i].timestamp, kSuccess);
        }
        //std::cerr << "check4" << std::endl;
        order_manager_->updateOrderStatus(user_name_.data, order.timestamp, kRefunded);
    } else {
        seat_manager_->removeFromWaitlist(order.trainID, order.date, order.timestamp);
        order_manager_->updateOrderStatus(user_name_.data, order.timestamp, kRefunded);
    }

    printf("[%lld] 0\n", current_timestamp);
    //std::cerr << "finish" << std::endl;
}


Clean::Clean(int argc, char *argv[], shared_ptr<BufferPoolManager> bpm,
             shared_ptr<UserManager> user_manager,
             shared_ptr<TrainManager> train_manager,
             shared_ptr<OrderManager> order_manager,
             shared_ptr<SeatManager> seat_manager)
  : bpm_(std::move(bpm)),
    user_manager_(std::move(user_manager)),
    train_manager_(std::move(train_manager)),
    order_manager_(std::move(order_manager)),
    seat_manager_(std::move(seat_manager)) {
    if (argc != 1) {
        throw Exception("clean : should not have arg");
    }
}

void Clean::execute() {
    bpm_->ClearAll();
    user_manager_->Reset();
    train_manager_->Reset();
    order_manager_->Reset();
    seat_manager_->Reset();
    printf("[%lld] 0\n", current_timestamp);
}

extern bool should_exit;

Exit::Exit(int argc, char *argv[]) {
    if (argc != 1) {
        //std::cerr << argc << std::endl;
        throw Exception("exit : should not have arg");
    }
}

void Exit::execute() {
    printf("[%lld] bye\n", current_timestamp);
    should_exit = true;
}
