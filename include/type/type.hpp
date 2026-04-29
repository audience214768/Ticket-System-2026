#pragma once

#include <cstring>
#include <string>

using std::string;

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
      int cmp = memcmp(a.key.key, b.key.key, 65);
      if (cmp != 0) {
        return cmp;
      }
      if (a.rid < b.rid || a.is_min) {
        return -1;
      } 
      if (a.rid > b.rid || b.is_min) {
        return 1;
      }
      return 0;
    }
};