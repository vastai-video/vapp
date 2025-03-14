cmake_minimum_required(VERSION 3.10)
project(vappi)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set (CMAKE_C_STANDARD 99)
set (CMAKE_CXX_STANDARD 11)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
    message(STATUS "use ccache")
endif(CCACHE_FOUND)

set(VAPP_MAJOR_VERSION 1)
set(VAPP_MINOR_VERSION 1)
set(VAPP_PATCH_VERSION 0)
set(VAPPI_VERSION "libvappi.so ${VAPP_MAJOR_VERSION}.${VAPP_MINOR_VERSION}.${VAPP_PATCH_VERSION} ")
add_definitions(-DVAPPI_VERSION="${VAPPI_VERSION}")
message(STATUS "vapp version ${VAPPI_VERSION}")

string(TIMESTAMP BUILD_DATE "%Y%m%d")

#######################################################
################  Get Repo git info  ##################
#######################################################
set(GIT_COMMIT "")
set(GIT_BRANCH "")
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} symbolic-ref --short -q HEAD
        OUTPUT_VARIABLE GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    execute_process(
        COMMAND ${GIT_EXECUTABLE} log -1 --pretty=format:%h
        OUTPUT_VARIABLE GIT_COMMIT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

add_definitions(-DVAPP_GIT_VERSION="${GIT_BRANCH} ${GIT_COMMIT}")
message(STATUS "vapp git version is ${GIT_BRANCH}:${GIT_COMMIT}")

string(TIMESTAMP BUILD_DATE "%a %d %b %Y %H:%M:%S")
add_definitions(-DBUILD_DATE="${BUILD_DATE}")
message(STATUS "Build date ${BUILD_DATE}")

#######################################################
##################  Build config  #####################
#######################################################
if("${BUILD_TYPE}" STREQUAL "")
    message(WARNING  "Undefined build type ${BUILD_TYPE}. Setting default build type rel")
    set(BUILD_TYPE "rel")           # setting default rel
endif()

if("${BUILD_TYPE}" STREQUAL "rel")
    set(CMAKE_BUILD_TYPE "Release")

    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2 -D_FORTIFY_SOURCE=2 -fstack-protector-all")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -D_FORTIFY_SOURCE=2 -fstack-protector-all")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fPIE -pie -s -Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now")

elseif("${BUILD_TYPE}" STREQUAL "dbg")
    set(CMAKE_BUILD_TYPE "Debug")

    set(CMAKE_C_FLAGS_DEBUG "-fsanitize=address -g -O0 -DDEBUG ${CMAKE_C_FLAGS_DEBUG}")
    set(CMAKE_CXX_FLAGS_DEBUG "-fsanitize=address -g -O0 -DDEBUG ${CMAKE_CXX_FLAGS_DEBUG}")

endif()

add_definitions("-Wall -fPIC")

#######################################################
#################### SOURCE CODE  #####################
#######################################################
include_directories(${PROJECT_SOURCE_DIR}/include)
aux_source_directory(${PROJECT_SOURCE_DIR}/src GROUP_SOURCE)

if(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Linux")
    message(STATUS "Building for Linux")
    add_definitions("-D__linux__")

elseif(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Windows")
    message(STATUS "Building for Windows")
    add_definitions("-D_WIN32")

endif()

if(UT)
    add_subdirectory(test)
endif()

add_library(vappi SHARED ${GROUP_SOURCE})

#######################################################
########## INSTALL and OUTPUT DIRECTORY  ##############
#######################################################
if(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Linux")
    set(target_os "linux")
elseif(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Windows")
	set(target_os "windows")
endif()

set(OUTPUT_DIR "${CMAKE_SYSTEM_PROCESSOR}-${target_os}-${BUILD_TYPE}")
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/${OUTPUT_DIR}")

#######################################################
################       install      ###################
#######################################################
set_target_properties(vappi PROPERTIES VERSION ${VAPP_MAJOR_VERSION}.${VAPP_MINOR_VERSION} SOVERSION ${VAPP_MAJOR_VERSION})

install(TARGETS vappi LIBRARY DESTINATION lib/ ARCHIVE DESTINATION lib/)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ DESTINATION include)
