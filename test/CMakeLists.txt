cmake_minimum_required(VERSION 3.10)
PROJECT(test)

message("-- Making vapp_sample")

if(MSVC)
    set(CMAKE_BUILD_TYPE Release)
    set(VAPP_LIB_DIR ${CMAKE_SOURCE_DIR}/../lib/${CMAKE_BUILD_TYPE})
    set(VAPP_SAMPLE_DIR ${CMAKE_SOURCE_DIR})
    set(VAPP_HEAD_FILE ${CMAKE_SOURCE_DIR}/../include)
    add_compile_options(
				/MP
				/W3
				/FC
				/D_CRT_SECURE_NO_WARNINGS
				/D_CRT_NONSTDC_NO_WARNINGS)
				
	set(VAPP_LIB_DIR ${CMAKE_SOURCE_DIR})
	set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/)
	
    file(GLOB SRC_LIST ${CMAKE_SOURCE_DIR}/*.c ${CMAKE_SOURCE_DIR}/*.cpp)
    include_directories(
         ${CMAKE_SOURCE_DIR}
         ${VAPP_HEAD_FILE}
    )
    
	add_executable(
        ${PROJECT_NAME}
		${SRC_LIST}
    )

	target_link_libraries(
		${PROJECT_NAME}
		${VAPP_LIB_DIR}/../lib/${CMAKE_BUILD_TYPE}/vappi.lib
	)
	
	target_compile_definitions(${PROJECT_NAME} PRIVATE OS_WINDOWS)
	target_compile_options(${PROJECT_NAME} PRIVATE 
        "/wd4101"   # disable "unused variables ..." warnings
        "/wd4996"   # disable "This function or variable may be unsafe ..." warnings
        "/wd4819"   # disable "The file contains a character that cannot be represented ..." warnings
        )
else()
    set(CMAKE_BUILD_TYPE Release)
    set(VAPP_LIB_DIR ${CMAKE_SOURCE_DIR}/../lib/)
    set(VAPP_SAMPLE_DIR ${CMAKE_SOURCE_DIR})
	set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/)
    set(VAPP_HEAD_FILE ${CMAKE_SOURCE_DIR}/../include)

	file(GLOB SRC_LIST ${CMAKE_SOURCE_DIR}/*.c ${CMAKE_SOURCE_DIR}/*.cpp)
	include_directories(
         ${CMAKE_SOURCE_DIR}
         ${VAPP_HEAD_FILE}
    )

	add_executable(
        ${PROJECT_NAME}
		${SRC_LIST}
    )

	target_link_libraries(
		${PROJECT_NAME}
		PUBLIC
		-Wl,--whole-archive
		-Wl,--start-group
		-Wl,-Bstatic
		-Wl,-Bdynamic
		${VAPP_LIB_DIR}/libvappi.so
		-Wl,--end-group
		-Wl,--no-whole-archive
		dl
		m
		pthread
	)
endif(MSVC)