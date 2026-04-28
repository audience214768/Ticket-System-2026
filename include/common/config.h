#pragma once

#include <iostream>

using page_id_t = size_t;
using frame_id_t = size_t;

constexpr int DISK_PAGE_SIZE = 4096;
constexpr page_id_t INVALID_PAGE_ID = -1;
constexpr size_t INF_TIME_STAMP = -1;
constexpr frame_id_t INVALID_FRAME_ID = -1; 

