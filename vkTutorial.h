#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

void initWindow();
void initVulkan();
void mainloop();
void cleanup();
GLFWwindow *getWindow();
void handleError();
