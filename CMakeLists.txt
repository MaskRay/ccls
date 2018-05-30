cmake_minimum_required(VERSION 3.1)
project(ccls LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/)
include(DefaultCMakeBuildType)

# Required Clang version
set(CLANG_VERSION 6.0.0 CACHE STRING "Clang version")
set(CLANG_DOWNLOAD_LOCATION ${CMAKE_BINARY_DIR}
    CACHE STRING "Downloaded Clang location")
option(SYSTEM_CLANG "Use system installation of Clang instead of \
       downloading Clang" OFF)
option(ASAN "Compile with address sanitizers" OFF)
option(CLANG_USE_BUNDLED_LIBC++ "Let Clang use bundled libc++" OFF)
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
# Disable gnu extensions except for Cygwin which needs them to build properly
if(NOT CYGWIN)
  set_property(TARGET ccls PROPERTY CXX_EXTENSIONS OFF)
endif()

# To link against LLVM libraries (usually compiled with -fno-rtti)
target_compile_options(ccls PRIVATE -fno-rtti)

# CMake sets MSVC for both MSVC and Clang(Windows)
if(MSVC)
  # Common MSVC/Clang(Windows) options
  target_compile_options(ccls PRIVATE
                         /nologo
                         /EHsc
                         /W3 # roughly -Wall
                         /wd4996 # disable loguru unsafe warnings
                         /wd4722 # ignores warning C4722
                                 # (destructor never returns) in loguru
                         /wd4267 # ignores warning C4267
                                 # (conversion from 'size_t' to 'type'),
                                 # roughly -Wno-sign-compare
                         /wd4800
                         $<$<CONFIG:Debug>:/FS>
                         )
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

  if(ASAN)
    target_compile_options(ccls PRIVATE -fsanitize=address,undefined)
    # target_link_libraries also takes linker flags
    target_link_libraries(ccls PRIVATE -fsanitize=address,undefined)
  endif()
endif()

### Download Clang if required

if(NOT SYSTEM_CLANG)
  message(STATUS "Using downloaded Clang")

  include(DownloadAndExtractClang)
  download_and_extract_clang(${CLANG_VERSION} ${CLANG_DOWNLOAD_LOCATION})
  # Used by FindClang
  set(CLANG_ROOT ${DOWNLOADED_CLANG_DIR})

  if(${CMAKE_CXX_COMPILER_ID} STREQUAL Clang AND CLANG_USE_BUNDLED_LIBC++)
    message(STATUS "Using bundled libc++")
    target_compile_options(ccls PRIVATE -nostdinc++ -cxx-isystem ${CLANG_ROOT}/include/c++/v1)
    if(${CMAKE_SYSTEM_NAME} STREQUAL Linux)
      # Don't use -stdlib=libc++ because while ccls is linked with libc++, bundled clang+llvm require libstdc++
      target_link_libraries(ccls PRIVATE -L${CLANG_ROOT}/lib c++ c++abi)

      # FreeBSD defaults to -stdlib=libc++ and uses system libcxxrt.a
    endif()
  endif()

else()
  message(STATUS "Using system Clang")
endif()

### Libraries

# See cmake/FindClang.cmake
find_package(Clang ${CLANG_VERSION} REQUIRED)
target_link_libraries(ccls PRIVATE Clang::Clang)

# Enable threading support
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(ccls PRIVATE Threads::Threads)

if(${CMAKE_SYSTEM_NAME} STREQUAL Darwin)
  target_link_libraries(ccls PRIVATE -lc++experimental)

elseif(${CMAKE_SYSTEM_NAME} STREQUAL Linux)
  # loguru calls dladdr
  target_link_libraries(ccls PRIVATE ${CMAKE_DL_LIBS})

elseif(${CMAKE_SYSTEM_NAME} STREQUAL FreeBSD)
  # loguru::stacktrace_as_stdstring calls backtrace_symbols
  # src/platform_posix.cc uses libthr
  find_package(Backtrace REQUIRED)
  target_link_libraries(ccls PRIVATE ${Backtrace_LIBRARIES} thr)
  if(SYSTEM_CLANG)
    target_link_libraries(ccls PRIVATE c++experimental)
  endif()
endif()

### Definitions

