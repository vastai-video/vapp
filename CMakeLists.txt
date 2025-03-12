# Set the project name and minimum required version
cmake_minimum_required(VERSION 3.8)
set(TARGET_LIB vappi)
project(${TARGET_LIB})
set(CMAKE_SYSTEM_NAME Generic)
message("C Compiler ID: ${CMAKE_C_COMPILER_ID}")

set(CMAKE_BUILD_TYPE Release)


set (CMAKE_C_STANDARD 11)
set (CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

set(SOURCE_DIR "src")
file(GLOB SOURCE_FILES "${SOURCE_DIR}/*.c")
add_library(${TARGET_LIB} SHARED ${SOURCE_FILES})


set_target_properties(${TARGET_LIB} PROPERTIES OUTPUT_NAME ${TARGET_LIB})

file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)
# Set the output directory for the generated DLL to the lib folder under the build directory
if (NOT DEFINED RUNTIME_LIBRARY_OUTPUT_PATH)
    set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib)
elseif(RUNTIME_LIBRARY_OUTPUT_PATH)
    set(LIBRARY_OUTPUT_PATH ${RUNTIME_LIBRARY_OUTPUT_PATH})
else()
    message(WARNING "RUNTIME_LIBRARY_OUTPUT_PATH is empty")
endif()


# Set compiler options if needed
# target_compile_options(${TARGET_LIB} PRIVATE -Wall -Wextra)

# Add include directory if any
target_include_directories(${TARGET_LIB} PUBLIC include)
add_compile_options( -O2 )
add_compile_definitions(LIBVAPP_EXPORTS)	
