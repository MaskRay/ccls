# Downloads and extracts the 7-Zip MSI installer from https://www.7-zip.org/. 
# 
# Returns the extracted 7-Zip directory in DOWNLOADED_7ZIP_DIR
#
# Downloads 7-Zip to extract LLVM if it isn't available in the PATH
function(download_and_extract_7zip)

set(7ZIP_VERSION 1801)
set(7ZIP_EXT .msi)
set(7ZIP_NAME 7z${7ZIP_VERSION}-x64)
set(7ZIP_FULL_NAME ${7ZIP_NAME}${7ZIP_EXT})

set(7ZIP_FILE ${CMAKE_BINARY_DIR}/${7ZIP_FULL_NAME})
set(7ZIP_EXTRACT_DIR ${CMAKE_BINARY_DIR}/${7ZIP_NAME})
set(7ZIP_URL https://www.7-zip.org/a/${7ZIP_FULL_NAME})

# msiexec requires Windows path separators (\)
file(TO_NATIVE_PATH ${7ZIP_FILE} 7ZIP_FILE)
file(TO_NATIVE_PATH ${7ZIP_EXTRACT_DIR} 7ZIP_EXTRACT_DIR) 

if(NOT EXISTS ${7ZIP_FILE})
  message(STATUS "Downloading 7-Zip ${7ZIP_VERSION} (${7ZIP_URL}) ...")
  file(DOWNLOAD ${7ZIP_URL} ${7ZIP_FILE})
endif()

if(NOT EXISTS ${7ZIP_EXTRACT_DIR})

  find_program(MSIEXEC_EXECUTABLE msiexec)
  if(NOT MSIEXEC_EXECUTABLE)
    message(FATAL_ERROR "Unable to find msiexec (required to extract 7-Zip msi \
installer). Install 7-Zip yourself and make sure it is available in the path")    
  endif()

  message(STATUS "Extracting downloaded 7-Zip ...")

  # msiexec with /a option allows extraction of msi installers without requiring
  # admin privileges. We use this to extract the 7-Zip installer without
  # requiring any actions from the user
  execute_process(COMMAND ${MSIEXEC_EXECUTABLE} /a ${7ZIP_FILE} /qn
                          TARGETDIR=${7ZIP_EXTRACT_DIR}
                  OUTPUT_QUIET)     
endif()

# Convert back to CMake separators (/) before returning
file(TO_CMAKE_PATH ${7ZIP_EXTRACT_DIR} 7ZIP_EXTRACT_DIR)

# Actual directory is nested inside the extract directory. We return the nested
# directory instead of the extract directory
set(DOWNLOADED_7ZIP_DIR ${7ZIP_EXTRACT_DIR}/Files/7-Zip PARENT_SCOPE)

endfunction()