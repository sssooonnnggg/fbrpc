cmake_minimum_required(VERSION 3.16)

find_package(Threads)
find_package(uvw CONFIG REQUIRED)
find_package(unofficial-libuv CONFIG REQUIRED)
find_package(flatbuffers CONFIG REQUIRED)

file(GLOB_RECURSE LIB_SOURCE_FILES ${LIB_ROOT}/*.h ${LIB_ROOT}/*.cpp)
createSrcGroups(${LIB_ROOT} LIB_SOURCE_FILES)

add_library(libfbrpc STATIC ${LIB_SOURCE_FILES})
target_include_directories(libfbrpc PRIVATE ${LIB_ROOT})

find_path(UVW_INCLUDE_DIR uvw.hpp)
target_include_directories(libfbrpc PUBLIC ${UVW_INCLUDE_DIR})
target_compile_features(libfbrpc PRIVATE cxx_std_17)
target_link_libraries(libfbrpc PRIVATE unofficial::libuv::libuv flatbuffers::flatbuffers)