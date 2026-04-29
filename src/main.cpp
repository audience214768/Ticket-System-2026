#include "common/config.h"
#include "index/b_plus_tree.h"
#include "disk/disk_manager.h"
#include "buffer/buffer_pool_manager.h"
#include "page/b_plus_tree_leaf_page.h"
#include "page/b_plus_tree_page.h"
#include "page/page_guard.h"
#include "type/type.hpp"

using std::string;
using std::cin;
using std::cout;
using std::endl;

int main() {
  vector<shared_ptr<DiskManager>> disk_manager;
  disk_manager.push_back(make_shared<DiskManager>(0, "data"));
  shared_ptr<BufferPoolManager> bpm = make_shared<BufferPoolManager>(3000, disk_manager);
  BPlusTree<ComposedKey<65>, int, Compare> tree(0, 0, bpm);
  int n;
  cin >> n;
  for(int i = 0; i < n; i++) {
    string cmd;
    cin >> cmd;
    if (cmd == "insert") {
        ComposedKey<65> key;
        cin >> key.key.key >> key.rid;
        tree.Insert(key, key.rid);
    } else if (cmd == "find") {
        ComposedKey<65> key;
        key.is_min = true;
        cin >> key.key.key;
        vector<int> result;
        tree.GetValue(key, &result);
        if(!result.empty()) {
          for(int i = 0; i < result.size() - 1; i++) {
            cout << result[i] << " ";
          }
          cout << result[result.size() - 1] << endl;
        } else {
          cout << "null" << endl;
        }
    } else if (cmd == "delete") {
        ComposedKey<65> key;
        cin >> key.key.key >> key.rid;
        tree.Remove(key);
    }
  }
}