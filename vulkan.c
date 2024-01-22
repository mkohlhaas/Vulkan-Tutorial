#include "vkTutorial.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <vulkan/vulkan_core.h>

extern VkResult err;
extern const char *requiredValidationLayers[];
extern int numRequiredValidationLayers;

VkInstance instance;

void createInstance() {

  VkApplicationInfo appInfo = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pApplicationName = "Vulkan Tutorial",
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "No Engine",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = VK_API_VERSION_1_0,
  };

  uint32_t glfwExtensionCount = 0;
  const char **glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  VkInstanceCreateInfo createInfo = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &appInfo,
      .enabledExtensionCount = glfwExtensionCount,
      .ppEnabledExtensionNames = glfwExtensions,
      .enabledLayerCount = numRequiredValidationLayers,
      .ppEnabledLayerNames = requiredValidationLayers,
  };
  printf("GLFW required extensions:\n");
  for (int i = 0; i < glfwExtensionCount; i++) {
    printf("  %s\n", glfwExtensions[i]);
  }

  err = vkCreateInstance(&createInfo, NULL, &instance);
  handleError();

  uint32_t propertyCount;
  err = vkEnumerateInstanceExtensionProperties(NULL, &propertyCount, NULL);
  handleError();
  VkExtensionProperties extensions[propertyCount];
  err = vkEnumerateInstanceExtensionProperties(NULL, &propertyCount, extensions);
  printf("\nAvailable extensions:\n");
  for (int i = 0; i < propertyCount; i++) {
    printf("  %s (%u)\n", extensions[i].extensionName, extensions[i].specVersion);
  }

  // check if all required extensions are available
  int found = 0;
  for (int i = 0; i < glfwExtensionCount; i++) {
    for (int j = 0; j < propertyCount; j++) {
      if (!strcmp(glfwExtensions[i], extensions[j].extensionName)) {
        found++;
        break;
      }
    }
  }
  err = found == glfwExtensionCount ? VK_SUCCESS : VK_ERROR_EXTENSION_NOT_PRESENT;
  handleError();
}

void initVulkan() {
  createInstance();
  checkValidationLayerSupport();
}
