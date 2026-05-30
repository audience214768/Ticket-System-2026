#include <cstdio>
#include <cstdlib>
#include "buffer/buffer_pool_manager.h"
#include "utils/err.h"
#include "disk/disk_manager.h"
#include "manager/train_manager.h"
#include "manager/user_manager.h"
#include "manager/order_manager.h"
#include "manager/seat_manager.h"
#include "core/ticket_system.h"
#include "shared_ptr/shared_ptr.hpp"
#include "vector/vector.hpp"

using sjtu::vector;
using sjtu::shared_ptr;
using sjtu::make_shared;
char input[2000];
constexpr int MAX_ARG_NUM = 30;
char *argv[MAX_ARG_NUM];
long long current_timestamp;

bool should_exit = false;

shared_ptr<UserManager> user_manager;
shared_ptr<TrainManager> train_manager;
shared_ptr<OrderManager> order_manager;
shared_ptr<SeatManager> seat_manager;
shared_ptr<BufferPoolManager> bpm;

void init() {

    vector<shared_ptr<DiskManager>> disk_manager;
    disk_manager.emplace_back(make_shared<DiskManager>(0, "user_data"));
    disk_manager.emplace_back(make_shared<DiskManager>(1, "user_name_index"));
    disk_manager.emplace_back(make_shared<DiskManager>(2, "train_index"));
    disk_manager.emplace_back(make_shared<DiskManager>(3, "station_data"));
    disk_manager.emplace_back(make_shared<DiskManager>(4, "station_lookup"));
    disk_manager.emplace_back(make_shared<DiskManager>(5, "order_data"));
    disk_manager.emplace_back(make_shared<DiskManager>(6, "order_index"));
    disk_manager.emplace_back(make_shared<DiskManager>(7, "seat_index"));
    disk_manager.emplace_back(make_shared<DiskManager>(8, "waitlist_data"));
    disk_manager.emplace_back(make_shared<DiskManager>(9, "waitlist_index"));
    bpm = make_shared<BufferPoolManager>(2100, disk_manager);
    user_manager = make_shared<UserManager>(bpm, 0, 1);
    train_manager = make_shared<TrainManager>(bpm, 2, 3, 4);
    order_manager = make_shared<OrderManager>(bpm, 5, 6);
    seat_manager = make_shared<SeatManager>(bpm, 7, 8, 9);
}

auto parser(char *input) -> Command* {
    int argc = 1;
    argv[0] = strtok(input, " \n\r\t");
    if (argv[0] == nullptr) {
        throw Exception("empty command");
    }
    while ((argv[argc++] = strtok(nullptr, " \n\r\t"))) {
        if (argc >= MAX_ARG_NUM) {
            throw Exception("too many arg");
        }
    };
    argc--;
    if (strcmp(argv[0], "add_user") == 0) {
        return new AddUser(argc, argv, user_manager);
    }
    if (strcmp(argv[0], "login") == 0) {
        return new LogIn(argc, argv, user_manager);
    }
    if (strcmp(argv[0], "logout") == 0) {
        return new LogOut(argc, argv, user_manager);
    }
    if (strcmp(argv[0], "query_profile") == 0) {
        return new QueryProfile(argc, argv, user_manager);
    }
    if (strcmp(argv[0], "modify_profile") == 0) {
        return new ModifyProfile(argc, argv, user_manager);
    }
    if (strcmp(argv[0], "add_train") == 0) {
        return new AddTrain(argc, argv, train_manager);
    }
    if (strcmp(argv[0], "delete_train") == 0) {
        return new DeleteTrain(argc, argv, train_manager);
    }
    if (strcmp(argv[0], "release_train") == 0) {
        return new ReleaseTrain(argc, argv, train_manager, seat_manager);
    }
    if (strcmp(argv[0], "query_train") == 0) {
        return new QueryTrain(argc, argv, train_manager, seat_manager);
    }
    if (strcmp(argv[0], "query_ticket") == 0) {
        return new QueryTicket(argc, argv, train_manager, seat_manager);
    }
    if (strcmp(argv[0], "query_transfer") == 0) {
        return new QueryTransfer(argc, argv, train_manager, seat_manager);
    }
    if (strcmp(argv[0], "buy_ticket") == 0) {
        return new BuyTicket(argc, argv, user_manager, train_manager, seat_manager, order_manager);
    }
    if (strcmp(argv[0], "query_order") == 0) {
        return new QueryOrder(argc, argv, user_manager, order_manager);
    }
    if (strcmp(argv[0], "refund_ticket") == 0) {
        return new RefundTicket(argc, argv, user_manager, order_manager, seat_manager, train_manager);
    }
    if (strcmp(argv[0], "clean") == 0) {
        return new Clean(argc, argv, bpm, user_manager, train_manager, order_manager, seat_manager);
    }
    if (strcmp(argv[0], "exit") == 0) {
        return new Exit(argc, argv);
    }
    throw Exception("invalid command");
}

int main() {
    init();
    while (!should_exit && fgets(input, sizeof(input), stdin)) {
        char *line = input;
        while (*line == ' ' || *line == '\t' || *line == '\r') {
            line++;
        }
        if (*line == '\0' || *line == '\n') {
            continue;
        }
        if (*line == '[') {
            current_timestamp = strtoll(line + 1, &line, 10);
            line++;
            while (*line == ' ') {
                line++;
            }
        }
        Command *command = nullptr;
        try {
            command = parser(line);
            command->execute();
            //std::cerr << current_timestamp << " success" << std::endl;
        } catch(const Exception &err) {
            printf("[%lld] -1\n", current_timestamp);
            //std::cerr << current_timestamp << " " << err.what() << std::endl;
        }
        delete command;
        //std::cerr << std::endl;
    }
}
