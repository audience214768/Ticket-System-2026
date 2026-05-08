#include <cstdio>
#include <filesystem>
#include <gtest/gtest.h>
#include <iostream>
#include "index/b_plus_tree.h" // 确保路径正确
#include "buffer/buffer_pool_manager.h"
#include "disk/disk_manager.h"
//#include "shared_ptr/shared_ptr.hpp"
#include "vector/vector.hpp"
#include "common/config.h"
#include "type/type.hpp"
#include <random>
#include <thread>

using sjtu::vector;
using sjtu::make_shared;
using sjtu::shared_ptr;

using std::random_device;

// 这里的宏定义需要根据你具体的模板参数调整
// 假设 KeyType 是 int, ValueType 是 int, NumTombs 是某个数值
#define TEST_KEY_TYPE ComposedKey<40>
#define TEST_VALUE_TYPE int
#define TEST_NUM_TOMBS 0

class BPlusTreeTest : public ::testing::Test {
 protected:
  // 每跑一个 TEST_F，都会执行一次 SetUp
  void SetUp() override {
    // 1. 初始化 BufferPoolManager
    // 这里的参数（如 50）代表 Buffer Pool 的大小
    // 你需要根据你实际的 BufferPoolManager 构造函数来修改
    if (std::filesystem::exists("data")) {
      std::filesystem::remove("data");
    }
    vector<shared_ptr<DiskManager>> disk_manager;
    auto disk = make_shared<DiskManager>(0, "data");
    disk_manager.push_back(disk);
    bpm_ = make_shared<BufferPoolManager>(1000, disk_manager);
    // 2. 假设 header_page 已经由 BPM 创建或通过其他方式获取
    //header_page_id_ = bpm_->NewPage(0); 
    //std::cerr << "header page id: " << header_page_id_ << std::endl;
    // 3. 实例化 B+ 树
    // 对应构造函数: BPlusTree(file_id, header_page_id, bpm, leaf_max, internal_max)
    tree_ = make_shared<BPlusTree<TEST_KEY_TYPE, TEST_VALUE_TYPE, Compare>>(
        0, 0, bpm_, 5, 5); // max_size 设小一点容易触发分裂
  }

  shared_ptr<BufferPoolManager> bpm_;
  page_id_t header_page_id_;
  shared_ptr<BPlusTree<TEST_KEY_TYPE, TEST_VALUE_TYPE, Compare>> tree_;
};

/*TEST_F(BPlusTreeTest, StressTest) {
  const int n = 30000;
  std::vector<int> keys(n);
  std::iota(keys.begin(), keys.end(), 1); // 生成 1 到 5000

  // 固定种子以确保 Bug 可复现
  std::mt19937 g(1337);
  std::shuffle(keys.begin(), keys.end(), g);

  // 1. 插入
  for (auto key : keys) {
    ComposedKey<40> composed_key;
    snprintf(composed_key.key.key, 40, "Book%d", key);
    //std::cerr << composed_key.GetKey() << std::endl;
    composed_key.rid = key;
    //std::cerr << key << std::endl;
    bool insert_success = tree_->Insert(composed_key, key);
    ASSERT_TRUE(insert_success) << "Insert failed for unique key: " << key;
  }
  //std::cerr << "check1" << std::endl;
  // 2. 验证
  for (auto key : keys) {
    ComposedKey<40> composed_key;
    snprintf(composed_key.key.key, 40, "Book%d", key);
    composed_key.rid = key;
    composed_key.is_min = true;
    vector<int> result;
    tree_->GetValue(composed_key, &result);
    ASSERT_TRUE(result.size() == 1) << "Key " << key << " was lost after insertion!";
    EXPECT_EQ(result[0], key);
  }
  //std::cerr << "check2" << std::endl;
  for (int i = 0; i < n / 2; i++) {
    ComposedKey<40> composed_key;
    snprintf(composed_key.key.key, 40, "Book%d", keys[i]);
    composed_key.rid = keys[i];
    bool delete_success = tree_->Remove(composed_key);
    ASSERT_TRUE(delete_success) << "Remove failed for key: " << keys[i];
  }
  //ASSERT_TRUE(!tree_->Remove(5001)) << "Remove should fail for non-existent key: 5001";
  //std::cerr << "check3" << std::endl;
  for (int i = 0; i < n / 2; i++) {
    ComposedKey<40> composed_key;
    snprintf(composed_key.key.key, 40, "Book%d", keys[i]);
    composed_key.rid = keys[i];
    composed_key.is_min = true;
    vector<int> result;
    tree_->GetValue(composed_key, &result);
    EXPECT_FALSE(result.size() == 1) << "Key " << keys[i] << " should have been removed!";
  }
  //std::cerr << "check4" << std::endl;
  for (int i = n / 2; i < n; i++) {
    ComposedKey<40> composed_key;
    snprintf(composed_key.key.key, 40, "Book%d", keys[i]);
    composed_key.rid = keys[i];
    composed_key.is_min = true;
    vector<int> result;
    tree_->GetValue(composed_key, &result);
    ASSERT_TRUE(result.size() == 1) << "Key " << keys[i] << " was lost after stress deletion!";
    EXPECT_EQ(result[0], keys[i]);
    //std::cerr << "check " << i << std::endl;
    composed_key.is_min = false;
    bool delete_success = tree_->Remove(composed_key);
    ASSERT_TRUE(delete_success) << "Remove failed for key: " << keys[i];
  }
}*/

