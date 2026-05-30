#pragma once

#include "storage/buffer/buffer_pool_manager.h"
#include "manager/train_manager.h"
#include "manager/user_manager.h"
#include "manager/order_manager.h"
#include "manager/seat_manager.h"
#include "shared_ptr/shared_ptr.hpp"
#include "utils/types.h"
#include "parser/parser.h"

using sjtu::shared_ptr;

class Command {
  public:
    virtual void execute() = 0;
    virtual ~Command() = default;
};

class AddUser : public Command {
  private:
    UserName cur_user_name_;
    UserName user_name_;
    PassWord pwd_;
    Name name_;
    MailAddress mail_addr_;
    Privilege priv_;
    shared_ptr<UserManager> user_manager_;
  public:
    AddUser(int argc, char *argv[], shared_ptr<UserManager> user_manager);
    void execute();
};

class LogIn : public Command {
  private:
    UserName user_name_;
    PassWord pwd_;
    shared_ptr<UserManager> user_manager_;
  public:
    LogIn(int argc, char *argv[], shared_ptr<UserManager> user_manager);
    void execute();
};

class LogOut : public Command {
  private:
    UserName user_name_;
    shared_ptr<UserManager> user_manager_;
  public:
    LogOut(int argc, char *argv[], shared_ptr<UserManager> user_manager);
    void execute();
};

class QueryProfile : public Command {
  private:
    UserName cur_user_name_;
    UserName user_name_;
    shared_ptr<UserManager> user_manager_;
  public:
    QueryProfile(int argc, char *argv[], shared_ptr<UserManager> user_manager);
    void execute();
};

class ModifyProfile : public Command {
  private:
    UserName cur_user_name_;
    UserName user_name_;
    PassWord new_pwd_;
    Name new_name_;
    MailAddress new_mail_addr_;
    Privilege new_priv_;
    shared_ptr<UserManager> user_manager_;
  public:
    ModifyProfile(int argc, char *argv[], shared_ptr<UserManager> user_manager);
    void execute();
};

class AddTrain : public Command {
  private:
    TrainID train_id_;
    StationNum station_num_;
    SeatNum seat_num_;
    vector<StationName> stations_name;
    vector<Price> prices_;
    StartTime start_time_;
    vector<TravelTime> travel_times_;
    vector<StopoverTime> stop_over_times_;
    SaleDate start_sale_date_;
    SaleDate end_sale_date_;
    TrainType type_;
    shared_ptr<TrainManager> train_manager_;
  public:
    AddTrain(int argc, char *argv[], shared_ptr<TrainManager> train_manager);
    void execute();
};

class DeleteTrain : public Command {
  private:
    TrainID train_id_;
    shared_ptr<TrainManager> train_manager_;
  public:
    DeleteTrain(int argc, char *argv[], shared_ptr<TrainManager> train_manager);
    void execute();
};

class ReleaseTrain : public Command {
  private:
    TrainID train_id_;
    shared_ptr<TrainManager> train_manager_;
    shared_ptr<SeatManager> seat_manager_;
  public:
    ReleaseTrain(int argc, char *argv[], shared_ptr<TrainManager> train_manager,
                 shared_ptr<SeatManager> seat_manager);
    void execute();
};

class QueryTrain : public Command {
  private:
    TrainID train_id_;
    Date date_;
    shared_ptr<TrainManager> train_manager_;
    shared_ptr<SeatManager> seat_manager_;
  public:
    QueryTrain(int argc, char *argv[], shared_ptr<TrainManager> train_manager,
               shared_ptr<SeatManager> seat_manager);
    void execute();
};

class QueryTicket : public Command {
  private:
    StationName start_station_;
    StationName end_station_;
    Date date_;
    SortParam sort_param_;
    shared_ptr<TrainManager> train_manager_;
    shared_ptr<SeatManager> seat_manager_;
  public:
    QueryTicket(int argc, char *argv[], shared_ptr<TrainManager> train_manager,
                shared_ptr<SeatManager> seat_manager);
    void execute();
};

class QueryTransfer : public Command {
  private:
    StationName start_station_;
    StationName end_station_;
    Date date_;
    SortParam sort_param_;
    shared_ptr<TrainManager> train_manager_;
    shared_ptr<SeatManager> seat_manager_;
  public:
    QueryTransfer(int argc, char *argv[], shared_ptr<TrainManager> train_manager,
                  shared_ptr<SeatManager> seat_manager);
    void execute();
};

class BuyTicket : public Command {
  private:
    UserName user_name_;
    TrainID train_id_;
    Date date_;
    Num num_;
    StationName from_station_;
    StationName to_station_;
    bool queue_;
    shared_ptr<UserManager> user_manager_;
    shared_ptr<TrainManager> train_manager_;
    shared_ptr<SeatManager> seat_manager_;
    shared_ptr<OrderManager> order_manager_;
  public:
    BuyTicket(int argc, char *argv[], shared_ptr<UserManager> user_manager,
              shared_ptr<TrainManager> train_manager,
              shared_ptr<SeatManager> seat_manager,
              shared_ptr<OrderManager> order_manager);
    void execute();
};

class QueryOrder : public Command {
  private:
    UserName user_name_;
    shared_ptr<UserManager> user_manager_;
    shared_ptr<OrderManager> order_manager_;
  public:
    QueryOrder(int argc, char *argv[], shared_ptr<UserManager> user_manager,
               shared_ptr<OrderManager> order_manager);
    void execute();
};

class RefundTicket : public Command {
  private:
    UserName user_name_;
    Num n_;
    shared_ptr<UserManager> user_manager_;
    shared_ptr<OrderManager> order_manager_;
    shared_ptr<SeatManager> seat_manager_;
    shared_ptr<TrainManager> train_manager_;
  public:
    RefundTicket(int argc, char *argv[], shared_ptr<UserManager> user_manager,
                 shared_ptr<OrderManager> order_manager,
                 shared_ptr<SeatManager> seat_manager,
                 shared_ptr<TrainManager> train_manager);
    void execute();
};

class Clean : public Command {
  private:
    shared_ptr<BufferPoolManager> bpm_;
    shared_ptr<UserManager> user_manager_;
    shared_ptr<TrainManager> train_manager_;
    shared_ptr<OrderManager> order_manager_;
    shared_ptr<SeatManager> seat_manager_;
  public:
    Clean(int argc, char *argv[], shared_ptr<BufferPoolManager> bpm,
          shared_ptr<UserManager> user_manager,
          shared_ptr<TrainManager> train_manager,
          shared_ptr<OrderManager> order_manager,
          shared_ptr<SeatManager> seat_manager);
    void execute();
};

class Exit : public Command {
  public:
    Exit(int argc, char *argv[]);
    void execute();
};
