#include "vkTutorial.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan_core.h>

VkInstance instance;
VkDebugUtilsMessengerEXT debugMessenger;

// required validation layers
const char *requiredValidationLayers[] = {"VK_LAYER_KHRONOS_validation"};
int requiredValidationLayersCount = sizeof(requiredValidationLayers) / sizeof(char *);

extern VkResult err;

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

static const char **getRequiredExtensions(uint32_t *count) {
  // get required GLFW extensions
  const char **glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(count);

  // print GLFW extensions
  printf("GLFW extensions:\n");
  for (int i = 0; i < *count; i++) {
    printf("  %s\n", glfwExtensions[i]);
  }

  // setup required extensions
  const char **requiredExtensions = malloc(*count * sizeof(char *));

  // add debug extension as last element
  requiredExtensions[*count] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

  // copy GLFW extensions into required extensions
  for (int i = 0; i < *count; i++) {
    requiredExtensions[i] = glfwExtensions[i];
  }

  // update number of required extensions
  (*count)++;

  // print required extensions
  printf("\nRequired extensions:\n");
  for (int i = 0; i < *count; i++) {
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
  err = allLayersAvailable ? VK_SUCCESS : VKT_ERROR_VALIDATIONLAYER_NOT_PRESENT;
  handleError();

  printf("\nAll validation layers are present: true\n");
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
  printf("\nAll required extensions are present: true\n");

  free(requiredExtensions);
}

void initVulkan() {
  createInstance();
  checkValidationLayerSupport();
  setupDebugMessenger();
}
