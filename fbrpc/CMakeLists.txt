cmake_minimum_required(VERSION 3.16)

if(USE_VCPKG)
    find_package(Threads)
    find_package(uvw CONFIG REQUIRED)
    find_package(unofficial-libuv CONFIG REQUIRED)
    find_package(flatbuffers CONFIG REQUIRED)
endif()

file(GLOB_RECURSE LIB_SOURCE_FILES ${LIB_ROOT}/*.h ${LIB_ROOT}/*.cpp)
createSrcGroups(${LIB_ROOT} LIB_SOURCE_FILES)

add_library(libfbrpc STATIC ${LIB_SOURCE_FILES})
target_compile_features(libfbrpc PRIVATE cxx_std_17)

if(USE_VCPKG)
    find_path(UVW_INCLUDE_DIR uvw.hpp)
    target_include_directories(libfbrpc PUBLIC ${UVW_INCLUDE_DIR})
    target_link_libraries(libfbrpc PRIVATE unofficial::libuv::libuv flatbuffers::flatbuffers)
else()
    target_include_directories(libfbrpc PUBLIC ${UVW_INCLUDE_DIR} ${UV_INCLUDE_DIR} ${FLATBUFFER_DIR}/include)
    if(PLATFORM_WIN64)
        target_link_libraries(libfbrpc 
            debug ${FLATBUFFER_DIR}/lib/windows/debug/flatbuffers.lib 
            optimized ${FLATBUFFER_DIR}/lib/release/debug/flatbuffers.lib
        )
    endif()
endif()

target_include_directories(libfbrpc PRIVATE ${FBRPC_ROOT})

set(TARGET_OUTPUT_DIR ${FBRPC_ROOT}/lib)

set_target_properties(libfbrpc PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${TARGET_OUTPUT_DIR}
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${TARGET_OUTPUT_DIR}
    ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${TARGET_OUTPUT_DIR}
)