#include "index/b_plus_tree.h"
#include "disk/disk_manager.h"
#include "buffer/buffer_pool_manager.h"
#include "page/page_guard.h"

using std::string;
using std::cin;
using std::cout;
using std::endl;

template <size_t len>
struct FixedString {
  char key[len]{};
  auto ToString() const -> string {
    return string(key);
  }
};

template<size_t len>
struct ComposedKey {
  FixedString<len> key;
  int rid;
  bool is_min = false;
  auto GetKey() const -> string {
    return key.ToString();
  }
};

class Compare {
  public:
    auto operator()(const ComposedKey<65> &a, const ComposedKey<65> &b) const -> int {
      if (a.GetKey() != b.GetKey()) {
        if (a.GetKey() < b.GetKey()) {
          return -1;
        } else {
          return 1;
        }
      } else {
        if (a.rid < b.rid || a.is_min) {
          return -1;
        } else if (a.rid > b.rid || b.is_min) {
          return 1;
        } else {
          return 0;
        }
      }
    }
};

int main() {
  vector<shared_ptr<DiskManager>> disk_manager;
  disk_manager.push_back(make_shared<DiskManager>(0, "data"));
  shared_ptr<BufferPoolManager> bpm = make_shared<BufferPoolManager>(1000, disk_manager);
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