# Downloads and extracts the LLVM archive for the current system from
# https://releases.llvm.org
#
# Returns the extracted LLVM archive directory in DOWNLOADED_CLANG_DIR
#
# Downloads 7-Zip to extract LLVM if it isn't available in the PATH
function(download_and_extract_llvm CLANG_VERSION)

include(DownloadAndExtract7zip)

set(CLANG_ARCHIVE_EXT .tar.xz)

if(${CMAKE_SYSTEM_NAME} STREQUAL Linux)

  set(CLANG_ARCHIVE_NAME
      clang+llvm-${CLANG_VERSION}-x86_64-linux-gnu-ubuntu-14.04)

elseif(${CMAKE_SYSTEM_NAME} STREQUAL Darwin)

  set(CLANG_ARCHIVE_NAME clang+llvm-${CLANG_VERSION}-x86_64-apple-darwin)

elseif(${CMAKE_SYSTEM_NAME} STREQUAL Windows)

  set(CLANG_ARCHIVE_NAME LLVM-${CLANG_VERSION}-win64)
  set(CLANG_ARCHIVE_EXT .exe)

elseif(${CMAKE_SYSTEM_NAME} STREQUAL FreeBSD)

  if(${CLANG_VERSION} STREQUAL 6.0.0)
    set(CLANG_ARCHIVE_NAME clang+llvm-${CLANG_VERSION}-amd64-unknown-freebsd-10)
  else()
    set(CLANG_ARCHIVE_NAME clang+llvm-${CLANG_VERSION}-amd64-unknown-freebsd10)
  endif()
endif()

if(NOT CLANG_ARCHIVE_NAME)
  message(FATAL_ERROR "No LLVM archive url specified for current platform \
(${CMAKE_SYSTEM_NAME}). Please file an issue to get it added.")
endif()

set(CLANG_ARCHIVE_FULL_NAME ${CLANG_ARCHIVE_NAME}${CLANG_ARCHIVE_EXT})
set(CLANG_ARCHIVE_FILE ${CMAKE_BINARY_DIR}/${CLANG_ARCHIVE_FULL_NAME})
set(CLANG_ARCHIVE_EXTRACT_DIR ${CMAKE_BINARY_DIR}/${CLANG_ARCHIVE_NAME})
set(CLANG_ARCHIVE_URL
    https://releases.llvm.org/${CLANG_VERSION}/${CLANG_ARCHIVE_FULL_NAME})
set(CLANG_ARCHIVE_HASH_FILE
    ${CMAKE_SOURCE_DIR}/clang_archive_hashes/${CLANG_ARCHIVE_FULL_NAME}.SHA256)

if(NOT EXISTS ${CLANG_ARCHIVE_HASH_FILE})
  message(FATAL_ERROR "No SHA256 hash available for the current platform \
(${CMAKE_SYSTEM_NAME}) + clang version (${CLANG_VERSION}) combination. Please \
file an issue to get it added.")
endif()

file(READ ${CLANG_ARCHIVE_HASH_FILE} CLANG_ARCHIVE_EXPECTED_HASH)
# Strip newline
string(STRIP ${CLANG_ARCHIVE_EXPECTED_HASH} CLANG_ARCHIVE_EXPECTED_HASH)

if(NOT EXISTS ${CLANG_ARCHIVE_FILE})
  message(STATUS "Downloading LLVM ${CLANG_VERSION} (${CLANG_ARCHIVE_URL}) ...")
  file(DOWNLOAD ${CLANG_ARCHIVE_URL} ${CLANG_ARCHIVE_FILE}
       STATUS CLANG_ARCHIVE_DOWNLOAD_RESULT)

  list(GET ${CLANG_ARCHIVE_DOWNLOAD_RESULT} 0 ERROR_CODE)
  if(${ERROR_CODE})
    list(GET ${CLANG_ARCHIVE_DOWNLOAD_RESULT} 1 ERROR_STRING)
    message(FATAL_ERROR ${ERROR_STRING})
  endif()
endif()

file(SHA256 ${CLANG_ARCHIVE_FILE} CLANG_ARCHIVE_HASH)
if(NOT ${CLANG_ARCHIVE_EXPECTED_HASH} STREQUAL ${CLANG_ARCHIVE_HASH})
  message(FATAL_ERROR "SHA256 hash of downloaded LLVM does not match \
expected hash. Remove the build directory and try running CMake again. If this \
keeps happening, file an issue to report the problem.")
endif()

if(NOT EXISTS ${CLANG_ARCHIVE_EXTRACT_DIR})
  if(${CLANG_ARCHIVE_EXT} STREQUAL .exe)
    find_program(7ZIP_EXECUTABLE 7z)

    if(NOT 7ZIP_EXECUTABLE)
      message(STATUS "7-Zip not found in PATH")
      download_and_extract_7zip()
      find_program(7ZIP_EXECUTABLE 7z NO_DEFAULT_PATH
                   PATHS ${DOWNLOADED_7ZIP_DIR})
    else()
      message(STATUS "7-Zip found in PATH")
    endif()

    message(STATUS "Extracting downloaded LLVM with 7-Zip ...")

    # Avoid running the LLVM installer by extracting the exe with 7-Zip
    execute_process(COMMAND ${7ZIP_EXECUTABLE} x
                            -o${CLANG_ARCHIVE_EXTRACT_DIR}
                            -xr!$PLUGINSDIR ${CLANG_ARCHIVE_FILE}
                    OUTPUT_QUIET)
  elseif(${CLANG_ARCHIVE_EXT} STREQUAL .tar.xz)
    message(STATUS "Extracting downloaded LLVM with CMake built-in tar ...")
    # CMake has builtin support for tar via the -E flag
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar -xf ${CLANG_ARCHIVE_FILE}
                    OUTPUT_QUIET)
  endif()
endif()

# CMake functions have no return values so we just lift our return variable to
# the parent scope
set(DOWNLOADED_CLANG_DIR ${CLANG_ARCHIVE_EXTRACT_DIR} PARENT_SCOPE)

endfunction()
