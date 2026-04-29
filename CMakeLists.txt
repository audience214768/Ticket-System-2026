cmake_minimum_required(VERSION 3.28)
project("ticket-manager-system")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Debug)


include_directories(${PROJECT_SOURCE_DIR}/lib)
include_directories(${PROJECT_SOURCE_DIR}/include)

add_subdirectory(${PROJECT_SOURCE_DIR}/src)
#add_subdirectory(${PROJECT_SOURCE_DIR}/test)

add_executable(code src/main.cpp)

target_link_libraries(
  code
  PRIVATE 
  _buffer
  _disk
  _page
)
enable_testing()
