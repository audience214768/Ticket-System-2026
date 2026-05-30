#pragma once

#include "utils/err.h"
#include "utils/config.h"

inline auto checkLetter(char ch) -> bool {
    return ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z';
}

inline auto checkNum(char ch) -> bool {
    return ch >= '0' && ch <= '9';
}

inline auto checkPrintable(char ch) -> bool {
    unsigned char uch = static_cast<unsigned char>(ch);
    return uch >= 32 && uch <= 126;
}

inline auto checkChinese(const char *ch, int min_len, int max_len) -> bool {
    if (!ch || *ch == '\0') {
        return false;
    }
    int count = 0;
    const unsigned char *uch = reinterpret_cast<const unsigned char *>(ch);
    while(*uch != '\0') {
        if (count >= max_len) {
            return false;
        }
        //std::cerr << std::hex << (size_t)*uch << std::endl;
        if (*uch >= 0xE0 && *uch <= 0xEF) {
            if (*(uch + 1) == '\0' || *(uch + 2) == '\0') {
                return false;
            }
            if (*(uch + 1) < 0x80 || *(uch + 1) > 0xBF || 
                *(uch + 2) < 0x80 || *(uch + 2) > 0xBF) {
                return false;
            }
            count++;
            uch += 3;
        } else if (*uch >= 0xF0 && *uch <= 0xF4) {
            if (*(uch + 1) == '\0' || *(uch + 2) == '\0' || *(uch + 3) == '\0') {
                return false;
            }
            if (*(uch + 1) < 0x80 || *(uch + 1) > 0xBF || 
                *(uch + 2) < 0x80 || *(uch + 2) > 0xBF || 
                *(uch + 3) < 0x80 || *(uch + 3) > 0xBF) {
                return false;
            }
            count++;
            uch += 4;
        } else {
            return false;
        }
    }
    if (count >= min_len) {
        return true;
    }
    return false;
}


static constexpr short days[13] = {0, 31, 28, 31,30,31, 30, 31, 31,30,31, 30, 31};

inline auto dateToDayOffset(const char *date) -> int { 
    int mm = (date[0] - '0') * 10 + (date[1] - '0');
    int dd = (date[3] - '0') * 10 + (date[4] - '0');
    if (mm > 12) {
        throw Exception("invalid date");
    }
    int ret = 0;
    for (int i = 1; i < mm; i++) {
        ret += days[i];
    }
    return ret + dd;
}

inline void dayOffsetToDate(int offset, char *date) {
    int mm = 1, dd = offset;
    while(dd > days[mm] && mm <= 12) {
        dd -= days[mm];
        mm++;
    }
    if (mm > 12) {
        throw Exception("invalid date");
    }
    date[0] = '0' + mm / 10;
    date[1] = '0' + mm % 10;
    date[2] = '-';
    date[3] = '0' + dd / 10;
    date[4] = '0' + dd % 10;
    date[5] = '\0';
}

inline void formatTime(const char *base_date, int minutes, char *output) {
    int day_offset = dateToDayOffset(base_date);
    int new_offset = day_offset + minutes / 1440;
    //std::cerr << new_offset << std::endl;
    int remaining = minutes % 1440;
    char new_date[6];
    dayOffsetToDate(new_offset, new_date);
    int hh = remaining / 60;
    int mi = remaining % 60;
    snprintf(output, 20, "%s %02d:%02d", new_date, hh, mi);
}




struct UserName {
    char data[USER_NAME_LEN + 1] = {0};
    void parser(const char *input) {
        if (data[0] != '\0') {
            throw Exception("duplicate arg");
        }
        int len = strlen(input);
        if (len > 20) {
            throw Exception("username shuold within 20 char");
        }
        for (int i = 0; i < len; i++) {
            if (i == 0 && !checkLetter(input[i])) {
                //puts(input);
                throw Exception("the first char of username should be letter");
            }
            //std::cerr << input[i] << std::endl;
            if (!(checkLetter(input[i]) || checkNum(input[i]) || input[i] == '_')) {
                throw Exception("the char of username should consis of letter, num or '_'");
            }
        }
        strcpy(data, input);
    }
};

struct PassWord {
    char data[PWD_LEN + 1] = {0};
    void parser(const char *input) {
        if (data[0] != '\0') {
            throw Exception("duplicate arg");
        }
        int len = strlen(input);
        if (len < 1 || len > 30) {
            throw Exception("the len of password should be between 1 and 30");
        }
        //puts(input);
        for (int i = 0; i < len; i++) {
            if (!checkPrintable(input[i])) {
                //printf("%d\n", i);
                throw Exception("the char of password should be printable");
            }
        }
        strcpy(data, input);
    }
};

struct Name {
    char data[NAME_LEN * 5] = {0};
    void parser(const char *input) {
        if (data[0] != '\0') {
            throw Exception("duplicate arg");
        }
        //puts(input);
        if (!checkChinese(input, 2, 5)) {
            throw Exception("the name should be Chinese and len should between 2 and 5");
        }
        strcpy(data, input);
    }
};

struct MailAddress {
    char data[MAIL_ADDR_LEN + 1] = {0};
    void parser(const char *input) {
        if (data[0] != '\0') {
            throw Exception("duplicate arg");
        }
        int len = strlen(input);
        if (len > 30) {
            throw Exception("the mail-address should be shorter than 30 char");
        }
        for (int i = 0; i < len; i++) {
            if (!checkLetter(input[i]) && !checkNum(input[i]) && input[i] != '@' && input[i] != '.') {
                throw Exception("the mail-address should consist of num, letter, '@' or '.'");
            }
        }
        strcpy(data, input);
    }
};

