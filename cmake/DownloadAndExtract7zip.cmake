# Downloads and extracts the 7-Zip MSI installer from https://www.7-zip.org/. 
# 
# Returns the extracted 7-Zip directory in DOWNLOADED_7ZIP_DIR
function(download_and_extract_7zip 7ZIP_DOWNLOAD_LOCATION)

set(7ZIP_VERSION 1801)
set(7ZIP_EXT .msi)
set(7ZIP_NAME 7z${7ZIP_VERSION}-x64)
set(7ZIP_FULL_NAME ${7ZIP_NAME}${7ZIP_EXT})

set(7ZIP_FILE ${7ZIP_DOWNLOAD_LOCATION}/${7ZIP_FULL_NAME})
set(7ZIP_EXTRACT_DIR ${7ZIP_DOWNLOAD_LOCATION}/${7ZIP_NAME})
set(7ZIP_URL https://www.7-zip.org/a/${7ZIP_FULL_NAME})

# Exit if 7-Zip is already downloaded and extracted
find_program(7ZIP_EXECUTABLE 7z NO_DEFAULT_PATH
             PATHS ${7ZIP_EXTRACT_DIR}/Files/7-Zip)
if(7ZIP_EXECUTABLE)
  message(STATUS "7-Zip already downloaded")
  return()
endif()

message(STATUS "Downloading 7-Zip ${7ZIP_VERSION} (${7ZIP_URL}) ...")
file(DOWNLOAD ${7ZIP_URL} ${7ZIP_FILE})

find_program(MSIEXEC_EXECUTABLE msiexec)
if(NOT MSIEXEC_EXECUTABLE)
  message(FATAL_ERROR "Unable to find msiexec (required to extract 7-Zip msi \
installer). Install 7-Zip yourself and make sure it is available in the path")    
endif()

message(STATUS "Extracting downloaded 7-Zip ...")

# msiexec requires Windows path separators (\)
file(TO_NATIVE_PATH ${7ZIP_FILE} 7ZIP_FILE)
file(TO_NATIVE_PATH ${7ZIP_EXTRACT_DIR} 7ZIP_EXTRACT_DIR) 

# msiexec with /a option allows extraction of msi installers without requiring
# admin privileges. We use this to extract the 7-Zip installer without
# requiring any actions from the user
execute_process(COMMAND ${MSIEXEC_EXECUTABLE} /a ${7ZIP_FILE} /qn
                        TARGETDIR=${7ZIP_EXTRACT_DIR}
                WORKING_DIRECTORY ${7ZIP_DOWNLOAD_LOCATION}
                OUTPUT_QUIET)     

# Convert back to CMake separators (/) before returning
file(TO_CMAKE_PATH ${7ZIP_EXTRACT_DIR} 7ZIP_EXTRACT_DIR)

# Actual 7-Zip directory is nested inside the extract directory.
set(DOWNLOADED_7ZIP_DIR ${7ZIP_EXTRACT_DIR}/Files/7-Zip PARENT_SCOPE)

endfunction()