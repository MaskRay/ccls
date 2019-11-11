cmake_minimum_required(VERSION 3.8)
project(ccls LANGUAGES CXX)

option(USE_SYSTEM_RAPIDJSON "Use system RapidJSON instead of the git submodule if exists" ON)

# Sources for the executable are specified at end of CMakeLists.txt
add_executable(ccls "")

### Default build type

set(DEFAULT_CMAKE_BUILD_TYPE Release)

# CMAKE_BUILD_TYPE is not available if a multi-configuration generator is used
# (eg Visual Studio generators)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${DEFAULT_CMAKE_BUILD_TYPE}' as none \
was specified.")
  set(CMAKE_BUILD_TYPE ${DEFAULT_CMAKE_BUILD_TYPE}
      CACHE STRING "Choose the type of build." FORCE)

  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE
               PROPERTY STRINGS Debug Release MinSizeRel RelWithDebInfo)
endif()

### Compile options

# Enable C++17 (Required)
set_property(TARGET ccls PROPERTY CXX_STANDARD 17)
set_property(TARGET ccls PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET ccls PROPERTY CXX_EXTENSIONS OFF)

# CMake sets MSVC for both MSVC and Clang(Windows)
if(MSVC)
  # Common MSVC/Clang(Windows) options
  target_compile_options(ccls PRIVATE
    /nologo
    /EHsc
    /D_CRT_SECURE_NO_WARNINGS # don't try to use MSVC std replacements
    /W3 # roughly -Wall
    /wd4996 # ignore deprecated declaration
    /wd4267 # ignores warning C4267
            # (conversion from 'size_t' to 'type'),
            # roughly -Wno-sign-compare
    /wd4800
    /wd4068 # Disable unknown pragma warning
    /std:c++17
    $<$<CONFIG:Debug>:/FS>
  )
  # relink system libs
  target_link_libraries(ccls PRIVATE Mincore.lib)
