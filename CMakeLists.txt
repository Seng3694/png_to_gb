cmake_minimum_required(VERSION 3.2)

add_subdirectory(ext)

project(png_to_gb C)

set(HEADER_FILES
	include/color.h
	include/image.h
)

set(SOURCE_FILES
	src/image.c
    src/main.c
)

include_directories(include)
add_executable(${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} CLI)
target_link_libraries(${PROJECT_NAME} stb)