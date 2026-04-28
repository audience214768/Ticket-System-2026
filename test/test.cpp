#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <random>
#include <vector>
#include "index/b_plus_tree.h" // 确保路径正确
#include "buffer/buffer_pool_manager.h"
#include "disk/disk_manager.h"

// 这里的宏定义需要根据你具体的模板参数调整
// 假设 KeyType 是 int, ValueType 是 int, NumTombs 是某个数值
#define TEST_KEY_TYPE int
#define TEST_VALUE_TYPE int
#define TEST_NUM_TOMBS 0

class BPlusTreeTest : public ::testing::Test {
 protected:
  // 每跑一个 TEST_F，都会执行一次 SetUp
  void SetUp() override {
    // 1. 初始化 BufferPoolManager
    // 这里的参数（如 50）代表 Buffer Pool 的大小
    // 你需要根据你实际的 BufferPoolManager 构造函数来修改
    std::vector<std::shared_ptr<DiskManager>> disk_manager;
    auto disk = std::make_shared<DiskManager>(0, "data");
    disk_manager.push_back(disk);
    bpm_ = std::make_shared<BufferPoolManager>(50, disk_manager);
    // 2. 假设 header_page 已经由 BPM 创建或通过其他方式获取
    header_page_id_ = bpm_->NewPage(0); 
    std::cerr << "header page id: " << header_page_id_ << std::endl;
    // 3. 实例化 B+ 树
    // 对应构造函数: BPlusTree(file_id, header_page_id, bpm, leaf_max, internal_max)
    tree_ = std::make_unique<BPlusTree<TEST_KEY_TYPE, TEST_VALUE_TYPE, int>>(
        0, header_page_id_, bpm_, 50, 50); // max_size 设小一点容易触发分裂
  }

  std::shared_ptr<BufferPoolManager> bpm_;
  page_id_t header_page_id_;
  std::unique_ptr<BPlusTree<TEST_KEY_TYPE, TEST_VALUE_TYPE, int>> tree_;
};

// --- 测试用例 ---
/*
// 1. 测试空树
TEST_F(BPlusTreeTest, IsEmptyTest) {
  std::cerr << "begin to test empty" << std::endl;
  EXPECT_TRUE(tree_->IsEmpty());
  //tree_->~BPlusTree<TEST_KEY_TYPE, TEST_VALUE_TYPE, TEST_NUM_TOMBS>();
  std::cerr << "finish pass" << std::endl;
}

// 2. 测试简单插入与获取
TEST_F(BPlusTreeTest, InsertGetTest) {
  std::cerr << "begin to test insert and get" << std::endl;
  tree_->Insert(1, 100);
  //std::cerr << "check" << std::endl;
  tree_->Insert(2, 200);
  //std::cerr << "check" << std::endl;
  tree_->Insert(3, 300);
  //std::cerr << "finish insert" << std::endl;
  std::vector<int> result;
  bool found = tree_->GetValue(2, &result);
  
  EXPECT_TRUE(found);
  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0], 200);
  EXPECT_FALSE(tree_->IsEmpty());
}

// 3. 测试分裂 (插入超过 leaf_max_size_)
TEST_F(BPlusTreeTest, SplitTest) {
  // 我们在 SetUp 里设了 leaf_max 为 3
  std::cerr << "begin to test split" << std::endl;
  for (int i = 1; i <= 10; ++i) {
    tree_->Insert(i, i * 10);
  }

  for (int i = 1; i <= 10; ++i) {
    std::vector<int> result;
    EXPECT_TRUE(tree_->GetValue(i, &result)) << "Key " << i << " should be found";
    EXPECT_EQ(result[0], i * 10);
  }
}

// 4. 测试删除
TEST_F(BPlusTreeTest, RemoveTest) {
  std::cerr << "begin to test remove" << std::endl;
  for (int i = 1; i <= 5; ++i) tree_->Insert(i, i);
  
  tree_->Remove(3);
  
  std::vector<int> result;
  EXPECT_FALSE(tree_->GetValue(3, &result)); // 3 应该找不到了
  EXPECT_TRUE(tree_->GetValue(1, &result));  // 1 应该还在
}*/

TEST_F(BPlusTreeTest, StressTestUniqueKeys) {
  const int n = 5000;
  std::vector<int> keys(n);
  std::iota(keys.begin(), keys.end(), 1); // 生成 1 到 5000

  // 固定种子以确保 Bug 可复现
  std::mt19937 g(1337);
  std::shuffle(keys.begin(), keys.end(), g);

  // 1. 插入
  for (auto key : keys) {
    //std::cerr << key << std::endl;
    bool insert_success = tree_->Insert(key, key);
    ASSERT_TRUE(insert_success) << "Insert failed for unique key: " << key;
  }
  //std::cerr << "check1" << std::endl;
  // 2. 验证
  for (auto key : keys) {
    std::vector<int> result;
    tree_->GetValue(key, &result);
    ASSERT_TRUE(result.size() == 1) << "Key " << key << " was lost after stress insertion!";
    EXPECT_EQ(result[0], key);
  }
  //std::cerr << "check2" << std::endl;
  for (int i = 0; i < n / 2; i++) {
    bool delete_success = tree_->Remove(keys[i]);
    ASSERT_TRUE(delete_success) << "Remove failed for key: " << keys[i];
  }
  ASSERT_TRUE(!tree_->Remove(5001)) << "Remove should fail for non-existent key: 5001";
  //std::cerr << "check3" << std::endl;
  for (int i = 0; i < n / 2; i++) {
    std::vector<int> result;
    tree_->GetValue(keys[i], &result);
    EXPECT_FALSE(result.size() == 1) << "Key " << keys[i] << " should have been removed!";
  }
  //std::cerr << "check4" << std::endl;
  for (int i = n / 2; i < n; i++) {
    std::vector<int> result;
    //std::cerr << "check " << i << " " << keys[i] << std::endl;
    tree_->GetValue(keys[i], &result);
    ASSERT_TRUE(result.size() == 1) << "Key " << keys[i] << " was lost after stress deletion!";
    EXPECT_EQ(result[0], keys[i]);
    //std::cerr << "check " << i << std::endl;
    bool delete_success = tree_->Remove(keys[i]);
    ASSERT_TRUE(delete_success) << "Remove failed for key: " << keys[i];
  }
}