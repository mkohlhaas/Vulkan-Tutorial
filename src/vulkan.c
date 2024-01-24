#include "vkTutorial.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan_core.h>

extern GLFWwindow *window;

// Vulkan objects
VkInstance instance;
VkDebugUtilsMessengerEXT debugMessenger;
VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
VkDevice device;
VkQueue graphicsQueue;
VkSurfaceKHR surface;

VkSwapchainKHR swapChain;
VkImage *swapChainImages;
uint32_t swapchainImagesCount;
VkFormat swapchainImageFormat;
VkExtent2D swapchainExtent;

// required validation layers
const char *requiredValidationLayers[] = {"VK_LAYER_KHRONOS_validation"};
int requiredValidationLayersCount = sizeof(requiredValidationLayers) / sizeof(char *);

// required device extensions
const char *requiredDeviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
int requiredDeviceExtensionsCount = sizeof(requiredDeviceExtensions) / sizeof(char *);

extern VkResult err;

// Data structures
typedef struct QueueFamilyIndices {
  uint32_t graphicsFamily;
  bool isGraphicsFamilyPresent;
} QueueFamilyIndices;

static VKAPI_PTR VkBool32 debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData) {
  fprintf(stderr, "%s\n", pCallbackData->pMessage);
  return VK_FALSE;
}

static VkResult createDebugUtilsMessenger(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                          const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger) {
  PFN_vkCreateDebugUtilsMessengerEXT fn = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
  if (fn) {
    return fn(instance, pCreateInfo, pAllocator, pDebugMessenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

static void setupDebugMessenger() {
  VkDebugUtilsMessengerCreateInfoEXT createInfo = {
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
      .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
      .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
      .pfnUserCallback = debugCallback,
  };

  err = createDebugUtilsMessenger(instance, &createInfo, NULL, &debugMessenger);
  handleError();
}

void destroyDebugUtilsMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator) {
  PFN_vkDestroyDebugUtilsMessengerEXT fn = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
  if (fn) {
    fn(instance, debugMessenger, pAllocator);
  }
}

static const char **getRequiredExtensions(uint32_t *requiredExtensionsCount) {
  // get required GLFW extensions
  const char **glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(requiredExtensionsCount);

  // print GLFW extensions
  printf("GLFW extensions:\n");
  for (int i = 0; i < *requiredExtensionsCount; i++) {
    printf("  %s\n", glfwExtensions[i]);
  }

  // setup required extensions
  const char **requiredExtensions = malloc(*requiredExtensionsCount * sizeof(char *));

  // add debug extension as last element
  requiredExtensions[*requiredExtensionsCount] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

  // copy GLFW extensions into required extensions
  for (int i = 0; i < *requiredExtensionsCount; i++) {
    requiredExtensions[i] = glfwExtensions[i];
  }

  // update number of required extensions
  (*requiredExtensionsCount)++;

  // print required extensions
  printf("\nRequired extensions:\n");
  for (int i = 0; i < *requiredExtensionsCount; i++) {
    printf("  %s\n", requiredExtensions[i]);
  }

  return requiredExtensions;
}

bool checkValidationLayerSupport() {
  // available validation layers
  uint32_t layerCount;
  err = vkEnumerateInstanceLayerProperties(&layerCount, NULL);
  handleError();

  VkLayerProperties availableLayers[layerCount];
  err = vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);
  handleError();

  printf("\nAvailable layers:\n");
  for (int i = 0; i < layerCount; i++) {
    printf("  %s\n", availableLayers[i].layerName);
  }

  // Are all required validation layers available ?
  int found = 0;
  for (int i = 0; i < requiredValidationLayersCount; i++) {
    for (int j = 0; j < layerCount; j++) {
      if (!strcmp(requiredValidationLayers[i], availableLayers[j].layerName)) {
        found++;
        break;
      }
    }
  }
  bool allLayersAvailable = found == requiredValidationLayersCount;
  err = allLayersAvailable ? VK_SUCCESS : VKT_ERROR_VALIDATIONLAYER_NOT_AVAILABLE;
  handleError();

  printf("\nAll validation layers are available: true\n");
  return true;
}

void createInstance() {
  VkApplicationInfo appInfo = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pApplicationName = "Vulkan Tutorial",
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "No Engine",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = VK_API_VERSION_1_0,
  };

  uint32_t requiredExtensionsCount;
  const char **requiredExtensions = getRequiredExtensions(&requiredExtensionsCount);

  VkInstanceCreateInfo instanceInfo = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &appInfo,
      .enabledExtensionCount = requiredExtensionsCount,
      .ppEnabledExtensionNames = requiredExtensions,
      .enabledLayerCount = requiredValidationLayersCount,
      .ppEnabledLayerNames = requiredValidationLayers,
  };
  err = vkCreateInstance(&instanceInfo, NULL, &instance);
  handleError();

  uint32_t extensionCount;
  err = vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);
  handleError();
  VkExtensionProperties extensions[extensionCount];
  err = vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, extensions);

  // print available extensions
  printf("\nAvailable extensions:\n");
  for (int i = 0; i < extensionCount; i++) {
    printf("  %s\n", extensions[i].extensionName);
  }

  // check if all required extensions are available
  int found = 0;
  for (int i = 0; i < requiredExtensionsCount; i++) {
    for (int j = 0; j < extensionCount; j++) {
      if (!strcmp(requiredExtensions[i], extensions[j].extensionName)) {
        found++;
        break;
      }
    }
  }
  err = found == requiredExtensionsCount ? VK_SUCCESS : VK_ERROR_EXTENSION_NOT_PRESENT;
  handleError();
  printf("\nAll required extensions are available: true\n");

  free(requiredExtensions);
}

