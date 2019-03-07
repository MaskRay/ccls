cmake_minimum_required(VERSION 3.11)
project(ccls LANGUAGES CXX)

### Colored output
if(NOT WIN32)
  string(ASCII 27 Esc)
  set(Green       "${Esc}[32m")
  set(BoldBlue    "${Esc}[1;34m")
endif()


list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/)
include(DefaultCMakeBuildType)

# Required Clang version
option(LLVM_ENABLE_RTTI "-fno-rtti if OFF. This should match LLVM libraries" OFF)
option(USE_SHARED_LLVM "Link against libLLVM.so instead separate LLVM{Option,Support,...}" OFF)

# Sources for the executable are specified at end of CMakeLists.txt
add_executable(ccls "")

### Compile options

# CMake default compile flags:
# MSVC + Clang(Windows):
#   debug: /MDd /Zi /Ob0 /Od /RTC1
#   release: /MD /O2 /Ob2 /DNDEBUG
# GCC + Clang(Linux):
#   debug: -g
#   release: -O3 -DNDEBUG

# Enable C++17 (Required)
set_property(TARGET ccls PROPERTY CXX_STANDARD 17)
set_property(TARGET ccls PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET ccls PROPERTY CXX_EXTENSIONS OFF)

if(NOT LLVM_ENABLE_RTTI)
  # releases.llvm.org libraries are compiled with -fno-rtti
  # The mismatch between lib{clang,LLVM}* and ccls can make libstdc++ std::make_shared return nullptr
  # _Sp_counted_ptr_inplace::_M_get_deleter
  target_compile_options(ccls PRIVATE -fno-rtti)
endif()

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

# See cmake/FindClang.cmake
find_package(Clang 7.0.0)
target_link_libraries(ccls PRIVATE Clang::Clang)

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

target_compile_definitions(ccls PRIVATE
                           DEFAULT_RESOURCE_DIRECTORY=R"\(${Clang_RESOURCE_DIR}\)")


### use local installed RapidJSON or FETCH from git via cmake
find_package(RapidJSON QUIET)
if(RapidJSON_FOUND)
  message("${BoldBlue}\nRapidJSON has been installed by your system before cloning ccls\n\n${ColourReset}")
  include_directories(${RapidJSON_INCLUDE_DIRS})
else()
  message("${Green}====> Downloading RapidJSON ---- A fast JSON parser/generator for C++ with both SAX/DOM style API http://rapidjson.org/
${ColourReset}")
  include(FetchContent)
  FetchContent_Declare(
    rapidjson
    GIT_REPOSITORY https://github.com/Tencent/rapidjson
    GIT_TAG master
    )

  FetchContent_GetProperties(rapidjson)
  if (NOT rapidjson_POPULATED)
    FetchContent_Populate(rapidjson)

    message(${rapidjson_SOURCE_DIR})
    message(${rapidjson_BINARY_DIR})
    add_subdirectory(${rapidjson_SOURCE_DIR})
    message("${BoldBlue}\ngit clone RapidJSON successfully!\n\n${ColourReset}")
    set(INCLUDE_DIRS ${rapidjson_SOURCE_DIR}/include)

    message("${BoldBlue}\n${rapidjson_SOURCE_DIR}/include ${ColourReset}")
    message("${BoldBlue}\n${INCLUDE_DIRS} ${ColourReset}")
    target_include_directories(ccls PUBLIC
      ${INCLUDE_DIRS}
      )

  endif()

endif()


### Includes

target_include_directories(ccls PRIVATE src)
target_include_directories(ccls SYSTEM PRIVATE
                           third_party)

### Install

install(TARGETS ccls RUNTIME DESTINATION bin)

### Tools

# We use glob here since source files are already manually added with
# target_sources further down
file(GLOB SOURCES src/*.cc src/*.h src/serializers/*.cc src/serializers/*.h
                  src/messages/*.h src/messages/*.cc)

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
