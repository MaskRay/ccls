# Downloads and extracts the Clang archive for the current system from
# https://releases.llvm.org
#
# Returns the extracted Clang archive directory in DOWNLOADED_CLANG_DIR
#
# Downloads 7-Zip to extract Clang if it isn't available in the PATH
function(download_and_extract_clang CLANG_DOWNLOAD_LOCATION)

set(CLANG_VERSION 7.0.0)
set(CLANG_ARCHIVE_EXT .tar.xz)

if(${CMAKE_SYSTEM_NAME} STREQUAL Linux)

  # Default to Ubuntu 16.04
  set(DEFAULT_UBUNTU_VERSION_NUMBER 16.04)

  # Select a Clang archive for other Ubuntu releases if the current platform
  # differs from the default one
  set(UBUNTU_VERSION_NUMBER ${DEFAULT_UBUNTU_VERSION_NUMBER})
  find_program(LSB_RELEASE_EXECUTABLE lsb_release)

  if(NOT LSB_RELEASE_EXECUTABLE)
    message(WARNING "Could not find lsb_release executable. \
Falling back to default Linux platform: Clang/LLVM archive for \
Ubuntu ${DEFAULT_UBUNTU_VERSION_NUMBER}.")

  else()
    execute_process(COMMAND ${LSB_RELEASE_EXECUTABLE} -si
      OUTPUT_VARIABLE LSB_RELEASE_ID
      OUTPUT_STRIP_TRAILING_WHITESPACE
      )

    if(${LSB_RELEASE_ID} STREQUAL Ubuntu)
      execute_process(COMMAND ${LSB_RELEASE_EXECUTABLE} -sr
        OUTPUT_VARIABLE LSB_RELEASE_NUMBER
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )

      set(UBUNTU_VERSION_NUMBER ${LSB_RELEASE_NUMBER})

    else()
      message(WARNING "Identified other Linux platform than Ubuntu. Falling \
back to default: Clang/LLVM for Ubuntu ${DEFAULT_UBUNTU_VERSION_NUMBER}.")

    endif()
  endif()

  set(CLANG_ARCHIVE_NAME "clang+llvm-${CLANG_VERSION}-x86_64-\
linux-gnu-ubuntu-${UBUNTU_VERSION_NUMBER}")

elseif(${CMAKE_SYSTEM_NAME} STREQUAL Darwin)

  set(CLANG_ARCHIVE_NAME clang+llvm-${CLANG_VERSION}-x86_64-apple-darwin)

elseif(${CMAKE_SYSTEM_NAME} STREQUAL Windows)

  set(CLANG_ARCHIVE_NAME LLVM-${CLANG_VERSION}-win64)
  set(CLANG_ARCHIVE_EXT .exe)

elseif(${CMAKE_SYSTEM_NAME} STREQUAL FreeBSD)

  set(CLANG_ARCHIVE_NAME clang+llvm-${CLANG_VERSION}-amd64-unknown-freebsd11)

endif()

