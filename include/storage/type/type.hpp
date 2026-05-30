#pragma once

#include <cstring>
#include <string>

using std::string;

template <size_t len>
struct FixedString {
  char key[len] = {0};
  auto ToString() const -> string {
    return string(key);
  }
};

template<size_t len>
struct ComposedKey {
  static const size_t LEN = len;
  FixedString<len> fixed_key;
  int rid;
  bool is_min = false;
  auto GetKey() const -> string {
    return fixed_key.ToString();
  }
};

class Compare {
  public:
    template<size_t len>
    auto operator()(const ComposedKey<len> &a, const ComposedKey<len> &b) const -> int {
      int cmp = memcmp(a.fixed_key.key, b.fixed_key.key, len);
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