target_compile_definitions(ccls PRIVATE
                           LOGURU_WITH_STREAMS=1
                           LOGURU_FILENAME_WIDTH=18
                           LOGURU_THREADNAME_WIDTH=13
                           DEFAULT_RESOURCE_DIRECTORY="${Clang_RESOURCE_DIR}")

### Includes

target_include_directories(ccls PRIVATE
                           src
                           third_party
                           third_party/rapidjson/include
                           third_party/loguru
                           third_party/doctest)

### Install

install(TARGETS ccls RUNTIME DESTINATION bin)

# TODO: install libclang.dll on Windows as well
if(NOT SYSTEM_CLANG AND NOT ${CMAKE_SYSTEM_NAME} STREQUAL Windows)

  if(${CMAKE_SYSTEM_NAME} MATCHES Linux|FreeBSD)
    set_property(TARGET ccls APPEND PROPERTY
                 INSTALL_RPATH $ORIGIN/../lib)
  elseif(${CMAKE_SYSTEM_NAME} STREQUAL Darwin)
    set_property(TARGET ccls APPEND PROPERTY
                 INSTALL_RPATH @loader_path/../lib)
  endif()

  file(GLOB LIBCLANG_PLUS_SYMLINKS
       ${DOWNLOADED_CLANG_DIR}/lib/libclang.[so,dylib]*)
  install(FILES ${LIBCLANG_PLUS_SYMLINKS} DESTINATION lib)
endif()

# Allow running from build Windows by copying libclang.dll to build directory
if(NOT SYSTEM_CLANG AND ${CMAKE_SYSTEM_NAME} STREQUAL Windows)
  add_custom_command(TARGET ccls
                     POST_BUILD
                     COMMAND ${CMAKE_COMMAND} -E copy
                     ${DOWNLOADED_CLANG_DIR}/bin/libclang.dll
                     $<TARGET_FILE_DIR:ccls>
                     COMMENT "Copying libclang.dll to build directory ...")
endif()

### Tools

# We use glob here since source files are already manually added with
# target_sources further down
file(GLOB SOURCES src/*.cc src/*.h src/serializers/*.cc src/serializers/*.h
                  src/messages/*.h src/messages/*.cc)

### Sources

target_sources(ccls PRIVATE third_party/siphash.cc)

target_sources(ccls PRIVATE
               src/clang_complete.cc
               src/clang_tu.cc
               src/clang_utils.cc
               src/config.cc
               src/diagnostics_engine.cc
               src/file_consumer.cc
               src/filesystem.cc
               src/fuzzy_match.cc
               src/main.cc
               src/include_complete.cc
               src/indexer.cc
               src/method.cc
               src/language.cc
               src/lex_utils.cc
               src/log.cc
               src/lsp.cc
               src/match.cc
               src/message_handler.cc
               src/pipeline.cc
               src/platform_posix.cc
               src/platform_win.cc
               src/position.cc
               src/project.cc
               src/query_utils.cc
               src/query.cc
               src/serializer.cc
               src/test.cc
               src/third_party_impl.cc
               src/timer.cc
               src/type_printer.cc
               src/utils.cc
               src/working_files.cc)

target_sources(ccls PRIVATE
               src/messages/ccls_base.cc
               src/messages/ccls_call_hierarchy.cc
               src/messages/ccls_callers.cc
               src/messages/ccls_file_info.cc
               src/messages/ccls_freshen_index.cc
               src/messages/ccls_inheritance_hierarchy.cc
               src/messages/ccls_member_hierarchy.cc
               src/messages/ccls_vars.cc
               src/messages/exit.cc
               src/messages/initialize.cc
               src/messages/shutdown.cc
               src/messages/text_document_code_lens.cc
               src/messages/text_document_completion.cc
               src/messages/text_document_definition.cc
               src/messages/text_document_did_change.cc
               src/messages/text_document_did_close.cc
               src/messages/text_document_did_open.cc
               src/messages/text_document_did_save.cc
               src/messages/text_document_document_highlight.cc
               src/messages/text_document_document_symbol.cc
               src/messages/text_document_hover.cc
               src/messages/text_document_implementation.cc
               src/messages/text_document_references.cc
               src/messages/text_document_rename.cc
               src/messages/text_document_signature_help.cc
               src/messages/text_document_type_definition.cc
               src/messages/workspace_did_change_configuration.cc
               src/messages/workspace_did_change_watched_files.cc
               src/messages/workspace_symbol.cc
               )
