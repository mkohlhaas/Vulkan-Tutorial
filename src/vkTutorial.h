#pragma once

#include <stdbool.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define VKT_ERROR_VALIDATIONLAYER_NOT_AVAILABLE -14
#define VKT_ERROR_NO_VULKAN_DEVICE_AVAILABLE -15
#define VKT_ERROR_NO_SUITABLE_GPU_AVAILABLE -16
#define VKT_ERROR_DEVICE_EXTENSIONS_NOT_AVAILABLE -17
#define VKT_ERROR_SWAP_CHAIN_NOT_ADEQUATE -18
#define VKT_ERROR_NO_SUITABLE_MEMORY_AVAILABLE -19
#define VKT_ERROR_NO_TEXTURE_FILE -20
#define VKT_ERROR_UNSUPPORTED_LAYOUT_TRANSITION -21
#define VKT_ERROR_FORMAT_NOT_AVAILABLE -22
#define VKT_ERROR_NO_VERT_SHADER -23
#define VKT_ERROR_NO_FRAG_SHADER -24
#define VKT_ERROR_NO_VALIDATION_LAYER -25

#define handleError(x) _handleError(__FILE__, __LINE__)

#ifdef NDEBUG
#define debugPrint(fmt, ...)
#else
#define debugPrint(fmt, ...)                                                                                                                         \
  do {                                                                                                                                               \
    fprintf(stderr, fmt, ##__VA_ARGS__);                                                                                                             \
  } while (0)
#endif

void initGLFW();
void initVulkan();
void mainloop();
void cleanupGLFW();
void cleanupVulkan();
void CreateSurface();
const char **getRequiredExtensions(uint32_t *);
GLFWwindow *getWindow();
void _handleError(const char *, int);
bool checkValidationLayerSupport();
void DestroyDebugUtilsMessenger(VkInstance, VkDebugUtilsMessengerEXT, const VkAllocationCallbacks *);
void drawFrame();
void DeviceWaitIdle();
void CopyBuffer(VkBuffer, VkBuffer, VkDeviceSize);