set(CLANG_ARCHIVE_FULL_NAME ${CLANG_ARCHIVE_NAME}${CLANG_ARCHIVE_EXT})
set(CLANG_ARCHIVE_FILE ${CLANG_DOWNLOAD_LOCATION}/${CLANG_ARCHIVE_FULL_NAME})
set(CLANG_ARCHIVE_EXTRACT_DIR ${CLANG_DOWNLOAD_LOCATION}/${CLANG_ARCHIVE_NAME})
set(CLANG_ARCHIVE_URL
    https://releases.llvm.org/${CLANG_VERSION}/${CLANG_ARCHIVE_FULL_NAME})
set(CLANG_ARCHIVE_HASH_FILE
    ${CMAKE_SOURCE_DIR}/clang_archive_hashes/${CLANG_ARCHIVE_FULL_NAME}.SHA256)

# Exit if Clang is already downloaded and extracted
set(CLANG_ROOT ${CLANG_ARCHIVE_EXTRACT_DIR})
find_package(Clang ${CLANG_VERSION} QUIET)
if(Clang_FOUND)
  message(STATUS "Clang already downloaded")
  set(DOWNLOADED_CLANG_DIR ${CLANG_ARCHIVE_EXTRACT_DIR} PARENT_SCOPE)
  return()
endif()

if(NOT CLANG_ARCHIVE_NAME)
  message(FATAL_ERROR "No Clang archive url specified for current platform \
(${CMAKE_SYSTEM_NAME}). Please file an issue to get it added.")
endif()

if(NOT EXISTS ${CLANG_ARCHIVE_HASH_FILE})
  message(FATAL_ERROR "No SHA256 hash available for the current platform \
(${CMAKE_SYSTEM_NAME}) + clang version (${CLANG_VERSION}) combination. Please \
file an issue to get it added.")
endif()

# Download Clang archive
message(STATUS "Downloading Clang ${CLANG_VERSION} (${CLANG_ARCHIVE_URL}) ...")
file(DOWNLOAD ${CLANG_ARCHIVE_URL} ${CLANG_ARCHIVE_FILE}
     STATUS CLANG_ARCHIVE_DOWNLOAD_RESULT)

# Abort if download failed
list(GET ${CLANG_ARCHIVE_DOWNLOAD_RESULT} 0 ERROR_CODE)
if(${ERROR_CODE})
  list(GET ${CLANG_ARCHIVE_DOWNLOAD_RESULT} 1 ERROR_STRING)
  message(FATAL_ERROR ${ERROR_STRING})
endif()

# Retrieve expected hash from file and strip newline
file(READ ${CLANG_ARCHIVE_HASH_FILE} CLANG_ARCHIVE_EXPECTED_HASH)
string(STRIP ${CLANG_ARCHIVE_EXPECTED_HASH} CLANG_ARCHIVE_EXPECTED_HASH)
# Calculate actual hash
file(SHA256 ${CLANG_ARCHIVE_FILE} CLANG_ARCHIVE_HASH)
# Abort if hashes do not match
if(NOT ${CLANG_ARCHIVE_EXPECTED_HASH} STREQUAL ${CLANG_ARCHIVE_HASH})
  message(FATAL_ERROR "SHA256 hash of downloaded Clang does not match \
expected hash. Remove the build directory and try running CMake again. If this \
keeps happening, file an issue to report the problem.")
endif()

if(${CLANG_ARCHIVE_EXT} STREQUAL .exe)
  # Download and extract 7-zip if not found in PATH
  find_program(7ZIP_EXECUTABLE 7z)
  if(NOT 7ZIP_EXECUTABLE)
    message(STATUS "7-Zip not found in PATH")

    include(DownloadAndExtract7zip)
    download_and_extract_7zip(${CLANG_DOWNLOAD_LOCATION})
    find_program(7ZIP_EXECUTABLE
      NAMES 7z
      NO_DEFAULT_PATH
      PATHS ${DOWNLOADED_7ZIP_DIR}
    )
  else()
    message(STATUS "7-Zip found in PATH")
  endif()

  message(STATUS "Extracting downloaded Clang with 7-Zip ...")

  # Avoid running the Clang installer by extracting the exe with 7-Zip
  execute_process(
    COMMAND ${7ZIP_EXECUTABLE} x 
            -o${CLANG_ARCHIVE_EXTRACT_DIR} 
            -xr!$PLUGINSDIR ${CLANG_ARCHIVE_FILE}
    WORKING_DIRECTORY ${CLANG_DOWNLOAD_LOCATION}
    OUTPUT_QUIET
  )

elseif(${CLANG_ARCHIVE_EXT} STREQUAL .tar.xz)
  message(STATUS "Extracting downloaded Clang with CMake built-in tar ...")

  # CMake has builtin support for tar via the -E flag
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar -xf ${CLANG_ARCHIVE_FILE}
    # Specify working directory to allow running cmake from 
    # everywhere
    # (example: cmake -H"$HOME/cquery" -B"$home/cquery/build")
    WORKING_DIRECTORY ${CLANG_DOWNLOAD_LOCATION}
    OUTPUT_QUIET
  )
endif()

set(DOWNLOADED_CLANG_DIR ${CLANG_ARCHIVE_EXTRACT_DIR} PARENT_SCOPE)

endfunction()
