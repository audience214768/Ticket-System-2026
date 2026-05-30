#pragma once

#include <string>

using std::string;

class Exception {
private:
    string message;
public:
    explicit Exception(const string& msg) : message(msg) {}
    const char* what() const noexcept {
        return message.c_str();
    }
};
