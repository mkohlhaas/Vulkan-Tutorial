#pragma once

#include <stdbool.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define VKT_ERROR_VALIDATIONLAYER_NOT_PRESENT -14

void initWindow();
void initVulkan();
void mainloop();
void cleanup();
GLFWwindow *getWindow();
void handleError();
bool checkValidationLayerSupport();
void destroyDebugUtilsMessenger(VkInstance, VkDebugUtilsMessengerEXT, const VkAllocationCallbacks*);
