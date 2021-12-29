cmake_minimum_required(VERSION 3.16)

add_executable(fbrpc_generator ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

target_link_libraries(fbrpc_generator libfbrpc)

get_target_property(GENERATOR_PATH fbrpc_generator CMAKE_BINARY_DIR)

target_include_directories(fbrpc_generator PRIVATE ${CMAKE_SOURCE_DIR})

set(TARGET_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/bin)

set_target_properties(fbrpc_generator PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${TARGET_OUTPUT_DIR}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${TARGET_OUTPUT_DIR}
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${TARGET_OUTPUT_DIR}
)
