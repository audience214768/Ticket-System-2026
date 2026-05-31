# 火车票订票系统 (Train Ticket Booking System)

类似 [12306](https://www.12306.cn/) 的火车票订票系统，基于 C++ 实现，采用 B+ 树索引和磁盘持久化存储。数据结构课程项目。

## 功能

- **用户管理**：注册、登录/登出、查询/修改个人信息、权限控制
- **列车管理**：添加、删除、发布、查询车次（含多站经停信息）
- **车票操作**：查询直达/换乘车票、购票（含余票扣减与候补队列）、退票、查询订单
- **持久化存储**：所有数据通过自定义缓冲池与 B+ 树索引持久化到磁盘，程序重启后数据不丢失
- **命令行交互**：基于标准输入输出的时间戳命令接口

## 项目结构

```
ticket-system/
├── CMakeLists.txt              # 顶层构建文件
├── include/                    # 头文件
│   ├── core/                   # 命令系统 (AddUser, BuyTicket, …)
│   ├── manager/                # 各管理器 (user, train, order, seat)
│   ├── parser/                 # 输入解析与类型校验
│   ├── storage/                # 存储层
│   │   ├── buffer/             # 缓冲池 (LRU-K/Clock 换出策略)
│   │   ├── common/             # 存储常量 (页大小 4096B)
│   │   ├── disk/               # 磁盘 I/O (异步调度器)
│   │   ├── index/              # B+ 树索引模板
│   │   ├── page/               # B+ 树页面结构 (叶/内部节点)
│   │   └── type/               # ComposedKey + 比较器
│   └── utils/                  # 应用常量、哈希函数、类型定义
├── src/                        # 源文件
│   ├── main.cpp                # 入口: init() → 命令解析循环
│   ├── core/                   # 所有命令实现
│   ├── manager/                # 管理器实现
│   └── storage/                # 存储层实现
├── lib/my-STL/                 # 自定义 STL 替代库 (header-only)
│   ├── vector/vector.hpp       # sjtu::vector
│   ├── deque/deque.hpp         # sjtu::deque
│   └── shared_ptr/shared_ptr.hpp  # sjtu::shared_ptr
├── test/                       # 测试用例 (102个测试点)
│   └── run_tests.py            # Python 测试运行器
├── build/                      # 构建输出目录
└── management_system.md        # 完整需求规格文档
```

## 架构设计

系统采用分层架构，自底向上为：数据类型 → 存储引擎 → 管理器 → 命令层。

```
┌─────────────────────────────────────┐
│  命令层 (core/ticket_system.cpp)     │  ← 命令解析、执行、异常处理
├─────────────────────────────────────┤
│  管理器层 (manager/)                 │  ← 业务逻辑：用户/列车/订单/座位
├─────────────────────────────────────┤
│  存储引擎 (storage/)                 │  ← B+ 树 → 缓冲池 → 磁盘 I/O
├─────────────────────────────────────┤
│  数据类型 (type/, utils/types.h)     │  ← ComposedKey, Compare, 记录结构体
└─────────────────────────────────────┘
```

---

## 一、数据类型

### 1.1 键值类型 (`include/storage/type/type.hpp`)

#### FixedString\<len\>

定长字符串模板。`char key[len]` 作为 B+ 树键的固定部分，天然支持 memcmp 比较，避免 std::string 的堆分配开销。

```cpp
template <size_t len>
struct FixedString {
  char key[len] = {0};
};
```

#### ComposedKey\<len\>

B+ 树统一键类型，由两部分组成。通过 `rid` 区分相同 `fixed_key` 的不同记录（如列车多个经停站），实现 B+ 树的重复键支持：

| 字段 | 类型 | 说明 |
|------|------|------|
| `fixed_key` | `FixedString<len>` | 定长字符串主键，如 trainID、username、stationName |
| `rid` | `int` | 记录 ID 或序列号，`RID_MIN = INT_MIN` 作为查询哨兵 |

**哨兵约定**：当 `rid = RID_MIN` 时天然排在所有键之前，等价于前缀查询。所有 `GetValue` 和 `ScanAndUpdate` 均通过传入 `RID_MIN` 来匹配同一 `fixed_key` 下的全部记录。

### 1.2 应用记录结构体 (`include/utils/types.h`)

#### 用户相关

| 结构体 | 字段 | 说明 |
|--------|------|------|
| `UserRecord` | `password[31]`, `name[25]`, `mailAddr[31]`, `privilege` | 持久化用户信息，privilege 控制权限等级 |
| `UserNode` | `user_name[21]`, `priv`, `next` | 登录态链表节点，挂载于哈希表的桶上 |

#### 列车相关

| 结构体 | 字段 | 说明 |
|--------|------|------|
| `TrainRecord` | `stationNum`, `seatNum`, `startTime`, `saleDateBegin/End`, `type`, `released` | 列车元信息（16 字节），与站点数据存放在同一页中 |
| `StationRecord` | `stationName[50]`, `price`, `travelTime`, `stopTime`, `lookup_rid` | 经停站信息，累计旅行/停站时间用于票价计算；`lookup_rid` 为时间戳，用于站点反查索引的删除 |
| `StationLookupValue` | `trainID[21]`, `seq` | 站点→列车反查结果，seq 为站序（0-based） |

#### 订单相关

| 结构体 | 字段 | 说明 |
|--------|------|------|
| `OrderRecord` | `trainID`, `from[50]`, `to[50]`, `date[6]`, `leave_time[20]`, `arrive_time[20]`, `fromSeq`, `toSeq`, `price`, `num`, `status`, `timestamp` | 完整订单信息，status 枚举：`kSuccess` / `kPending` / `kRefunded` |
| `WaitlistRecord` | `username[21]`, `fromSeq`, `toSeq`, `num`, `timestamp` | 候补记录，timestamp 用于唯一标识和排序 |

---

## 二、存储引擎 (`include/storage/`)

### 2.1 B+ 树索引 (`storage/index/b_plus_tree.h`)

模板类 `BPlusTree<KeyType, ValueType, Compare>` 是系统的核心索引结构：

- **两种遍历模式**：乐观遍历（共享读锁，无分裂时高效）和悲观遍历（写锁，保证分裂安全）
- **页面类型**：叶节点存键值对 + `next_page_id` 链表指针，内部节点存路由键 + 子页 ID
- **操作集**：
  - `Insert(key, value)` — 插入键值对，必要时触发分裂
  - `Remove(key)` — 删除键值对，必要时触发合并
  - `Update(key, value)` — 更新单条值
  - `GetValue(key, &result)` — 前缀查询，获取同一 `fixed_key` 下的所有值
  - `ScanAndUpdate(key, func)` — **单次遍历完成多区段原地更新**，适用于余票扣减等批量操作
- **哨兵机制**：内部节点的首键为 `RID_MIN` 哨兵，简化分裂合并逻辑

### 2.2 缓冲池 (`storage/buffer/buffer_pool_manager.h`)

将磁盘页缓存于内存的中间层：

- **帧数**：1500 帧，每帧 4096 字节，总计约 6MB 内存占用
- **哈希表**：线性探测法，映射 `page_id → frame_id`。采用 XOR 折叠 `hash_page(page_id) = (page_id ^ (page_id >> FILE_BIT)) & mask` 避免跨文件同页号碰撞。含墓碑机制 + **重哈希**，防止长期运行后探测链退化
- **换出策略**：Clock 算法，通过引用位近似最近未使用
- **页面守卫**：`ReadPageGuard` / `WritePageGuard` RAII 守卫，出作用域自动解锁和解引用（`shared_mutex`），异常安全
- **并发控制**：全局 `bpm_mutex_` 保护哈希表和空闲链表，每帧独立 `shared_mutex` 控制读写并发

### 2.3 磁盘管理 (`storage/disk/`)

- **DiskManager**：按页寻址（`offset = page_id × 4096`），直接文件 I/O，单文件最大 512MB（131072 页 × 4096 字节）
- **DiskScheduler**：异步 I/O 调度器，生产者-消费者模型 + 工作线程，读/写/分配/删除操作入队后通过 `promise/future` 返回结果
- **页 ID 编码**：`page_id = (fileID << 17) | page_index`，高 17 位为文件号，低 17 位为页号。系统共 10 个文件：
  `user_data`, `user_name_index`, `train_index`, `station_data`, `station_lookup`,
  `order_data`, `order_index`, `seat_index`, `waitlist_data`, `waitlist_index`

---

## 三、管理器层 (`include/manager/`)

### 3.1 UserManager — 用户管理器

管理用户注册、登录、信息查询与修改。

| 组件 | 说明 |
|------|------|
| `user_name_index_` | B+ 树，键 `ComposedKey<USER_NAME_LEN+1>` (username)，值 `size_t` (指向用户数据页的 record_id) |
| `UserListPage` | 数据页，每页容纳约 131 个 `UserRecord`，通过 `last_index` 追加 |
| `UserListHeaderPage` | 头页，记录 `last_page` 和 `have_user` 标志位 |
| `log_table[1024]` | 登录态哈希表，10 位寻址，挂链处理冲突。`login()` 插入，`logout()` 删除，`exit` 时全部清空 |

**操作流程**：
- `addUser` → 检查 `user_name_index_` 无重复 → 追加到用户数据页 → 索引插入
- `login` → 校验密码 → 查 `log_table` 防重复登录 → 插入登录节点
- `queryProfile / modifyProfile` → 索引找到 record_id → 读取/写入数据页

### 3.2 TrainManager — 列车管理器

管理列车元信息和经停站数据，支持站点反查。

| 组件 | 说明 |
|------|------|
| `train_index_` | B+ 树，键 `ComposedKey<TRAIN_ID_LEN+1>` (trainID)，值 `page_id_t` (列车首页) |
| `station_lookup_index_` | B+ 树，键 `ComposedKey<STATION_NAME_LEN*5>` (stationName)，值 `StationLookupValue{trainID, seq}`，用于 query_ticket 的起始站直达车次查找 |
| 列车首页 | 前 16 字节 `TrainRecord` + 8 字节 page1 指针 + `StationRecord[]`（约 64 站） |
| 列车次页 | 额外 `StationRecord[]`（约 69 站），当站点数超过首页容量时分配 |

**数据布局设计**：1 列火车至多 2 页（首页 + 次页），最小化离散 I/O。
首页 `StationRecord[0]` 位于固定偏移 `TRAIN_STATIONS_OFFSET`（24 字节处），
次页地址从首页固定偏移 `TRAIN_PAGE1_OFFSET`（16 字节处）读取。

### 3.3 SeatManager — 座位管理器

管理每趟列车每日的区段余票和候补队列。

| 组件 | 说明 |
|------|------|
| `seat_index_` | B+ 树，键 `ComposedKey<SEAT_KEY_LEN>` (trainID[20] + date[6])，rid = 区段序号，值 `int`（该区段剩余座位数） |
| `waitlist_index_` | B+ 树，键同上，rid = 时间戳，值 `size_t`（指向候补数据页的 record_id） |
| `WaitlistPage` | 候补数据页，每页约 146 条 `WaitlistRecord`，追加写入 + 头页 last_page 指针 |
| `WaitlistPageHeader` | 头页，记录 `last_page` 和 magic number |

**操作流程**：

- `initSeats` → 对售票期内每天、每区段插入 `seatNum` 条记录
- `deductSeats` → 先 GetValue 获取各区段余票 → 检查是否足够 → `ScanAndUpdate` 原地扣减各区段
- `refundSeats` → `ScanAndUpdate` 原地恢复各区段余票 → `processWaitlist` 依次检查候补队列能否满足
- `addToWaitlist` → 追加到 `WaitlistPage` → waitlist_index_ 插入索引
- `processWaitlist` → 遍历候补记录 → 对每个候补检查各区段余票 → 足够则 `ScanAndUpdate` 扣减并出队

### 3.4 OrderManager — 订单管理器

管理用户订单的增、查、改。

| 组件 | 说明 |
|------|------|
| `order_index_` | B+ 树，键 `ComposedKey<USER_NAME_LEN+1>` (username)，值 `size_t` (指向订单数据页的 record_id) |
| `OrderPage` | 数据页，每页约 47 条 `OrderRecord`，追加写入 |
| `OrderPageHeader` | 头页，记录 `last_page` 和 magic number |

**操作流程**：

- `addOrder` → 追加到 `OrderPage` → order_index_ 插入索引，返回 record_id
- `getOrders` → 索引获取所有 record_id → 逐页读取，利用页面守卫缓存避免同一页重复 I/O
- `updateOrderStatus` → 通过 record_id 定位 `page_id + offset` → 修改 status 字段

---

## 四、命令层 (`include/core/ticket_system.h`)

所有命令继承抽象基类 `Command`，构造函数解析参数，`execute()` 执行逻辑。

| 命令 | 频率 | 说明 |
|------|------|------|
| `AddUser` | | 注册用户 |
| `Login / Logout` | | 登入/登出 |
| `QueryProfile / ModifyProfile` | SF | 查询/修改个人信息，高频指令 |
| `AddTrain / DeleteTrain / ReleaseTrain` | | 列车增删发布 |
| `QueryTrain` | | 查询车次及经停信息 |
| `QueryTicket` | SF | 查询直达车票，核心高频指令。使用 **哈希连接**（DJB2 哈希映射 trainID→seq，O(N+M)）替代排序合并 |
| `QueryTransfer` | N | 查询换乘车票，中转站枚举。预取 trains_to 数据到平铺数组消除重复 getTrainData 调用 |
| `BuyTicket` | SF | 购票，高频指令。含余票检查、扣减、候补入队 |
| `QueryOrder / RefundTicket` | | 查询订单 / 退票（含候补处理） |
| `Clean / Exit` | | 清理所有数据 / 退出（清空登录态） |

## 优化要点

- **BPM 哈希表**：XOR 折叠 + 重哈希机制，避免跨文件页号碰撞与墓碑累积
- **TrainData 合并**：`getTrain` + `getStations` → `getTrainData`，减少重复 B+ 树遍历
- **ScanAndUpdate**：单次 B+ 树遍历完成多区段订座/退票，替代逐区段 Update
- **哈希连接**：`QueryTicket` 使用 DJB2 哈希映射替代 O(N²) 排序合并
- **预取平铺**：`QueryTransfer` 用平铺数组缓存 trains_to 数据，消除嵌套循环中的重复查询

