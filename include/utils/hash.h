#pragma once
inline auto hash_djb2(const char *str) -> unsigned long {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}