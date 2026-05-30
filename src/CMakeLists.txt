set(SOURCES
    main.cpp
    core/ticket_system.cpp
    manager/user_manager.cpp
    manager/train_manager.cpp
    manager/order_manager.cpp
    manager/seat_manager.cpp
)

file(GLOB_RECURSE STORAGE_SOURCES "storage/*.cpp")

set(SOURCES ${SOURCES} ${STORAGE_SOURCES})

add_executable(code ${SOURCES})

set_target_properties(code PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_link_libraries(code PRIVATE my_stl)

target_include_directories(code PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

target_include_directories(code PRIVATE
    ${PROJECT_SOURCE_DIR}/include/storage
)