TEST_F(BPlusTreeTest, ConcurrentStressTest) {
  const int num_threads = 16;
  const int keys_per_thread = 5000;
  const int total_keys = num_threads * keys_per_thread;

  std::vector<int> all_keys(total_keys);
  std::iota(all_keys.begin(), all_keys.end(), 1);
  
  random_device rd;
  std::mt19937 g(rd());
  std::shuffle(all_keys.begin(), all_keys.end(), g);

  {
  vector<shared_ptr<DiskManager>> disk_manager1;
    auto disk = make_shared<DiskManager>(0, "data");
    disk_manager1.push_back(disk);
    auto bpm1_ = make_shared<BufferPoolManager>(1000, disk_manager1);
    // 2. 假设 header_page 已经由 BPM 创建或通过其他方式获取
    //header_page_id_ = bpm_->NewPage(0); 
    //std::cerr << "header page id: " << header_page_id_ << std::endl;
    // 3. 实例化 B+ 树
    // 对应构造函数: BPlusTree(file_id, header_page_id, bpm, leaf_max, internal_max)
    auto tree1_ = make_shared<BPlusTree<TEST_KEY_TYPE, TEST_VALUE_TYPE, Compare>>(
        0, 0, bpm_, 5, 5); // max_size 设小一点容易触发分裂
  

  auto concurrent_insert1 = [&](int thread_id) {
    for (int i = 0; i < keys_per_thread; ++i) {
      int key = all_keys[thread_id * keys_per_thread + i];
      ComposedKey<40> composed_key;
      snprintf(composed_key.key.key, 40, "Book%d", key);
      composed_key.rid = key;
      tree1_->Insert(composed_key, key);
    }
  };

  auto concurrent_remove1 = [&](int thread_id) {
    for (int i = 0; i < keys_per_thread; ++i) {
      int key = all_keys[thread_id * keys_per_thread + i];
      ComposedKey<40> composed_key;
      snprintf(composed_key.key.key, 40, "Book%d", key);
      composed_key.rid = key;
      composed_key.is_min = false;
      tree1_->Remove(composed_key);
    }
  };

  auto concurrent_get1 = [&](int thread_id) {
    for (int i = 0; i < keys_per_thread; ++i) {
      int key = all_keys[thread_id * keys_per_thread + i];
      ComposedKey<40> composed_key;
      snprintf(composed_key.key.key, 40, "Book%d", key);
      composed_key.rid = key;
      composed_key.is_min = true;

      sjtu::vector<int> result;
      tree1_->GetValue(composed_key, &result);
      
      ASSERT_EQ(result.size(), 1) << "Key " << key << " not found!";
      EXPECT_EQ(result[0], key);
    }
  };

    std::vector<std::thread> threads;
  for (int i = 0; i < num_threads / 2; ++i) {
    threads.emplace_back(concurrent_insert1, i);
  }
  for (auto &t : threads) {
    t.join();
  }
  std::cerr << "insert finish" << std::endl;

  std::vector<std::thread> threads2;
  for (int i = 0; i < num_threads / 2; i++) {
    threads2.emplace_back(concurrent_get1, i);
  }
  for (int i = num_threads / 2; i < num_threads; i++) {
    threads2.emplace_back(concurrent_insert1, i);
  }
  for (auto &t : threads2) {
    t.join();
  }
  std::cerr << "insert and get finish" << std::endl;


  std::vector<std::thread> threads3;
  for (int i = 0; i < num_threads / 2; i++) {
    threads3.emplace_back(concurrent_remove1, i);
  }
  for (int i = num_threads / 2; i < num_threads; i++) {
    threads3.emplace_back(concurrent_get1, i);
  }
  for (auto &t : threads3) {
    t.join();
  }
  std::cerr << "get and remove finish" << std::endl;
}


  auto concurrent_insert = [&](int thread_id) {
    for (int i = 0; i < keys_per_thread; ++i) {
      int key = all_keys[thread_id * keys_per_thread + i];
      ComposedKey<40> composed_key;
      snprintf(composed_key.key.key, 40, "Book%d", key);
      composed_key.rid = key;
      tree_->Insert(composed_key, key);
    }
  };

  auto concurrent_remove = [&](int thread_id) {
    for (int i = 0; i < keys_per_thread; ++i) {
      int key = all_keys[thread_id * keys_per_thread + i];
      ComposedKey<40> composed_key;
      snprintf(composed_key.key.key, 40, "Book%d", key);
      composed_key.rid = key;
      composed_key.is_min = false;
      tree_->Remove(composed_key);
    }
  };

  auto concurrent_get = [&](int thread_id) {
    for (int i = 0; i < keys_per_thread; ++i) {
      int key = all_keys[thread_id * keys_per_thread + i];
      ComposedKey<40> composed_key;
      snprintf(composed_key.key.key, 40, "Book%d", key);
      composed_key.rid = key;
      composed_key.is_min = true;

      sjtu::vector<int> result;
      tree_->GetValue(composed_key, &result);
      
      ASSERT_EQ(result.size(), 1) << "Key " << key << " not found!";
      EXPECT_EQ(result[0], key);
    }
  };


  std::vector<std::thread> threads4;
  for (int i = 0; i < num_threads / 2; i++) {
    threads4.emplace_back(concurrent_insert, i);
  }
  for (int i = num_threads / 2; i < num_threads; ++i) {
    threads4.emplace_back(concurrent_remove, i);
  }
  for (auto &t : threads4) {
    t.join();
  }
  std::cerr << "insert and remove finish" << std::endl;

  std::vector<std::thread> threads5;
  for (int i = 0; i < num_threads / 2; i++) {
    threads5.emplace_back(concurrent_get, i);
  }
  for (auto &t : threads5) {
    t.join();
  }
  std::cerr << "get remain finish" << std::endl;

  std::vector<std::thread> threads6;
  for (int i = 0; i < num_threads / 2; i++) {
    threads6.emplace_back(concurrent_remove, i);
  }
  for (auto &t : threads6) {
    t.join();
  }
  std::cerr << "remove remain finish" << std::endl;

}