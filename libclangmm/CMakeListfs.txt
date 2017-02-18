set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3 -Wall -Wextra -Wno-unused-parameter -Wno-reorder")
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_HOME_DIRECTORY}/cmake/Modules/")

message("Searcing for libclang")
#LIBCLANG_FOUND System has libclang.
#LIBCLANG_INCLUDE_DIRS The libclang include directories.
#LIBCLANG_LIBRARIES  The libraries needed to use libclang.
#LIBCLANG_LIBRARY_DIR The path to the directory containing libclang.
#LIBCLANG_KNOWN_LLVM_VERSIONS  Known LLVM release numbers.
find_package(LibClang REQUIRED)

set(header_files
  clangmm.h
  CodeCompleteResults.h 
  CompilationDatabase.h 
  CompileCommand.h      
  CompileCommands.h     
  CompletionString.h    
  Cursor.h
  Index.h               
  SourceLocation.h      
  SourceRange.h         
  Token.h               
  Tokens.h              
  TranslationUnit.h
  Diagnostic.h
  Utility.h
  )
set(cc_files
  CodeCompleteResults.cc
  CompilationDatabase.cc
  CompileCommand.cc     
  CompileCommands.cc    
  CompletionString.cc   
  Cursor.cc
  Index.cc              
  SourceLocation.cc     
  SourceRange.cc        
  Token.cc              
  Tokens.cc             
  TranslationUnit.cc
  Diagnostic.cc
  Utility.cc
  )

add_library(${project_name} SHARED ${header_files} ${cc_files})

include_directories(${LIBCLANG_INCLUDE_DIRS})
target_link_libraries(${project_name} ${LIBCLANG_LIBRARIES})

install(TARGETS ${project_name} RUNTIME DESTINATION bin LIBRARY DESTINATION lib)
install(FILES ${header_files} DESTINATION include/libclangmm)
