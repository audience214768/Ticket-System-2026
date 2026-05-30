#pragma once

#include "storage/common/config.h"

inline auto hash_djb2(const char *str) -> unsigned long {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

// page_id = (fileID << FILE_BIT) | page_index
// XOR folds fileID bits into lower bits to avoid collisions between
// pages at the same offset in different files
inline auto hash_page(page_id_t page_id, size_t mask) -> int {
    return (page_id ^ (page_id >> FILE_BIT)) & mask;
}