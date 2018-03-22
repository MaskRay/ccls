cmake_minimum_required(VERSION 3.1)
project(cquery LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/)
include(DefaultCMakeBuildType)
include(DownloadAndExtractLLVM)

set(CLANG_VERSION 6.0.0 CACHE STRING "Downloaded Clang version (6.0.0)")
option(SYSTEM_CLANG "Use system Clang instead of downloading Clang" OFF)
option(ASAN "Compile with address sanitizers" OFF)
option(CLANG_CXX "Build with Clang C++ api required by some cquery \
features (warning: not available in LLVM Windows downloads)" OFF)

# Sources for the executable are specified at end of CMakeLists.txt
add_executable(cquery "")

### Compile options

# CMake default compile flags:
# MSVC + Clang(Windows):
#   debug: /MDd /Zi /Ob0 /Od /RTC1
#   release: /MD /O2 /Ob2 /DNDEBUG
# GCC + Clang(Linux):
#   debug: -g
#   release: -O3 -DNDEBUG

# Enable C++14 (Required)
set_property(TARGET cquery PROPERTY CXX_STANDARD 14)
set_property(TARGET cquery PROPERTY CXX_STANDARD_REQUIRED ON)
# Disable gcc extensions
set_property(TARGET cquery PROPERTY CXX_EXTENSIONS OFF)

