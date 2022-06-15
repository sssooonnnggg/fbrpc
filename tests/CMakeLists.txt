cmake_minimum_required(VERSION 3.16)

# gen flatbuffers proto
if(USE_VCPKG)
    find_path(FLAT_BUFFER_PATH flatbuffers/flatbuffers.h NO_CACHE)
    set(FLATC_PATH "${FLAT_BUFFER_PATH}/../../${VCPKG_TARGET_TRIPLET}/tools/flatbuffers/flatc")
    message(${FLATC_PATH})
else()
    set(FLATC_PATH ${FLATBUFFERS_FLATC_EXECUTABLE})
endif()

set(PROTO_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/proto)
set(PROTO_GEN_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/generated)
set(SERVICE_GEN_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/generated)

file(REMOVE_RECURSE ${PROTO_GEN_ROOT})
file(GLOB_RECURSE PROTO_FILES ${PROTO_ROOT}/*.fbs)

foreach(PROTO_FILE ${PROTO_FILES})
    get_filename_component(PROTO_NAME ${PROTO_FILE} NAME)
    string(REGEX REPLACE "[.]fbs$" "_generated.h" PROTO_GEN_FILE_NAME ${PROTO_NAME})
    set(PROTO_GEN_FILE ${PROTO_GEN_ROOT}/${PROTO_GEN_FILE_NAME})
    list(APPEND PROTO_GEN_FILES ${PROTO_GEN_FILE})
    add_custom_command(
        OUTPUT ${PROTO_GEN_FILE}
        COMMAND ${FLATC_PATH} -c -o ${PROTO_GEN_ROOT} -I ${PROTO_ROOT} ${PROTO_FILE}
        DEPENDS ${PROTO_FILE}
        COMMENT "[flatc] generating ${PROTO_FILE} to ${PROTO_GEN_FILE} ..."
        VERBATIM
    )
    string(REGEX REPLACE "[.]fbs$" "Service_generated.h" SERVICE_GEN_FILE_NAME ${PROTO_NAME})
    string(REGEX REPLACE "[.]fbs$" "Stub_generated.h" STUB_GEN_FILE_NAME ${PROTO_NAME})
    set(SERVICE_GEN_FILE ${PROTO_GEN_ROOT}/${SERVICE_GEN_FILE_NAME})
    set(STUB_GEN_FILE ${PROTO_GEN_ROOT}/${STUB_GEN_FILE_NAME})
    add_custom_command(
        OUTPUT ${SERVICE_GEN_FILE} ${STUB_GEN_FILE}
        COMMAND $<TARGET_FILE:fbrpc_generator> -c ${PROTO_FILE} ${SERVICE_GEN_ROOT}
        DEPENDS ${PROTO_FILE}
        COMMENT "[fbrpc_generator] generating ${PROTO_FILE} services ..."
        VERBATIM
    )
    list(APPEND SERVICE_GEN_FILES ${SERVICE_GEN_FILE} ${STUB_GEN_FILE})
endforeach()

file(GLOB_RECURSE SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.fbs)
createSrcGroups(${CMAKE_CURRENT_SOURCE_DIR} ${SOURCE_FILES})
add_executable(fbrpc_test ${SOURCE_FILES} ${PROTO_GEN_FILES} ${SERVICE_GEN_FILES})

add_dependencies(fbrpc_test fbrpc_generator)

target_include_directories(fbrpc_test PRIVATE ${FBRPC_ROOT})
target_link_libraries(fbrpc_test libfbrpc)

if (UNIX AND USE_VCPKG)
    target_link_libraries(fbrpc_test ${CMAKE_DL_LIBS})
endif ()

set(TARGET_OUTPUT_DIR ${FBRPC_ROOT}/bin)

set_target_properties(fbrpc_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${TARGET_OUTPUT_DIR}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${TARGET_OUTPUT_DIR}
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${TARGET_OUTPUT_DIR}
)