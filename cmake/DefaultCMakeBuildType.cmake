# Set a default build type if none was specified
set(DEFAULT_CMAKE_BUILD_TYPE Release)
if(EXISTS ${CMAKE_SOURCE_DIR}/.git)
  set(DEFAULT_CMAKE_BUILD_TYPE Debug)
endif()

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
