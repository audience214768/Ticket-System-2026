cmake_minimum_required(VERSION 3.28)
project("ticket-manager-system")

set(CMAKE_CXX_STANDARD 20)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g")

add_subdirectory(lib/my-STL)
add_subdirectory(src)