else()
  # Common GCC/Clang(Linux) options
  target_compile_options(ccls PRIVATE
                         -Wall
                         -Wno-sign-compare
                         )

  if(${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
    target_compile_options(ccls PRIVATE -Wno-return-type -Wno-unused-result)
  endif()

  if(${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
    target_compile_options(ccls PRIVATE
                           $<$<CONFIG:Debug>:-fno-limit-debug-info>)
  endif()
endif()

### Libraries

find_package(Clang REQUIRED)

if(CLANG_LINK_CLANG_DYLIB)
  target_link_libraries(ccls PRIVATE clang-cpp)
else()
  target_link_libraries(ccls PRIVATE
    clangIndex
    clangFormat
    clangTooling
    clangToolingInclusions
    clangToolingCore
    clangFrontend
    clangParse
    clangSerialization
    clangSema
    clangAST
    clangLex
    clangDriver
    clangBasic
  )
endif()

if(LLVM_LINK_LLVM_DYLIB)
  target_link_libraries(ccls PRIVATE LLVM)
else()
  # In llvm 7, clangDriver headers reference LLVMOption
  target_link_libraries(ccls PRIVATE LLVMOption LLVMSupport)
endif()

if(NOT LLVM_ENABLE_RTTI)
  # releases.llvm.org libraries are compiled with -fno-rtti
  # The mismatch between lib{clang,LLVM}* and ccls can make libstdc++ std::make_shared return nullptr
  # _Sp_counted_ptr_inplace::_M_get_deleter
  if(MSVC)
    target_compile_options(ccls PRIVATE /GR-)
  else()
    target_compile_options(ccls PRIVATE -fno-rtti)
  endif()
endif()

# Enable threading support
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(ccls PRIVATE Threads::Threads)

if(${CMAKE_SYSTEM_NAME} STREQUAL FreeBSD)
  find_package(Backtrace REQUIRED)
  target_link_libraries(ccls PRIVATE ${Backtrace_LIBRARIES})
  # src/platform_posix.cc uses libthr
  target_link_libraries(ccls PRIVATE thr)
endif()

### Definitions

# Find Clang resource directory with Clang executable.

if(NOT CLANG_RESOURCE_DIR)
  find_program(CLANG_EXECUTABLE clang)
  if(NOT CLANG_EXECUTABLE)
    message(FATAL_ERROR "clang executable not found.")
  endif()

  execute_process(
    COMMAND ${CLANG_EXECUTABLE} -print-resource-dir
    RESULT_VARIABLE CLANG_FIND_RESOURCE_DIR_RESULT
    OUTPUT_VARIABLE CLANG_RESOURCE_DIR
    ERROR_VARIABLE CLANG_FIND_RESOURCE_DIR_ERROR
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  if(CLANG_FIND_RESOURCE_DIR_RESULT)
    message(FATAL_ERROR "Error retrieving Clang resource directory with Clang \
            executable. Output:\n ${CLANG_FIND_RESOURCE_DIR_ERROR}")
  endif()
endif()

set_property(SOURCE src/utils.cc APPEND PROPERTY COMPILE_DEFINITIONS
             CLANG_RESOURCE_DIRECTORY=R"\(${CLANG_RESOURCE_DIR}\)")

### Includes

if(USE_SYSTEM_RAPIDJSON)
  find_package(RapidJSON QUIET)
endif()
if(NOT RapidJSON_FOUND)
  set(RapidJSON_INCLUDE_DIRS third_party/rapidjson/include)
endif()

target_include_directories(ccls PRIVATE src)

foreach(include_dir third_party
    ${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS} ${RapidJSON_INCLUDE_DIRS})
  get_filename_component(include_dir_realpath ${include_dir} REALPATH)
  # Don't add as SYSTEM if they are in CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES.
  # It would reorder the system search paths and cause issues with libstdc++'s
  # use of #include_next. See https://github.com/MaskRay/ccls/pull/417
  if(NOT "${include_dir_realpath}" IN_LIST CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES)
    target_include_directories(ccls SYSTEM PRIVATE ${include_dir})
  endif()
endforeach()

### Install

install(TARGETS ccls RUNTIME DESTINATION bin)

### Sources

target_sources(ccls PRIVATE third_party/siphash.cc)

target_sources(ccls PRIVATE
  src/clang_tu.cc
  src/config.cc
  src/filesystem.cc
  src/fuzzy_match.cc
  src/main.cc
  src/include_complete.cc
  src/indexer.cc
  src/log.cc
  src/lsp.cc
  src/message_handler.cc
  src/pipeline.cc
  src/platform_posix.cc
  src/platform_win.cc
  src/position.cc
  src/project.cc
  src/query.cc
  src/sema_manager.cc
  src/serializer.cc
  src/test.cc
  src/utils.cc
  src/working_files.cc
)

target_sources(ccls PRIVATE
  src/messages/ccls_call.cc
  src/messages/ccls_info.cc
  src/messages/ccls_inheritance.cc
  src/messages/ccls_member.cc
  src/messages/ccls_navigate.cc
  src/messages/ccls_reload.cc
  src/messages/ccls_vars.cc
  src/messages/initialize.cc
  src/messages/textDocument_code.cc
  src/messages/textDocument_completion.cc
  src/messages/textDocument_definition.cc
  src/messages/textDocument_did.cc
  src/messages/textDocument_foldingRange.cc
  src/messages/textDocument_formatting.cc
  src/messages/textDocument_document.cc
  src/messages/textDocument_hover.cc
  src/messages/textDocument_references.cc
  src/messages/textDocument_rename.cc
  src/messages/textDocument_signatureHelp.cc
  src/messages/workspace.cc
)

### Obtain CCLS version information from Git
### This only happens when cmake is re-run!

if(NOT CCLS_VERSION)
  execute_process(COMMAND git describe --tag --long HEAD
    OUTPUT_VARIABLE CCLS_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

  if(NOT CCLS_VERSION)
    set(CCLS_VERSION "<unknown>")
  endif()
endif()

set_property(SOURCE src/main.cc APPEND PROPERTY
             COMPILE_DEFINITIONS CCLS_VERSION=\"${CCLS_VERSION}\")
set_property(SOURCE src/messages/initialize.cc APPEND PROPERTY
             COMPILE_DEFINITIONS CCLS_VERSION=\"${CCLS_VERSION}\")
