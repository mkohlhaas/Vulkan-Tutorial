cmake_minimum_required(VERSION 3.27)

project(vktutorial LANGUAGES C)

find_package(Vulkan    REQUIRED)
find_package(glfw3     REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(FLEX      REQUIRED)

FLEX_TARGET(SCANNER src/lexer.l ${CMAKE_CURRENT_BINARY_DIR}/lexer.c)

# Glib2
pkg_check_modules(GLIB2 REQUIRED glib-2.0)
include_directories(${GLIB2_INCLUDE_DIRS})
link_directories(${GLIB2_LIBRARY_DIRS})
link_libraries(${GLIB2_LIBRARIES})

# cglm
pkg_check_modules(CGLM REQUIRED cglm)
include_directories(${CGLM_INCLUDE_DIRS})
link_directories(${CGLM_LIBRARY_DIRS})
link_libraries(${CGLM_LIBRARIES})

# GLAD
# add_library(glad SHARED glad.c)
# target_include_directories(glad PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(${PROJECT_NAME} src/main.c src/vulkan.c src/window.c src/error.c ${FLEX_SCANNER_OUTPUTS})
set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD 23)
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan glfw m ${FLEX_LIBRARIES})
target_compile_definitions(${PROJECT_NAME} PUBLIC CGLM_DEFINE_PRINTS=1)
include_directories(${CMAKE_CURRENT_SOURCE_DIR/src})

add_custom_command(
  OUTPUT  vert.spv
  OUTPUT  frag.spv
  COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
  COMMAND Vulkan::glslc shader.vert -o "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders/vert.spv"
  COMMAND Vulkan::glslc shader.frag -o "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders/frag.spv"
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/shaders"
)

add_custom_target(Compile_Shaders DEPENDS vert.spv frag.spv)

configure_file(vk_layer_settings.txt   .                       COPYONLY)
configure_file(textures/texture.jpg    textures/texture.jpg    COPYONLY)
configure_file(models/cube.obj         models/cube.obj         COPYONLY)
configure_file(models/dodecahedron.obj models/dodecahedron.obj COPYONLY)
configure_file(models/rectangle.obj    models/rectangle.obj    COPYONLY)
configure_file(models/tetrahedron.obj  models/tetrahedron.obj  COPYONLY)
configure_file(models/teapot.obj       models/teapot.obj       COPYONLY)
configure_file(src/vk.h                vk.h                    COPYONLY)
