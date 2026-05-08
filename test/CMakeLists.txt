include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
FetchContent_MakeAvailable(googletest)

add_executable(bplus_tree_test test.cpp)

target_link_libraries(
    bplus_tree_test 
    _page
    _disk 
    _buffer 
    GTest::gtest_main
)

add_test(NAME MyBPlusTreeTest COMMAND bplus_tree_test)