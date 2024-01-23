cmake_minimum_required(VERSION 3.27)

project(vktutorial LANGUAGES C)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

# GLAD
# add_library(glad SHARED glad.c)
# target_include_directories(glad PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(${PROJECT_NAME} src/main.c src/vulkan.c src/window.c src/error.c)
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan glfw)
