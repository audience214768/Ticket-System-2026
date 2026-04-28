# pragma once

#include <iostream>
#include <exception>
#include <string>

class Exception {
private:
    std::string message;
public:
    explicit Exception(const std::string& msg) : message(msg) {}
    const char* what() const noexcept {
        return message.c_str();
    }
};

template <typename T>
struct is_basic_string : std::false_type {};

template <typename CharT, typename Traits, typename Allocator>
struct is_basic_string<std::basic_string<CharT, Traits, Allocator>> : std::true_type {};

template<class T>
class Expect {
private:
  const T &value;
  bool negitive = 0;
  void check(bool condition) {
    if (!(condition ^ negitive)) {
      throw Exception("false");
    }
  }
public:
  Expect &And;
  Expect &Or;
  Expect &but;
  Expect(const T &t):value(t), And(*this), Or(*this), but(*this) {}
  Expect &ge(const T &t) {
    check(value >= t);
    return *this;
  }
  Expect &le(const T &t) {
    check(value <= t);
    return *this;
  }
  Expect &Not() {
    negitive = !negitive;
    return *this;
  }
  Expect &toBe(const T &t) {
    check(value == t);
    return *this;
  }
  template<class U>
  Expect &toBe() {
    static_assert(std::is_base_of<T, U>::value, "U should be derived from T");
    if constexpr(std::is_polymorphic_v<T>) {
       const U *ptr = dynamic_cast<const U *>(&value);
       check(ptr != nullptr);
    } else if constexpr(std::is_same_v<T, U>) {
      check(true);
    } else {
      check(false);
    }
    return *this;
  }
  template<typename... Args>
  Expect &toBeOneOf(Args... args) {
    check(((value == args) || ...));
    return *this;
  }
  template<typename setType, typename checkType = T>
  auto consistedOf(const setType &set) -> std::enable_if_t<is_basic_string<checkType>::value, Expect&> {
    check(value.find_first_not_of(set) == T::npos);
    return *this;
  }
  template<typename checkType = T>
  auto toBeLength(int min, int max) -> std::enable_if_t<is_basic_string<checkType>::value, Expect&> {
    check(value.length() >= min && value.length() <= max);
    return *this;
  }
};

template<class T>
Expect<T> expect(const T &t) {
    return Expect<T>(t);
}
