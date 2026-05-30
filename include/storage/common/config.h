#pragma once

#include <iostream>

using page_id_t = size_t;
using frame_id_t = size_t;

#define Replacer ClockReplacer

constexpr int DISK_PAGE_SIZE = 4096;
constexpr int DISK_FILE_SIZE = 131072;
constexpr int FILE_BIT = 17;
constexpr page_id_t INVALID_PAGE_ID = -1;
constexpr size_t INF_TIME_STAMP = -1;
constexpr frame_id_t INVALID_FRAME_ID = -1;