void createSwapChain() {
  fprintf(stderr, "  Swap Chain Support:\n");

  // surface capabilities
  fprintf(stderr, "    Surface Capabilities:\n");
  VkSurfaceCapabilitiesKHR surfaceCapabilities;
  err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);
  handleError();
  fprintf(
      stderr,
      "      Min image count: %u\n      Max image count: %u (0 means ∞)\n      Max image array layers: %u\n      Current width: %u\n      Current "
      "height: %u\n",
      surfaceCapabilities.minImageCount, surfaceCapabilities.maxImageCount, surfaceCapabilities.maxImageArrayLayers,
      surfaceCapabilities.currentExtent.width, surfaceCapabilities.currentExtent.height);
  fprintf(stderr, "      Max image width: %u\n      Max image height: %u\n", surfaceCapabilities.maxImageExtent.width,
          surfaceCapabilities.maxImageExtent.height);

  // surface formats
  uint32_t formatCount;
  err = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, NULL);
  handleError();
  VkSurfaceFormatKHR surfaceFormats[formatCount];
  err = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, surfaceFormats);
  handleError();
  fprintf(stderr, "    Surface formats: %u\n", formatCount);

  for (int i = 0; i < formatCount; i++) {
    if (surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      fprintf(stderr, "      Special surface format found (8Bit colors and nonlinear color space).\n");
    }
  }

  // present modes
  uint32_t presentModeCount;
  err = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, NULL);
  handleError();
  VkPresentModeKHR presentModes[presentModeCount];
  err = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes);
  handleError();
  fprintf(stderr, "    Surface present modes: %u\n", presentModeCount);

  for (int i = 0; i < presentModeCount; i++) {
    if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
      fprintf(stderr, "      Mailbox present mode found.\n");
    }
  }

  if (formatCount == 0 && presentModeCount == 0) {
    err = VKT_ERROR_SWAP_CHAIN_NOT_ADEQUATE;
    handleError();
  }

  // create swapchain
  swapchainImagesCount = surfaceCapabilities.minImageCount + 1;
  swapchainImageFormat = surfaceFormats[0].format;
  swapchainExtent = surfaceCapabilities.currentExtent;
  VkSwapchainCreateInfoKHR swapchainCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .surface = surface,
      .minImageCount = swapchainImagesCount,
      .imageFormat = swapchainImageFormat,
      .imageColorSpace = surfaceFormats[0].colorSpace,
      .imageExtent = swapchainExtent,
      .imageArrayLayers = 1,
      .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .preTransform = surfaceCapabilities.currentTransform,
      .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      .presentMode = VK_PRESENT_MODE_FIFO_KHR,
      .clipped = VK_TRUE,
      .oldSwapchain = VK_NULL_HANDLE,
  };

  err = vkCreateSwapchainKHR(device, &swapchainCreateInfo, NULL, &swapChain);
  handleError();

  // swapchain images
  err = vkGetSwapchainImagesKHR(device, swapChain, &swapchainImagesCount, NULL);
  handleError();
  swapChainImages = malloc(swapchainImagesCount * sizeof(VkImage));
  err = vkGetSwapchainImagesKHR(device, swapChain, &swapchainImagesCount, swapChainImages);
  handleError();
}

bool isPhysicalDeviceSuitable(VkPhysicalDevice device) {
  // get device properties and features
  VkPhysicalDeviceProperties deviceProperties;
  vkGetPhysicalDeviceProperties(device, &deviceProperties);
  VkPhysicalDeviceFeatures deviceFeatures;
  vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

  // print physical device info
  fprintf(stderr, "\nPhysical Device:\n  Device ID: %u\n  Device Name: %s\n", deviceProperties.deviceID, deviceProperties.deviceName);

  // get device extensions
  uint32_t extensionCount;
  vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, NULL);
  VkExtensionProperties availableDeviceExtensions[extensionCount];
  vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, availableDeviceExtensions);

  // print required device extensions
  fprintf(stderr, "  Required extensions:\n");
  for (int i = 0; i < requiredDeviceExtensionsCount; i++) {
    fprintf(stderr, "    %s\n", requiredDeviceExtensions[i]);
  }

  // print available device extensions
  fprintf(stderr, "  Available extensions:\n");
  for (int i = 0; i < extensionCount; i++) {
    fprintf(stderr, "    %s\n", availableDeviceExtensions[i].extensionName);
  }

  // check if required extensions are available
  int found = 0;
  for (int i = 0; i < requiredDeviceExtensionsCount; i++) {
    for (int j = 0; j < extensionCount; j++) {
      if (!strcmp(requiredDeviceExtensions[i], availableDeviceExtensions[j].extensionName)) {
        found++;
        break;
      }
    }
  }
  bool allExtensionsAvailable = found == requiredDeviceExtensionsCount;
  err = allExtensionsAvailable ? VK_SUCCESS : VKT_ERROR_DEVICE_EXTENSIONS_NOT_AVAILABLE;
  handleError();
  printf("  All required device extensions are available: true\n");

  return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU || deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
  //  && deviceFeatures.geometryShader;
}

