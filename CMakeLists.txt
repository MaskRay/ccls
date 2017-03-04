cmake_minimum_required (VERSION 3.3.1)

#
# Project configuration
#
project (SuperClangIndex)
set(CMAKE_CXX_STANDARD 11)
file(GLOB SOURCE_FILES *.cc *.cpp libclangmm/*.cc)
find_program(llvm_config llvm-config-3.8)


#
# Find clang library and include directories
#
execute_process(
  COMMAND ${llvm_config} --includedir
  OUTPUT_VARIABLE LIBCLANG_INCLUDEDIR)
string(STRIP "${LIBCLANG_INCLUDEDIR}" LIBCLANG_INCLUDEDIR)

execute_process(
  COMMAND ${llvm_config} --libdir
  OUTPUT_VARIABLE LIBCLANG_LIBDIR)
string(STRIP "${LIBCLANG_LIBDIR}" LIBCLANG_LIBDIR)

find_library(LIBCLANG_LIBRARIES NAMES libclang clang
                                PATHS ${LIBCLANG_LIBDIR})




set(LIBCLANG_INCLUDEDIR "/usr/local/google/home/jdufault/super-clang-index/clang+llvm-3.9.1-x86_64-linux-gnu-ubuntu-16.04/include")
set(LIBCLANG_LIBDIR     "/usr/local/google/home/jdufault/super-clang-index/clang+llvm-3.9.1-x86_64-linux-gnu-ubuntu-16.04/lib")
set(LIBCLANG_LIBRARIES  "/usr/local/google/home/jdufault/super-clang-index/clang+llvm-3.9.1-x86_64-linux-gnu-ubuntu-16.04/lib/libclang.so")


#
# Debugging
#
#message("SOURCE_FILES:        " ${SOURCE_FILES} )
message("LIBCLANG_INCLUDEDIR: " ${LIBCLANG_INCLUDEDIR})
message("LIBCLANG_LIBDIR:     " ${LIBCLANG_LIBDIR})
message("LIBCLANG_LIBRARIES:  " ${LIBCLANG_LIBRARIES})
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")


#
# Setup output binary.
#
include_directories("${PROJECT_SOURCE_DIR}/third_party/rapidjson/include")
include_directories("${LIBCLANG_INCLUDEDIR}")

add_executable (indexer ${SOURCE_FILES})
target_link_libraries(indexer ${LIBCLANG_LIBRARIES})