struct Privilege {
    int data = -1;
    void parser(const char *input) {
        if (data != -1) {
            throw Exception("duplicate arg");
        }
        if (!checkNum(*input) || *(input + 1) != '\0') {
             throw Exception("privilege should be a num in 0..10");
        }
        data = *input - '0';
    }
};

struct TrainID {
    char data[TRAIN_ID_LEN + 1] = {0};
    void parser(const char *input) {
        if (data[0] != '\0') {
            throw Exception("dupilcate arg");
        } 
        if (strlen(input) > 20) {
            throw Exception("the len of trainID should be less than 20");
        }
        strcpy(data, input);
    }
};

inline auto StringToNum(const char *input) -> int {
    int data = 0;
    int len = strlen(input);
    for (int i = 0; i < len; i++) {
        if (!checkNum(input[i])) {
            //std::cerr << input[i] << std::endl;
            data = -1;
            throw Exception("not num");
        }
        data = data * 10 + input[i] - '0';
    }
    return data;
}

struct StationNum {
    int data = -1;
    void parser(const char *input) {
        if (data != -1) {
            throw Exception("duplicate arg");
        }
        data = StringToNum(input);
        if (data < 2 || data > 100) {
            throw Exception("stationNum should between 2 and 100");
        }
    }
};

struct SeatNum {
    int data = -1;
    void parser(const char *input) {
        if (data != -1) {
            throw Exception("duplicate arg");
        }
        data = StringToNum(input);
        if (data > 1e5) {
            throw Exception("seatNum should not more than 100000");
        }
    }
};


struct StationName {
    char data[STATION_NAME_LEN * 5] = {0};
    void parser(const char *input) {
        if (data[0] != '\0') {
            throw Exception("dupilcate arg");
        }
        if (!checkChinese(input, 1, 10)) {
            throw Exception("the station name should consist of chinese within 10 word");
        }
        strcpy(data, input);
    }
};

struct Price {
    int data = -1;
    void parser(const char *input) {
        if (data != -1) {
            throw Exception("duplicate arg");
        }
        data = StringToNum(input);
        if (data > 1e5) {
            throw Exception("the price should not more than 100000");
        }
    }
};

struct StartTime {
    short data = -1;
    void parser(const char *input) {
        if (data != -1) {
            throw Exception("duplicate arg");
        }
        if (strlen(input) != 5 || input[2] != ':'
            || !checkNum(input[0]) || !checkNum(input[1])
            || !checkNum(input[3]) || !checkNum(input[4])) {
            throw Exception("startTime should be hh:mm format");
        }
        int hh = (input[0] - '0') * 10 + (input[1] - '0');
        int mm = (input[3] - '0') * 10 + (input[4] - '0');
        if (hh > 23 || mm > 59) {
            throw Exception("startTime should be between 00:00 and 23:59");
        }
        data = hh * 60 + mm;
    }
};

struct SaleDate {
    short data = -1;
    void parser(const char *input) {
        if (data != -1) {
            throw Exception("duplicate arg");
        }
        if (strlen(input) != 5 || input[2] != '-'
            || !checkNum(input[0]) || !checkNum(input[1])
            || !checkNum(input[3]) || !checkNum(input[4])) {
            throw Exception("saleDate should be MM-DD format");
        }
        data = dateToDayOffset(input);
        if (data == -1) {
            throw Exception("invalid date");
        }
    }
};

struct TravelTime {
    int data = -1;
    void parser(const char *input) {
        if (data != -1) {
            throw Exception("duplicate arg");
        }
        data = StringToNum(input);
        if (data > 1e4) {
            throw Exception("travel time should not more than 10000");
        }
    }
};

struct StopoverTime {
    int data = -1;
    void parser(const char *input) {
        if (data != -1) {
            throw Exception("duplicate arg");
        }
        data = StringToNum(input);
        if (data > 1e4) {
            throw Exception("stop over time should not more than 10000");
        }
    }
};

struct TrainType {
    char data = '\0';
    void parser(const char *input) {
        if (data != '\0') {
            throw Exception("duplicate arg");
        }
        if (strlen(input) != 1 || input[0] < 'A' || input[0] > 'Z') {
            throw Exception("train type should be a single uppercase letter");
        }
        data = input[0];
    }
};

struct Date {
    char data[6] = {0};
    void parser(const char *input) {
        if (data[0] != '\0') {
            throw Exception("duplicate arg");
        }
        if (strlen(input) != 5 || input[2] != '-'
            || !checkNum(input[0]) || !checkNum(input[1])
            || !checkNum(input[3]) || !checkNum(input[4])) {
            throw Exception("date should be mm-dd format");
        }
        strcpy(data, input);
    }
};

struct SortParam {
    char data[5] = {0};
    void parser(const char *input) {
        if (data[0] != '\0') {
            throw Exception("duplicate arg");
        }
        if (strcmp(input, "time") != 0 && strcmp(input, "cost") != 0) {
            throw Exception("sort param should be time or cost");
        }
        strcpy(data, input);
    }
};

struct Num {
    int data = 0;
    void parser(const char *input) {
        if (data != 0) {
            throw Exception("duplicate arg");
        }
        data = StringToNum(input);
        if (data <= 0) {
            throw Exception("num should be positive");
        }
    }
};