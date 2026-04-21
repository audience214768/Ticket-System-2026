cmake_minimum_required(VERSION 3.28)
project("ticket-manager-system")
enable_testing()

include_directories(${PROJECT_SOURCE_DIR}/include)

add_subdirectory(src)

add_executable(bpt src/main.cpp)

target_link_libraries(
  bpt
  PRIVATE 
  lib_buffer
  lib_disk
  lib_index
  lib_page
)
enable_testing()
