#pragma once

#include <stdbool.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define VKT_ERROR_VALIDATIONLAYER_NOT_AVAILABLE -14
#define VKT_ERROR_NO_VULKAN_DEVICE_AVAILABLE -15
#define VKT_ERROR_NO_SUITABLE_GPU_AVAILABLE -16
#define VKT_ERROR_DEVICE_EXTENSIONS_NOT_AVAILABLE -17

void initWindow();
void initVulkan();
void mainloop();
void cleanup();
GLFWwindow *getWindow();
void handleError();
bool checkValidationLayerSupport();
void destroyDebugUtilsMessenger(VkInstance, VkDebugUtilsMessengerEXT, const VkAllocationCallbacks*);