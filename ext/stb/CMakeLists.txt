cmake_minimum_required(VERSION 3.2)

project(stb C)

add_library(${PROJECT_NAME} src/stb.c include/stb_image.h)
target_include_directories(${PROJECT_NAME} PUBLIC include)
