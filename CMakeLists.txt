cmake_minimum_required (VERSION 3.3.1)

project (SuperClangIndex)

set (CMAKE_CXX_STANDARD 11)
#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

file (GLOB SOURCE_FILES *.cc *.cpp)

include_directories("${PROJECT_SOURCE_DIR}/third_party")

add_executable (indexer ${SOURCE_FILES})

find_program (llvm_config llvm-config)

set (llvm_param --libdir)
message (${llvm_config})
execute_process (
  COMMAND ${llvm_config} ${llvm_param}
  OUTPUT_VARIABLE libclang_libdir)

message ("Using libclang at " ${libclang_libdir})

#find_package (clang COMPONENTS REQUIRED libclang)
#ind_package (libclang REQUIRED)
#target_link_libraries (indexer ${libclang})

