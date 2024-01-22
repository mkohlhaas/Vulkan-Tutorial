cmake_minimum_required(VERSION 3.27)

project(triangle LANGUAGES C)

find_package(Vulkan REQUIRED)
find_package(glfw3 3.3 REQUIRED)

# GLAD
# add_library(glad SHARED glad.c)
# target_include_directories(glad PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(${PROJECT_NAME} main.c)
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan glfw)
