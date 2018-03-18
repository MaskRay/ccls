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

  # 6.0.0 uses freebsd-10 while 5.0.1 uses freebsd10
  set(CLANG_ARCHIVE_NAME clang+llvm-${CLANG_VERSION}-amd64-unknown-freebsd-10)

endif()

set(CLANG_ARCHIVE_FULL_NAME ${CLANG_ARCHIVE_NAME}${CLANG_ARCHIVE_EXT})
set(CLANG_ARCHIVE_FILE ${CMAKE_BINARY_DIR}/${CLANG_ARCHIVE_FULL_NAME})
set(CLANG_ARCHIVE_EXTRACT_DIR ${CMAKE_BINARY_DIR}/${CLANG_ARCHIVE_NAME})
set(CLANG_ARCHIVE_URL 
    https://releases.llvm.org/${CLANG_VERSION}/${CLANG_ARCHIVE_FULL_NAME})

if(NOT EXISTS ${CLANG_ARCHIVE_FILE})
  message(STATUS "Downloading LLVM ${CLANG_VERSION} (${CLANG_ARCHIVE_URL}) ...")
  file(DOWNLOAD ${CLANG_ARCHIVE_URL} ${CLANG_ARCHIVE_FILE})
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

  # There is a null pointer dereference issue in 
  # tools/libclang/CXIndexDataConsumer.cpp handleReference.
  # https://github.com/cquery-project/cquery/issues/219
  if(${CMAKE_SYSTEM_NAME} STREQUAL Linux AND 
     ${CLANG_VERSION} MATCHES 4.0.0|5.0.1)
    message(STATUS "Patching downloaded LLVM (see \
https://github.com/cquery-project/cquery/issues/219)")
    
    if(${CLANG_VERSION} STREQUAL 4.0.0)
      # 4289205 = $[0x4172b5] (we use decimals for seek since execute_process 
      # does not evaluate $[] bash syntax)
      execute_process(COMMAND printf \\x4d
                      COMMAND dd 
                        of=${CLANG_ARCHIVE_EXTRACT_DIR}/lib/libclang.so.4.0
                        obs=1 seek=4289205 conv=notrunc
                      OUTPUT_QUIET)

    elseif(${CLANG_VERSION} STREQUAL 5.0.1)
      # 4697806 = $[0x47aece] 
      execute_process(COMMAND printf \\x4d
                      COMMAND dd 
                        of=${CLANG_ARCHIVE_EXTRACT_DIR}/lib/libclang.so.5.0
                        obs=1 seek=4697806 conv=notrunc
                      OUTPUT_QUIET)
    endif()
  endif()
endif()

# CMake functions have no return values so we just lift our return variable to
# the parent scope
set(DOWNLOADED_CLANG_DIR ${CLANG_ARCHIVE_EXTRACT_DIR} PARENT_SCOPE)

endfunction()
