#.rst
# FindClang
# ---------
#
# Find Clang and LLVM libraries required by cquery
#
# Results are reported in the following variables::
#
#   Clang_FOUND         - True if headers and requested libraries were found
#   Clang_EXECUTABLE    - Clang executable
#   Clang_RESOURCE_DIR  - Clang resource directory
#   Clang_VERSION       - Clang version as reported by Clang executable
#
# The following :prop_tgt:`IMPORTED` targets are also defined::
#
#   Clang::Clang        - Target for all required Clang libraries and headers
#
# This module reads hints about which libraries to look for and where to find
# them from the following variables::
#
#   CLANG_CXX           - Search for and add Clang C++ libraries
#   CLANG_ROOT          - If set, only look for Clang components in CLANG_ROOT
#
# Example to link against Clang target::
#
#   target_link_libraries(<target> PRIVATE Clang::Clang)

### Definitions

# Wrapper macro's around the find_* macro's from CMake that only search in
# CLANG_ROOT if it is defined

macro(_Clang_find_library VAR NAME)
  # Windows needs lib prefix
  if (CLANG_ROOT)    
    find_library(${VAR} NAMES ${NAME} lib${NAME} 
                 NO_DEFAULT_PATH PATHS ${CLANG_ROOT} PATH_SUFFIXES lib)
  else()
    find_library(${VAR} NAMES ${NAME} lib${NAME})
  endif()
endmacro()

macro(_Clang_find_path VAR INCLUDE_FILE)
  if (CLANG_ROOT)
    find_path(${VAR} ${INCLUDE_FILE} 
              NO_DEFAULT_PATH PATHS ${CLANG_ROOT} PATH_SUFFIXES include)
  else()
    find_path(${VAR} ${INCLUDE_FILE})
  endif()
endmacro()

macro(_Clang_find_program VAR NAME)
  if (CLANG_ROOT)
    find_program(${VAR} ${NAME} 
                 NO_DEFAULT_PATH PATHS ${CLANG_ROOT} PATH_SUFFIXES bin)
  else()
    find_program(${VAR} ${NAME})
  endif()
endmacro()

# Macro to avoid duplicating logic for each Clang C++ library
macro(_Clang_find_and_add_cxx_lib NAME INCLUDE_FILE)
  # Find library
  _Clang_find_library(Clang_${NAME}_LIBRARY ${NAME})
  list(APPEND _Clang_REQUIRED_VARS Clang_${NAME}_LIBRARY)
  list(APPEND _Clang_CXX_LIBRARIES ${Clang_${NAME}_LIBRARY})

  # Find corresponding include directory
  _Clang_find_path(Clang_${NAME}_INCLUDE_DIR ${INCLUDE_FILE})
  list(APPEND _Clang_REQUIRED_VARS Clang_${NAME}_INCLUDE_DIR)
  list(APPEND _Clang_CXX_INCLUDE_DIRS ${Clang_${NAME}_INCLUDE_DIR})
endmacro()

### Start

set(_Clang_REQUIRED_VARS Clang_LIBRARY Clang_INCLUDE_DIR Clang_EXECUTABLE 
                         Clang_RESOURCE_DIR Clang_VERSION)

_Clang_find_library(Clang_LIBRARY clang)
_Clang_find_path(Clang_INCLUDE_DIR clang-c/Index.h)

if(CLANG_CXX)
  # The order is derived by topological sorting LINK_LIBS in 
  # clang/lib/*/CMakeLists.txt
  _Clang_find_and_add_cxx_lib(clangFormat clang/Format/Format.h)
  _Clang_find_and_add_cxx_lib(clangToolingCore clang/Tooling/Core/Diagnostic.h)
  _Clang_find_and_add_cxx_lib(clangRewrite clang/Rewrite/Core/Rewriter.h)
  _Clang_find_and_add_cxx_lib(clangAST clang/AST/AST.h)
  _Clang_find_and_add_cxx_lib(clangLex clang/Lex/Lexer.h)
  _Clang_find_and_add_cxx_lib(clangBasic clang/Basic/ABI.h)

  # The order is derived from llvm-config --libs core
  _Clang_find_and_add_cxx_lib(LLVMCore llvm/Pass.h)
  _Clang_find_and_add_cxx_lib(LLVMBinaryFormat llvm/BinaryFormat/Dwarf.h)
  _Clang_find_and_add_cxx_lib(LLVMSupport llvm/Support/Error.h)
  _Clang_find_and_add_cxx_lib(LLVMDemangle llvm/Demangle/Demangle.h)
endif()

_Clang_find_program(Clang_EXECUTABLE clang)
if(Clang_EXECUTABLE)
  # Find Clang resource directory with Clang executable
  # TODO: simplify by using -print-resource-dir once Clang 4 support is dropped
  if(${CMAKE_SYSTEM_NAME} STREQUAL Windows)
    set(_DEV_NULL NUL)
  else()
    set(_DEV_NULL /dev/null)
  endif()

  # clang "-###" -xc /dev/null
  execute_process(COMMAND ${Clang_EXECUTABLE} "-###" -xc ${_DEV_NULL} 
                  ERROR_VARIABLE Clang_RESOURCE_DIR OUTPUT_QUIET)
  # Strip everything except '"-resource-dir" "<resource-dir-path>"'
  string(REGEX MATCH "\"-resource-dir\" \"([^\"]*)\"" 
         Clang_RESOURCE_DIR ${Clang_RESOURCE_DIR})
  # Strip quotes
  string(REPLACE "\"" "" Clang_RESOURCE_DIR ${Clang_RESOURCE_DIR})
  # Strip '-resource-dir '
  string(REPLACE "-resource-dir " "" Clang_RESOURCE_DIR ${Clang_RESOURCE_DIR})

  # Find Clang version
  set(_Clang_VERSION_REGEX "([0-9]+)\\.([0-9]+)\\.([0-9]+)")
  execute_process(COMMAND ${Clang_EXECUTABLE} --version 
                  OUTPUT_VARIABLE Clang_VERSION)
  string(REGEX MATCH ${_Clang_VERSION_REGEX} Clang_VERSION ${Clang_VERSION})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Clang
  FOUND_VAR Clang_FOUND
  REQUIRED_VARS ${_Clang_REQUIRED_VARS}
  VERSION_VAR Clang_VERSION
)

if(Clang_FOUND AND NOT TARGET Clang::Clang)
  set(_Clang_LIBRARIES ${Clang_LIBRARY} ${_Clang_CXX_LIBRARIES})
  set(_Clang_INCLUDE_DIRS ${Clang_INCLUDE_DIR} ${_Clang_CXX_INCLUDE_DIRS})

  add_library(Clang::Clang INTERFACE IMPORTED)
  set_property(TARGET Clang::Clang PROPERTY 
               INTERFACE_LINK_LIBRARIES ${_Clang_LIBRARIES})
  set_property(TARGET Clang::Clang PROPERTY 
               INTERFACE_INCLUDE_DIRECTORIES ${_Clang_INCLUDE_DIRS})  
endif()