if(${CMAKE_SYSTEM_NAME} STREQUAL Windows)
  # Common MSVC/Clang(Windows) options
  target_compile_options(cquery PRIVATE
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
  target_compile_options(cquery PRIVATE
                         -Wall
                         -Wno-sign-compare
                         )

  if(${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
    target_compile_options(cquery PRIVATE -Wno-return-type -Wno-unused-result)
  endif()

  if(${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
    target_compile_options(cquery PRIVATE
                           $<$<CONFIG:Debug>:-fno-limit-debug-info>)
  endif()

  if(CLANG_CXX)
    # -Wno-comment: include/clang/Format/Format.h error: multi-line comment
    # -fno-rtti: # Without -fno-rtti, some Clang C++ functions may report
    # `undefined references to typeinfo`
    target_compile_options(cquery PRIVATE -Wno-comment -fno-rtti)
  endif()

  if(ASAN)
    target_compile_options(cquery PRIVATE -fsanitize=address,undefined)
    # target_link_libraries also takes linker flags
    target_link_libraries(cquery PRIVATE -fsanitize=address,undefined)
  endif()
endif()

### Download Clang if required

if(NOT SYSTEM_CLANG)
  download_and_extract_llvm(${CLANG_VERSION})
  # Used by FindClang
  set(CLANG_ROOT ${DOWNLOADED_CLANG_DIR})
endif()

### Libraries

# See cmake/FindClang.cmake
find_package(Clang REQUIRED)
target_link_libraries(cquery PRIVATE Clang::Clang)

# Enable threading support
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(cquery PRIVATE Threads::Threads)

if(${CMAKE_SYSTEM_NAME} STREQUAL Linux)
  # loguru calls dladdr
  target_link_libraries(cquery PRIVATE ${CMAKE_DL_LIBS})

elseif(${CMAKE_SYSTEM_NAME} STREQUAL FreeBSD)
  # loguru::stacktrace_as_stdstring calls backtrace_symbols
  # sparsepp/spp_memory.h uses libkvm
  # src/platform_posix.cc uses libthr
  find_package(Backtrace REQUIRED)
  target_link_libraries(cquery PRIVATE ${Backtrace_LIBRARIES} kvm thr)

elseif(${CMAKE_SYSTEM_NAME} STREQUAL Windows)
  # sparsepp/spp_memory.h uses LibPsapi
  target_link_libraries(cquery PRIVATE Psapi)
endif()

if(CLANG_CXX)
  # Clang C++ api uses ncurses
  find_package(Curses REQUIRED)
  target_link_libraries(cquery PRIVATE ${CURSES_LIBRARIES})
endif()

### Definitions

target_compile_definitions(cquery PRIVATE
                           LOGURU_WITH_STREAMS=1
                           LOGURU_FILENAME_WIDTH=18
                           LOGURU_THREADNAME_WIDTH=13
                           DEFAULT_RESOURCE_DIRECTORY="${Clang_RESOURCE_DIR}")

if(CLANG_CXX)
  target_compile_definitions(cquery PRIVATE USE_CLANG_CXX=1 LOGURU_RTTI=0)
endif()

### Includes

target_include_directories(cquery PRIVATE
                           src
                           third_party
                           third_party/rapidjson/include
                           third_party/sparsepp
                           third_party/loguru
                           third_party/doctest
                           third_party/msgpack-c/include)

### Install

install(TARGETS cquery RUNTIME DESTINATION bin)

# We don't need to install libclang on Windows if we are using downloaded LLVM
# since libclang is distributed as a static library on Windows
if(NOT SYSTEM_CLANG AND NOT ${CMAKE_SYSTEM_NAME} STREQUAL Windows)

  if(${CMAKE_SYSTEM_NAME} MATCHES Linux|FreeBSD)
    set_property(TARGET cquery APPEND PROPERTY
                 INSTALL_RPATH $ORIGIN/../lib)
  elseif(${CMAKE_SYSTEM_NAME} STREQUAL Darwin)
    set_property(TARGET cquery APPEND PROPERTY
                 INSTALL_RPATH @loader_path/../lib)
  endif()

  file(GLOB LIBCLANG_PLUS_SYMLINKS
       ${DOWNLOADED_CLANG_DIR}/lib/libclang.[so,dylib]*)
  install(FILES ${LIBCLANG_PLUS_SYMLINKS} DESTINATION lib)
endif()

### Sources

target_sources(cquery PRIVATE third_party/siphash.cc)

target_sources(cquery PRIVATE
               src/cache_manager.cc
               src/clang_complete.cc
               src/clang_cursor.cc
               src/clang_format.cc
               src/clang_index.cc
               src/clang_indexer.cc
               src/clang_translation_unit.cc
               src/clang_utils.cc
               src/code_complete_cache.cc
               src/command_line.cc
               src/diagnostics_engine.cc
               src/file_consumer.cc
               src/file_contents.cc
               src/fuzzy_match.cc
               src/iindexer.cc
               src/import_manager.cc
               src/import_pipeline.cc
               src/include_complete.cc
               src/method.cc
               src/lex_utils.cc
               src/lsp_diagnostic.cc
               src/lsp.cc
               src/match.cc
               src/message_handler.cc
               src/options.cc
               src/platform_posix.cc
               src/platform_win.cc
               src/platform.cc
               src/port.cc
               src/position.cc
               src/project.cc
               src/query_utils.cc
               src/query.cc
               src/queue_manager.cc
               src/recorder.cc
               src/semantic_highlight_symbol_cache.cc
               src/serializer.cc
               src/standard_includes.cc
               src/task.cc
               src/test.cc
               src/third_party_impl.cc
               src/timer.cc
               src/timestamp_manager.cc
               src/type_printer.cc
               src/utils.cc
               src/work_thread.cc
               src/working_files.cc)

target_sources(cquery PRIVATE
               src/messages/cquery_base.cc
               src/messages/cquery_call_hierarchy.cc
               src/messages/cquery_callers.cc
               src/messages/cquery_derived.cc
               src/messages/cquery_did_view.cc
               src/messages/cquery_file_info.cc
               src/messages/cquery_freshen_index.cc
               src/messages/cquery_index_file.cc
               src/messages/cquery_inheritance_hierarchy.cc
               src/messages/cquery_member_hierarchy.cc
               src/messages/cquery_random.cc
               src/messages/cquery_vars.cc
               src/messages/cquery_wait.cc
               src/messages/exit.cc
               src/messages/initialize.cc
               src/messages/shutdown.cc
               src/messages/text_document_code_action.cc
               src/messages/text_document_code_lens.cc
               src/messages/text_document_completion.cc
               src/messages/text_document_definition.cc
               src/messages/text_document_did_change.cc
               src/messages/text_document_did_close.cc
               src/messages/text_document_did_open.cc
               src/messages/text_document_did_save.cc
               src/messages/text_document_document_highlight.cc
               src/messages/text_document_document_link.cc
               src/messages/text_document_document_symbol.cc
               src/messages/text_document_formatting.cc
               src/messages/text_document_hover.cc
               src/messages/text_document_range_formatting.cc
               src/messages/text_document_references.cc
               src/messages/text_document_rename.cc
               src/messages/text_document_signature_help.cc
               src/messages/text_document_type_definition.cc
               src/messages/workspace_did_change_configuration.cc
               src/messages/workspace_did_change_watched_files.cc
               src/messages/workspace_symbol.cc)