int firstGraphicsQueueFamilyIndex() {
  int result = -1;
  uint32_t queueFamilyCount;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);

  VkQueueFamilyProperties queueFamilies[queueFamilyCount];
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies);

  VkBool32 surfaceSupport = VK_FALSE;
  for (int i = 0; i < queueFamilyCount; i++) {
    if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &surfaceSupport);
      if (surfaceSupport) {
        fprintf(stderr, "\nFirst graphics queue family index: %d\n", i);
        result = i;
        break;
      }
    }
  }

  if (result == -1) {
    err = VKT_ERROR_NO_SUITABLE_GPU_AVAILABLE;
    handleError();
  }
  return result;
}

QueueFamilyIndices printQueueFamilies() {
  QueueFamilyIndices indices;

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);

  VkQueueFamilyProperties queueFamilies[queueFamilyCount];
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies);
  fprintf(stderr, "  Number of queue families: %u\n", queueFamilyCount);

  for (int i = 0; i < queueFamilyCount; i++) {
    fprintf(stderr, "    %d. Family: %2u queues → ", i + 1, queueFamilies[i].queueCount);
    if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      fprintf(stderr, "Graphics ");
    }
    if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
      fprintf(stderr, "Compute ");
    }
    if (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
      fprintf(stderr, "Transfer ");
    }
    if (queueFamilies[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) {
      fprintf(stderr, "SparseBinding ");
    }
    if (queueFamilies[i].queueFlags & VK_QUEUE_PROTECTED_BIT) {
      fprintf(stderr, "Protected ");
    }
    if (queueFamilies[i].queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR) {
      fprintf(stderr, "VideoDecode ");
    }
    if (queueFamilies[i].queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR) {
      fprintf(stderr, "VideoEncode ");
    }
    if (queueFamilies[i].queueFlags & VK_QUEUE_OPTICAL_FLOW_BIT_NV) {
      fprintf(stderr, "OpticalFlow ");
    }
    if (queueFamilies[i].queueFlags & VK_QUEUE_FLAG_BITS_MAX_ENUM) {
      fprintf(stderr, "FlagBits ");
    }
    fprintf(stderr, "\n");
  }

  return indices;
}

void pickPhysicalDevice() {
  // get count of phsysical devices
  uint32_t physicalDeviceCount;
  vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, NULL);

  // bail out if no physical device available
  if (!physicalDeviceCount) {
    err = VKT_ERROR_NO_VULKAN_DEVICE_AVAILABLE;
    handleError();
  }

  // get all physical devices
  VkPhysicalDevice physicalDevices[physicalDeviceCount];
  vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices);

  // choose the first suitable GPU device
  for (int i = 0; i < physicalDeviceCount; i++) {
    if (isPhysicalDeviceSuitable(physicalDevices[i])) {
      physicalDevice = physicalDevices[i];
      break;
    }
  }

  // bail out if no suitable physical device available
  if (!physicalDevice) {
    err = VKT_ERROR_NO_SUITABLE_GPU_AVAILABLE;
    handleError();
  }
}

void createLogicalDevice() {
  int queueFamilyIndex = firstGraphicsQueueFamilyIndex();
  float queuePriority = 1.0f;
  VkDeviceQueueCreateInfo deviceQueueCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = queueFamilyIndex,
      .queueCount = 1,
      .pQueuePriorities = &queuePriority,
  };

  VkDeviceCreateInfo deviceCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .queueCreateInfoCount = 1,
      .pQueueCreateInfos = &deviceQueueCreateInfo,
      .enabledExtensionCount = requiredDeviceExtensionsCount,
      .ppEnabledExtensionNames = requiredDeviceExtensions,
  };

  err = vkCreateDevice(physicalDevice, &deviceCreateInfo, NULL, &device);
  handleError();
  vkGetDeviceQueue(device, queueFamilyIndex, 0, &graphicsQueue);
}

void createSurface() {
  err = glfwCreateWindowSurface(instance, window, NULL, &surface);
  handleError();
}

void cleanup() {
  vkDestroySwapchainKHR(device, swapChain, NULL);
  vkDestroyDevice(device, NULL);
  destroyDebugUtilsMessenger(instance, debugMessenger, NULL);
  vkDestroySurfaceKHR(instance, surface, NULL);
  vkDestroyInstance(instance, NULL);
  glfwDestroyWindow(window);
  glfwTerminate();
}

void initVulkan() {
  createInstance();
  checkValidationLayerSupport();
  setupDebugMessenger();
  createSurface();
  pickPhysicalDevice();
  printQueueFamilies();
  createLogicalDevice();
  createSwapChain();
}
