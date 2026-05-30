#pragma once

#include <cstring>

#include "utils/config.h"
#include "vector/vector.hpp"

using sjtu::vector;

struct UserRecord {
    char password[PWD_LEN + 1] = {0};    
    char name[NAME_LEN * 5] = {0};
    char mailAddr[MAIL_ADDR_LEN + 1] = {0};
    int privilege;
};


struct UserNode {             //use for log table
    char user_name[USER_NAME_LEN + 1];
    int priv;
    UserNode *next;
};

struct TrainRecord {
    int stationNum;
    int seatNum;
    short startTime;       // minutes
    short saleDateBegin;
    short saleDateEnd;
    char type;
    bool released;
};

struct StationRecord {
    char stationName[STATION_NAME_LEN * 5];
    int price;             // price from origin
    int travelTime;        // travel time from origin
    int stopTime;          // cumulative stopover time before arriving at this station
    size_t lookup_rid;     // counter value for station_lookup deletion
};

struct StationLookupValue {
    char trainID[TRAIN_ID_LEN + 1];
    int seq;               // station index in this train's route (0-based)
};

enum OrderStatus {kSuccess = 0, kPending, kRefunded};

struct OrderRecord {
    char trainID[TRAIN_ID_LEN + 1];
    char from[STATION_NAME_LEN * 5];
    char to[STATION_NAME_LEN * 5];
    char date[6];
    char leave_time[20];
    char arrive_time[20];
    int fromSeq;
    int toSeq;
    int price;
    int num;
    OrderStatus status;
    long long timestamp;
};

struct WaitlistRecord {
    char username[USER_NAME_LEN + 1];
    int fromSeq;
    int toSeq;
    int num;
    long long timestamp;
};


