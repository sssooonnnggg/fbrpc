cmake_minimum_required(VERSION 3.16)

add_executable(fbrpc_generator ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

target_include_directories(fbrpc_generator PRIVATE ${LIB_ROOT})
target_link_libraries(fbrpc_generator libfbrpc)

get_target_property(GENERATOR_PATH fbrpc_generator CMAKE_BINARY_DIR)
