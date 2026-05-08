cmake_minimum_required(VERSION 3.28)
project("ticket-manager-system")

set(CMAKE_CXX_STANDARD 20)

enable_testing()
#set(CMAKE_BUILD_TYPE Debug)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread -g -O1")
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=thread")

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
