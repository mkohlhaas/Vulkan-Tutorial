#include "vkTutorial.h"
#include <glib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan_core.h>

const int MAX_FRAMES_IN_FLIGHT = 2;

extern GLFWwindow *window;

// Vulkan objects
VkInstance instance;
VkDebugUtilsMessengerEXT debugMessenger;
VkSurfaceKHR surface;

VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

VkQueue graphicsQueue;
VkDevice device;

VkSwapchainKHR swapChain;
VkImage *swapChainImages;
uint32_t swapChainImagesCount;
VkFormat swapChainImageFormat;
VkExtent2D swapChainExtent;
VkImageView *swapChainImageViews;

VkRenderPass renderPass;
VkPipelineLayout pipelineLayout;
VkPipeline graphicsPipeline;

VkFramebuffer *swapChainFramebuffers;
VkCommandPool commandPool;
VkCommandBuffer *commandBuffers;

VkSemaphore *imageAvailableSemaphores;
VkSemaphore *finishedRenderingSemaphores;
VkFence *inFlightFences;

uint32_t currentFrame = 0;

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

VKAPI_PTR VkBool32 debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                 const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData) {
  fprintf(stderr, "Validation Layer: %s (%s)\n", pCallbackData->pMessage,
          messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT   ? "Severity: Verbose"
          : messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT    ? "Severity: Info"
          : messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT ? "Severity: Warning"
          : messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT   ? "Severity: Error"
                                                                               : "Severity: Unknown");
  return VK_FALSE;
}

VkResult createDebugMessenger(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator,
                              VkDebugUtilsMessengerEXT *pDebugMessenger) {
  PFN_vkCreateDebugUtilsMessengerEXT fn = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
  if (fn) {
    return fn(instance, pCreateInfo, pAllocator, pDebugMessenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void setupDebugMessenger() {
  VkDebugUtilsMessengerCreateInfoEXT createInfo = {
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
      .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
      .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
      .pfnUserCallback = debugCallback,
  };

  err = createDebugMessenger(instance, &createInfo, NULL, &debugMessenger);
  handleError();
}

void destroyDebugUtilsMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator) {
  PFN_vkDestroyDebugUtilsMessengerEXT fn = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
  if (fn) {
    fn(instance, debugMessenger, pAllocator);
  }
}

const char **getRequiredExtensions(uint32_t *requiredExtensionsCount) {
  // get required GLFW extensions
  const char **glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(requiredExtensionsCount);

  // print GLFW extensions
  printf("GLFW:\n");
  printf("  Extensions:\n");
  for (int i = 0; i < *requiredExtensionsCount; i++) {
    printf("    %s\n", glfwExtensions[i]);
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

  printf("  Available layers:\n");
  for (int i = 0; i < layerCount; i++) {
    printf("    %s (%s)\n", availableLayers[i].layerName, availableLayers[i].description);
  }

  // print required layers
  printf("  Required layers:\n");
  for (int i = 0; i < requiredValidationLayersCount; i++) {
    printf("    %s\n", requiredValidationLayers[i]);
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

  printf("  All required validation layers are available: true\n");
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

  fprintf(stderr, "\nVulkan instance:\n");
  // print available extensions
  printf("  Available extensions:\n");
  for (int i = 0; i < extensionCount; i++) {
    printf("    %s\n", extensions[i].extensionName);
  }

  // print required extensions
  printf("  Required extensions:\n");
  for (int i = 0; i < requiredExtensionsCount; i++) {
    printf("    %s\n", requiredExtensions[i]);
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
  printf("  All required extensions are available: true\n");

  free(requiredExtensions);
}

void createSwapChain() {
  fprintf(stderr, "  Swap Chain Support:\n");

  // surface capabilities
  // clang-format off
  fprintf(stderr, "    Surface Capabilities:\n");
  VkSurfaceCapabilitiesKHR surfaceCapabilities;
  err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);
  handleError();

  // basic cababilities
  fprintf(stderr, "      Min image count: %u\n", surfaceCapabilities.minImageCount);
  fprintf(stderr, "      Max image count: %u (0 means ∞)\n", surfaceCapabilities.maxImageCount);
  fprintf(stderr, "      Current Extent:\n");
  fprintf(stderr, "        width: %u\n", surfaceCapabilities.currentExtent.width);
  fprintf(stderr, "        height: %u\n", surfaceCapabilities.currentExtent.height);
  fprintf(stderr, "      Min Image Extent:\n");
  fprintf(stderr, "        width: %u\n", surfaceCapabilities.minImageExtent.width);
  fprintf(stderr, "        height: %u\n", surfaceCapabilities.minImageExtent.height);
  fprintf(stderr, "      Max Image Extent:\n");
  fprintf(stderr, "        width: %u\n", surfaceCapabilities.maxImageExtent.width);
  fprintf(stderr, "        height: %u\n", surfaceCapabilities.maxImageExtent.height);
  fprintf(stderr, "      Max image array layers: %u\n", surfaceCapabilities.maxImageArrayLayers);
  fprintf(stderr, "      Supported Transforms:\n");
  fprintf(stderr, "        identity: %s\n", surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR ? "true" : "false");
  fprintf(stderr, "        rotate 90°: %s\n", surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR ? "true" : "false");
  fprintf(stderr, "        rotate 180°: %s\n", surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR ? "true" : "false");
  fprintf(stderr, "        rotate 270°: %s\n", surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR ? "true" : "false");
  fprintf(stderr, "        horizontal mirror: %s\n", surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR ? "true" : "false");
  fprintf(stderr, "        horizontal mirror 90°: %s\n", surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR ? "true" : "false");
  fprintf(stderr, "        horizontal mirror 180°: %s\n", surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR ? "true" : "false");
  fprintf(stderr, "        horizontal mirror 270°: %s\n", surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR ? "true" : "false");
  fprintf(stderr, "        inherit: %s\n", surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR ? "true" : "false");
  // clang-format on

  // current transform
  fprintf(stderr, "      Current Transform:\n");
  if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
    fprintf(stderr, "        identity\n");
  } else if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR) {
    fprintf(stderr, "        rotate 90°\n");
  } else if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR) {
    fprintf(stderr, "        rotate 180°\n");
  } else if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR) {
    fprintf(stderr, "        rotate 270°\n");
  } else if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR) {
    fprintf(stderr, "        horizontal mirror\n");
  } else if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR) {
    fprintf(stderr, "        horizontal mirror 90°\n");
  } else if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR) {
    fprintf(stderr, "        horizontal mirror 180°\n");
  } else if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR) {
    fprintf(stderr, "        horizontal mirror 270°\n");
  } else if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR) {
    fprintf(stderr, "        inherit\n");
  }

  // composite alpha
  fprintf(stderr, "      Composite Alpha:\n");
  if (surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) {
    fprintf(stderr, "        opaque\n");
  }
  if (surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR) {
    fprintf(stderr, "        pre multiplied\n");
  }
  if (surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR) {
    fprintf(stderr, "        post multiplied\n");
  }
  if (surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) {
    fprintf(stderr, "        inherit\n");
  }

  // image usage
  fprintf(stderr, "      Image Usage:\n");
  if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
    fprintf(stderr, "        transfer src\n");
  }
  if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
    fprintf(stderr, "        transfer dst\n");
  }
  if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_SAMPLED_BIT) {
    fprintf(stderr, "        sampled\n");
  }
  if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_STORAGE_BIT) {
    fprintf(stderr, "        storage\n");
  }
  if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
    fprintf(stderr, "        color attachment\n");
  }
  if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
    fprintf(stderr, "        depth stencil attachment\n");
  }
  if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) {
    fprintf(stderr, "        transient attachment\n");
  }
  if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT) {
    fprintf(stderr, "        input attachment\n");
  }
  if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR) {
    fprintf(stderr, "        video decode dst\n");
  }
  if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_VIDEO_DECODE_SRC_BIT_KHR) {
    fprintf(stderr, "        video decode src\n");
  }
  if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR) {
    fprintf(stderr, "        video decode dpb\n");
  }
  if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT) {
    fprintf(stderr, "        fragment density map\n");
  }
  if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR) {
    fprintf(stderr, "        fragment shading rate attachment\n");
  }
  if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_HOST_TRANSFER_BIT_EXT) {
    fprintf(stderr, "        host transfer\n");
  }
  if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_VIDEO_ENCODE_DST_BIT_KHR) {
    fprintf(stderr, "        video encode dst\n");
  }
  if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_VIDEO_ENCODE_SRC_BIT_KHR) {
    fprintf(stderr, "        video encode src\n");
  }
  if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_VIDEO_ENCODE_DPB_BIT_KHR) {
    fprintf(stderr, "        video encode dpb\n");
  }
  if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_ATTACHMENT_FEEDBACK_LOOP_BIT_EXT) {
    fprintf(stderr, "        attachment feedback loop\n");
  }
  if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_INVOCATION_MASK_BIT_HUAWEI) {
    fprintf(stderr, "        invocation mask\n");
  }
  if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_SAMPLE_WEIGHT_BIT_QCOM) {
    fprintf(stderr, "        sample weight\n");
  }
  if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_SAMPLE_BLOCK_MATCH_BIT_QCOM) {
    fprintf(stderr, "        sample block match\n");
  }
  if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_SHADING_RATE_IMAGE_BIT_NV) {
    fprintf(stderr, "        shading rate image\n");
  }

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

  // presentation modes
  uint32_t presentModeCount;
  err = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, NULL);
  handleError();
  VkPresentModeKHR presentModes[presentModeCount];
  err = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes);
  handleError();
  fprintf(stderr, "    Surface presentation modes:\n");
  for (int i = 0; i < presentModeCount; i++) {
    if (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
      fprintf(stderr, "      immediate\n");
    } else if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
      fprintf(stderr, "      mailbox\n");
    } else if (presentModes[i] == VK_PRESENT_MODE_FIFO_KHR) {
      fprintf(stderr, "      fifo\n");
    } else if (presentModes[i] == VK_PRESENT_MODE_FIFO_RELAXED_KHR) {
      fprintf(stderr, "      fifo relaxed\n");
    } else if (presentModes[i] == VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR) {
      fprintf(stderr, "      shared demand refresh\n");
    } else if (presentModes[i] == VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR) {
      fprintf(stderr, "      shared continuous refresh\n");
    }
  }

  // For us it's sufficient to have an surface format and a presentation mode.
  if (formatCount == 0 && presentModeCount == 0) {
    err = VKT_ERROR_SWAP_CHAIN_NOT_ADEQUATE;
    handleError();
  }

  // create swapchain
  swapChainImagesCount = surfaceCapabilities.minImageCount + 1;
  swapChainImageFormat = surfaceFormats[0].format;
  swapChainExtent = surfaceCapabilities.currentExtent;
  VkSwapchainCreateInfoKHR swapchainCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .surface = surface,
      .minImageCount = swapChainImagesCount,
      .imageFormat = swapChainImageFormat,
      .imageColorSpace = surfaceFormats[0].colorSpace,
      .imageExtent = swapChainExtent,
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

  // retrieve swapchain images
  err = vkGetSwapchainImagesKHR(device, swapChain, &swapChainImagesCount, NULL);
  handleError();
  swapChainImages = malloc(swapChainImagesCount * sizeof(VkImage));
  err = vkGetSwapchainImagesKHR(device, swapChain, &swapChainImagesCount, swapChainImages);
  handleError();
}

bool isPhysicalDeviceSuitable(VkPhysicalDevice device) {
  // get device properties and features
  VkPhysicalDeviceProperties deviceProperties;
  vkGetPhysicalDeviceProperties(device, &deviceProperties);
  VkPhysicalDeviceFeatures deviceFeatures;
  vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

  // print physical device info
  // clang-format off
  fprintf(stderr, "\nPhysical Device:\n");
  uint32_t  vId = deviceProperties.vendorID;
  fprintf(stderr, "  Vendor: %s\n",  vId == 0x1002 ? "AMD" : vId == 0x1010 ? "ImgTec" : vId == 0x10DE ? "Nvidia" : vId == 0x13B5 ? "ARM" : vId == 0x5143 ? "Qualcomm" : vId == 0x8086 ? "Intel" : "Unknown");
  fprintf(stderr, "  Device Name: %s\n",  deviceProperties.deviceName);
  fprintf(stderr, "  Limits:\n");
  fprintf(stderr, "    maxImageDimension1D: %u\n", deviceProperties.limits.maxImageDimension1D);
  fprintf(stderr, "    maxImageDimension2D: %u\n", deviceProperties.limits.maxImageDimension2D);
  fprintf(stderr, "    maxImageDimension3D: %u\n", deviceProperties.limits.maxImageDimension3D);
  fprintf(stderr, "    maxImageDimensionCube: %u\n", deviceProperties.limits.maxImageDimensionCube);
  fprintf(stderr, "    maxImageArrayLayers: %u\n", deviceProperties.limits.maxImageArrayLayers);
  fprintf(stderr, "    maxTexelBufferElements: %u\n", deviceProperties.limits.maxTexelBufferElements);
  fprintf(stderr, "    maxUniformBufferRange: %u\n", deviceProperties.limits.maxUniformBufferRange);
  fprintf(stderr, "    maxStorageBufferRange: %u\n", deviceProperties.limits.maxStorageBufferRange);
  fprintf(stderr, "    maxPushConstantsSize: %u\n", deviceProperties.limits.maxPushConstantsSize);
  fprintf(stderr, "    maxMemoryAllocationCount: %u\n", deviceProperties.limits.maxMemoryAllocationCount);
  fprintf(stderr, "    maxSamplerAllocationCount: %u\n", deviceProperties.limits.maxSamplerAllocationCount);
  fprintf(stderr, "    bufferImageGranularity: %lu\n", deviceProperties.limits.bufferImageGranularity);
  fprintf(stderr, "    sparseAddressSpaceSize: %lu\n", deviceProperties.limits.sparseAddressSpaceSize);
  fprintf(stderr, "    maxBoundDescriptorSets: %u\n", deviceProperties.limits.maxBoundDescriptorSets);
  fprintf(stderr, "    maxPerStageDescriptorSamplers: %u\n", deviceProperties.limits.maxPerStageDescriptorSamplers);
  fprintf(stderr, "    maxPerStageDescriptorUniformBuffers: %u\n", deviceProperties.limits.maxPerStageDescriptorUniformBuffers);
  fprintf(stderr, "    maxPerStageDescriptorStorageBuffers: %u\n", deviceProperties.limits.maxPerStageDescriptorStorageBuffers);
  fprintf(stderr, "    maxPerStageDescriptorSampledImages: %u\n", deviceProperties.limits.maxPerStageDescriptorSampledImages);
  fprintf(stderr, "    maxPerStageDescriptorStorageImages: %u\n", deviceProperties.limits.maxPerStageDescriptorStorageImages);
  fprintf(stderr, "    maxPerStageDescriptorInputAttachments: %u\n", deviceProperties.limits.maxPerStageDescriptorInputAttachments);
  fprintf(stderr, "    maxPerStageResources: %u\n", deviceProperties.limits.maxPerStageResources);
  fprintf(stderr, "    maxDescriptorSetSamplers: %u\n", deviceProperties.limits.maxDescriptorSetSamplers);
  fprintf(stderr, "    maxDescriptorSetUniformBuffers: %u\n", deviceProperties.limits.maxDescriptorSetUniformBuffers);
  fprintf(stderr, "    maxDescriptorSetUniformBuffersDynamic: %u\n", deviceProperties.limits.maxDescriptorSetUniformBuffersDynamic);
  fprintf(stderr, "    maxDescriptorSetStorageBuffers: %u\n", deviceProperties.limits.maxDescriptorSetStorageBuffers);
  fprintf(stderr, "    maxDescriptorSetStorageBuffersDynamic: %u\n", deviceProperties.limits.maxDescriptorSetStorageBuffersDynamic);
  fprintf(stderr, "    maxDescriptorSetSampledImages: %u\n", deviceProperties.limits.maxDescriptorSetSampledImages);
  fprintf(stderr, "    maxDescriptorSetStorageImages: %u\n", deviceProperties.limits.maxDescriptorSetStorageImages);
  fprintf(stderr, "    maxDescriptorSetInputAttachments: %u\n", deviceProperties.limits.maxDescriptorSetInputAttachments);
  fprintf(stderr, "    maxVertexInputAttributes: %u\n", deviceProperties.limits.maxVertexInputAttributes);
  fprintf(stderr, "    maxVertexInputBindings: %u\n", deviceProperties.limits.maxVertexInputBindings);
  fprintf(stderr, "    maxVertexInputAttributeOffset: %u\n", deviceProperties.limits.maxVertexInputAttributeOffset);
  fprintf(stderr, "    maxVertexInputBindingStride: %u\n", deviceProperties.limits.maxVertexInputBindingStride);
  fprintf(stderr, "    maxVertexOutputComponents: %u\n", deviceProperties.limits.maxVertexOutputComponents);
  fprintf(stderr, "    maxTessellationGenerationLevel: %u\n", deviceProperties.limits.maxTessellationGenerationLevel);
  fprintf(stderr, "    maxTessellationPatchSize: %u\n", deviceProperties.limits.maxTessellationPatchSize);
  fprintf(stderr, "    maxTessellationControlPerVertexInputComponents: %u\n", deviceProperties.limits.maxTessellationControlPerVertexInputComponents);
  fprintf(stderr, "    maxTessellationControlPerVertexOutputComponents: %u\n", deviceProperties.limits.maxTessellationControlPerVertexOutputComponents);
  fprintf(stderr, "    maxTessellationControlPerPatchOutputComponents: %u\n", deviceProperties.limits.maxTessellationControlPerPatchOutputComponents);
  fprintf(stderr, "    maxTessellationControlTotalOutputComponents: %u\n", deviceProperties.limits.maxTessellationControlTotalOutputComponents);
  fprintf(stderr, "    maxTessellationEvaluationInputComponents: %u\n", deviceProperties.limits.maxTessellationEvaluationInputComponents);
  fprintf(stderr, "    maxTessellationEvaluationOutputComponents: %u\n", deviceProperties.limits.maxTessellationEvaluationOutputComponents);
  fprintf(stderr, "    maxGeometryShaderInvocations: %u\n", deviceProperties.limits.maxGeometryShaderInvocations);
  fprintf(stderr, "    maxGeometryInputComponents: %u\n", deviceProperties.limits.maxGeometryInputComponents);
  fprintf(stderr, "    maxGeometryOutputComponents: %u\n", deviceProperties.limits.maxGeometryOutputComponents);
  fprintf(stderr, "    maxGeometryOutputVertices: %u\n", deviceProperties.limits.maxGeometryOutputVertices);
  fprintf(stderr, "    maxGeometryTotalOutputComponents: %u\n", deviceProperties.limits.maxGeometryTotalOutputComponents);
  fprintf(stderr, "    maxFragmentInputComponents: %u\n", deviceProperties.limits.maxFragmentInputComponents);
  fprintf(stderr, "    maxFragmentOutputAttachments: %u\n", deviceProperties.limits.maxFragmentOutputAttachments);
  fprintf(stderr, "    maxFragmentDualSrcAttachments: %u\n", deviceProperties.limits.maxFragmentDualSrcAttachments);
  fprintf(stderr, "    maxFragmentCombinedOutputResources: %u\n", deviceProperties.limits.maxFragmentCombinedOutputResources);
  fprintf(stderr, "    maxComputeSharedMemorySize: %u\n", deviceProperties.limits.maxComputeSharedMemorySize);
  fprintf(stderr, "    maxComputeWorkGroupCount(: %u, %u, %u)\n", deviceProperties.limits.maxComputeWorkGroupCount[0], deviceProperties.limits.maxComputeWorkGroupCount[1], deviceProperties.limits.maxComputeWorkGroupCount[2]);
  fprintf(stderr, "    maxComputeWorkGroupInvocationsmaxComputeWorkGroupInvocations: %u\n", deviceProperties.limits.maxComputeWorkGroupInvocations);
  fprintf(stderr, "    maxComputeWorkGroupSize(: %u, %u, %u)\n", deviceProperties.limits.maxComputeWorkGroupSize[0], deviceProperties.limits.maxComputeWorkGroupSize[1], deviceProperties.limits.maxComputeWorkGroupSize[2]);
  fprintf(stderr, "    subPixelPrecisionBits: %u\n", deviceProperties.limits.subPixelPrecisionBits);
  fprintf(stderr, "    subTexelPrecisionBits: %u\n", deviceProperties.limits.subTexelPrecisionBits);
  fprintf(stderr, "    mipmapPrecisionBits: %u\n", deviceProperties.limits.mipmapPrecisionBits);
  fprintf(stderr, "    maxDrawIndexedIndexValue: %u\n", deviceProperties.limits.maxDrawIndexedIndexValue);
  fprintf(stderr, "    maxDrawIndirectCount: %u\n", deviceProperties.limits.maxDrawIndirectCount);
  fprintf(stderr, "    maxSamplerLodBias: %f\n", deviceProperties.limits.maxSamplerLodBias);
  fprintf(stderr, "    maxSamplerAnisotropy: %f\n", deviceProperties.limits.maxSamplerAnisotropy);
  fprintf(stderr, "    maxViewports: %u\n", deviceProperties.limits.maxViewports);
  fprintf(stderr, "    maxViewportDimensions(: %u, %u)\n", deviceProperties.limits.maxViewportDimensions[0], deviceProperties.limits.maxViewportDimensions[1]);
  fprintf(stderr, "    viewportBoundsRange(: %f, %f)\n", deviceProperties.limits.viewportBoundsRange[0], deviceProperties.limits.viewportBoundsRange[1]);
  fprintf(stderr, "    viewportSubPixelBits: %u\n", deviceProperties.limits.viewportSubPixelBits);
  fprintf(stderr, "    minMemoryMapAlignment: %zu\n", deviceProperties.limits.minMemoryMapAlignment);
  fprintf(stderr, "    minTexelBufferOffsetAlignment: %lu\n", deviceProperties.limits.minTexelBufferOffsetAlignment);
  fprintf(stderr, "    minUniformBufferOffsetAlignment: %lu\n", deviceProperties.limits.minUniformBufferOffsetAlignment);
  fprintf(stderr, "    minStorageBufferOffsetAlignment: %lu\n", deviceProperties.limits.minStorageBufferOffsetAlignment);
  fprintf(stderr, "    minTexelOffset: %d\n", deviceProperties.limits.minTexelOffset);
  fprintf(stderr, "    maxTexelOffset: %u\n", deviceProperties.limits.maxTexelOffset);
  fprintf(stderr, "    minTexelGatherOffset: %d\n", deviceProperties.limits.minTexelGatherOffset);
  fprintf(stderr, "    maxTexelGatherOffset: %u\n", deviceProperties.limits.maxTexelGatherOffset);
  fprintf(stderr, "    minInterpolationOffset: %f\n", deviceProperties.limits.minInterpolationOffset);
  fprintf(stderr, "    maxInterpolationOffset: %f\n", deviceProperties.limits.maxInterpolationOffset);
  fprintf(stderr, "    subPixelInterpolationOffsetBits: %u\n", deviceProperties.limits.subPixelInterpolationOffsetBits);
  fprintf(stderr, "    maxFramebufferWidth: %u\n", deviceProperties.limits.maxFramebufferWidth);
  fprintf(stderr, "    maxFramebufferHeight: %u\n", deviceProperties.limits.maxFramebufferHeight);
  fprintf(stderr, "    maxFramebufferLayers: %u\n", deviceProperties.limits.maxFramebufferLayers);
  fprintf(stderr, "    framebufferColorSampleCounts: %u\n", deviceProperties.limits.framebufferColorSampleCounts);
  fprintf(stderr, "    framebufferDepthSampleCounts: %u\n", deviceProperties.limits.framebufferDepthSampleCounts);
  fprintf(stderr, "    framebufferStencilSampleCounts: %u\n", deviceProperties.limits.framebufferStencilSampleCounts);
  fprintf(stderr, "    framebufferNoAttachmentsSampleCounts: %u\n", deviceProperties.limits.framebufferNoAttachmentsSampleCounts);
  fprintf(stderr, "    maxColorAttachments: %u\n", deviceProperties.limits.maxColorAttachments);
  fprintf(stderr, "    sampledImageColorSampleCounts: %u\n", deviceProperties.limits.sampledImageColorSampleCounts);
  fprintf(stderr, "    sampledImageIntegerSampleCounts: %u\n", deviceProperties.limits.sampledImageIntegerSampleCounts);
  fprintf(stderr, "    sampledImageDepthSampleCounts: %u\n", deviceProperties.limits.sampledImageDepthSampleCounts);
  fprintf(stderr, "    sampledImageStencilSampleCounts: %u\n", deviceProperties.limits.sampledImageStencilSampleCounts);
  fprintf(stderr, "    storageImageSampleCounts: %u\n", deviceProperties.limits.storageImageSampleCounts);
  fprintf(stderr, "    maxSampleMaskWords: %u\n", deviceProperties.limits.maxSampleMaskWords);
  fprintf(stderr, "    timestampComputeAndGraphics: %s\n", deviceProperties.limits.timestampComputeAndGraphics ? "true" : "false");
  fprintf(stderr, "    timestampPeriod: %f\n", deviceProperties.limits.timestampPeriod);
  fprintf(stderr, "    maxClipDistances: %u\n", deviceProperties.limits.maxClipDistances);
  fprintf(stderr, "    maxCullDistances: %u\n", deviceProperties.limits.maxCullDistances);
  fprintf(stderr, "    maxCombinedClipAndCullDistances: %u\n", deviceProperties.limits.maxCombinedClipAndCullDistances);
  fprintf(stderr, "    discreteQueuePriorities: %u\n", deviceProperties.limits.discreteQueuePriorities);
  fprintf(stderr, "    pointSizeRange(: %f, %f)\n", deviceProperties.limits.pointSizeRange[0], deviceProperties.limits.pointSizeRange[1]);
  fprintf(stderr, "    lineWidthRange(: %f, %f)\n", deviceProperties.limits.lineWidthRange[0], deviceProperties.limits.lineWidthRange[1]);
  fprintf(stderr, "    pointSizeGranularity: %f\n", deviceProperties.limits.pointSizeGranularity);
  fprintf(stderr, "    lineWidthGranularity: %f\n", deviceProperties.limits.lineWidthGranularity);
  fprintf(stderr, "    strictLines: %s\n", deviceProperties.limits.strictLines? "true" : "false");
  fprintf(stderr, "    standardSampleLocations: %s\n", deviceProperties.limits.standardSampleLocations? "true" : "false");
  fprintf(stderr, "    optimalBufferCopyOffsetAlignment: %lu\n", deviceProperties.limits.optimalBufferCopyOffsetAlignment);
  fprintf(stderr, "    optimalBufferCopyRowPitchAlignment: %lu\n", deviceProperties.limits.optimalBufferCopyRowPitchAlignment);
  fprintf(stderr, "    nonCoherentAtomSize: %lu\n", deviceProperties.limits.nonCoherentAtomSize);
  fprintf(stderr, "  Features:\n");
  fprintf(stderr, "    robustBufferAccess: %s\n", deviceFeatures.robustBufferAccess ? "true" : "false");
  fprintf(stderr, "    fullDrawIndexUint32: %s\n", deviceFeatures.fullDrawIndexUint32 ? "true" : "false");
  fprintf(stderr, "    imageCubeArray: %s\n", deviceFeatures.imageCubeArray ? "true" : "false");
  fprintf(stderr, "    independentBlend: %s\n", deviceFeatures.independentBlend ? "true" : "false");
  fprintf(stderr, "    geometryShader: %s\n", deviceFeatures.geometryShader ? "true" : "false");
  fprintf(stderr, "    tessellationShader: %s\n", deviceFeatures.tessellationShader ? "true" : "false");
  fprintf(stderr, "    sampleRateShading: %s\n", deviceFeatures.sampleRateShading ? "true" : "false");
  fprintf(stderr, "    dualSrcBlend: %s\n", deviceFeatures.dualSrcBlend ? "true" : "false");
  fprintf(stderr, "    logicOp: %s\n", deviceFeatures.logicOp ? "true" : "false");
  fprintf(stderr, "    multiDrawIndirect: %s\n", deviceFeatures.multiDrawIndirect ? "true" : "false");
  fprintf(stderr, "    drawIndirectFirstInstance: %s\n", deviceFeatures.drawIndirectFirstInstance ? "true" : "false");
  fprintf(stderr, "    depthClamp: %s\n", deviceFeatures.depthClamp ? "true" : "false");
  fprintf(stderr, "    depthBiasClamp: %s\n", deviceFeatures.depthBiasClamp ? "true" : "false");
  fprintf(stderr, "    fillModeNonSolid: %s\n", deviceFeatures.fillModeNonSolid ? "true" : "false");
  fprintf(stderr, "    depthBounds: %s\n", deviceFeatures.depthBounds ? "true" : "false");
  fprintf(stderr, "    wideLines: %s\n", deviceFeatures.wideLines ? "true" : "false");
  fprintf(stderr, "    largePoints: %s\n", deviceFeatures.largePoints ? "true" : "false");
  fprintf(stderr, "    alphaToOne: %s\n", deviceFeatures.alphaToOne ? "true" : "false");
  fprintf(stderr, "    multiViewport: %s\n", deviceFeatures.multiViewport ? "true" : "false");
  fprintf(stderr, "    samplerAnisotropy: %s\n", deviceFeatures.samplerAnisotropy ? "true" : "false");
  fprintf(stderr, "    textureCompressionETC2: %s\n", deviceFeatures.textureCompressionETC2 ? "true" : "false");
  fprintf(stderr, "    textureCompressionASTC_LDR: %s\n", deviceFeatures.textureCompressionASTC_LDR ? "true" : "false");
  fprintf(stderr, "    textureCompressionBC: %s\n", deviceFeatures.textureCompressionBC ? "true" : "false");
  fprintf(stderr, "    occlusionQueryPrecise: %s\n", deviceFeatures.occlusionQueryPrecise ? "true" : "false");
  fprintf(stderr, "    pipelineStatisticsQuery: %s\n", deviceFeatures.pipelineStatisticsQuery ? "true" : "false");
  fprintf(stderr, "    vertexPipelineStoresAndAtomics: %s\n", deviceFeatures.vertexPipelineStoresAndAtomics ? "true" : "false");
  fprintf(stderr, "    fragmentStoresAndAtomics: %s\n", deviceFeatures.fragmentStoresAndAtomics ? "true" : "false");
  fprintf(stderr, "    shaderTessellationAndGeometryPointSize: %s\n", deviceFeatures.shaderTessellationAndGeometryPointSize ? "true" : "false");
  fprintf(stderr, "    shaderImageGatherExtended: %s\n", deviceFeatures.shaderImageGatherExtended ? "true" : "false");
  fprintf(stderr, "    shaderStorageImageExtendedFormats: %s\n", deviceFeatures.shaderStorageImageExtendedFormats ? "true" : "false");
  fprintf(stderr, "    shaderStorageImageMultisample: %s\n", deviceFeatures.shaderStorageImageMultisample ? "true" : "false");
  fprintf(stderr, "    shaderStorageImageReadWithoutFormat: %s\n", deviceFeatures.shaderStorageImageReadWithoutFormat ? "true" : "false");
  fprintf(stderr, "    shaderStorageImageWriteWithoutFormat: %s\n", deviceFeatures.shaderStorageImageWriteWithoutFormat ? "true" : "false");
  fprintf(stderr, "    shaderUniformBufferArrayDynamicIndexing: %s\n", deviceFeatures.shaderUniformBufferArrayDynamicIndexing ? "true" : "false");
  fprintf(stderr, "    shaderSampledImageArrayDynamicIndexing: %s\n", deviceFeatures.shaderSampledImageArrayDynamicIndexing ? "true" : "false");
  fprintf(stderr, "    shaderStorageBufferArrayDynamicIndexing: %s\n", deviceFeatures.shaderStorageBufferArrayDynamicIndexing ? "true" : "false");
  fprintf(stderr, "    shaderStorageImageArrayDynamicIndexing: %s\n", deviceFeatures.shaderStorageImageArrayDynamicIndexing ? "true" : "false");
  fprintf(stderr, "    shaderClipDistance: %s\n", deviceFeatures.shaderClipDistance ? "true" : "false");
  fprintf(stderr, "    shaderCullDistance: %s\n", deviceFeatures.shaderCullDistance ? "true" : "false");
  fprintf(stderr, "    shaderFloat64: %s\n", deviceFeatures.shaderFloat64 ? "true" : "false");
  fprintf(stderr, "    shaderInt64: %s\n", deviceFeatures.shaderInt64 ? "true" : "false");
  fprintf(stderr, "    shaderInt16: %s\n", deviceFeatures.shaderInt16 ? "true" : "false");
  fprintf(stderr, "    shaderResourceResidency: %s\n", deviceFeatures.shaderResourceResidency ? "true" : "false");
  fprintf(stderr, "    shaderResourceMinLod: %s\n", deviceFeatures.shaderResourceMinLod ? "true" : "false");
  fprintf(stderr, "    sparseBinding: %s\n", deviceFeatures.sparseBinding ? "true" : "false");
  fprintf(stderr, "    sparseResidencyBuffer: %s\n", deviceFeatures.sparseResidencyBuffer ? "true" : "false");
  fprintf(stderr, "    sparseResidencyImage2D: %s\n", deviceFeatures.sparseResidencyImage2D ? "true" : "false");
  fprintf(stderr, "    sparseResidencyImage3D: %s\n", deviceFeatures.sparseResidencyImage3D ? "true" : "false");
  fprintf(stderr, "    sparseResidency2Samples: %s\n", deviceFeatures.sparseResidency2Samples ? "true" : "false");
  fprintf(stderr, "    sparseResidency4Samples: %s\n", deviceFeatures.sparseResidency4Samples ? "true" : "false");
  fprintf(stderr, "    sparseResidency8Samples: %s\n", deviceFeatures.sparseResidency8Samples ? "true" : "false");
  fprintf(stderr, "    sparseResidency16Samples: %s\n", deviceFeatures.sparseResidency16Samples ? "true" : "false");
  fprintf(stderr, "    sparseResidencyAliased: %s\n", deviceFeatures.sparseResidencyAliased ? "true" : "false");
  fprintf(stderr, "    variableMultisampleRate: %s\n", deviceFeatures.variableMultisampleRate ? "true" : "false");
  fprintf(stderr, "    inheritedQueries: %s\n", deviceFeatures.inheritedQueries ? "true" : "false");
  // clang-format on

  // get device extensions
  uint32_t extensionCount;
  vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, NULL);
  VkExtensionProperties availableDeviceExtensions[extensionCount];
  vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, availableDeviceExtensions);

  // print available device extensions
  fprintf(stderr, "  Available extensions:\n");
  for (int i = 0; i < extensionCount; i++) {
    fprintf(stderr, "    %s\n", availableDeviceExtensions[i].extensionName);
  }

  // print required device extensions
  fprintf(stderr, "  Required extensions:\n");
  for (int i = 0; i < requiredDeviceExtensionsCount; i++) {
    fprintf(stderr, "    %s\n", requiredDeviceExtensions[i]);
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

  // This is the criteria for being a suitable physical device: being a GPU!
  return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU || deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
}

int fstGraphicsQueueFamilyIndex() {
  int result = -1;
  uint32_t queueFamilyCount;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);

  VkQueueFamilyProperties queueFamilies[queueFamilyCount];
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies);

  VkBool32 surfaceSupport = VK_FALSE;
  for (int i = 0; i < queueFamilyCount; i++) {
    if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      err = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &surfaceSupport);
      handleError();
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
  int queueFamilyIndex = fstGraphicsQueueFamilyIndex();
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

void createImageViews() {
  swapChainImageViews = malloc(swapChainImagesCount * sizeof(VkImageView));
  for (size_t i = 0; i < swapChainImagesCount; i++) {
    VkImageViewCreateInfo imageViewCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = swapChainImages[i],
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = swapChainImageFormat,
        .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.baseMipLevel = 0,
        .subresourceRange.levelCount = 1,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount = 1,
    };
    err = vkCreateImageView(device, &imageViewCreateInfo, NULL, &swapChainImageViews[i]);
    handleError();
  }
}

void createRenderPass() {
  VkAttachmentDescription colorAttachment = {
      .format = swapChainImageFormat,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
  };

  VkAttachmentReference colorAttachmentRef = {
      .attachment = 0,
      .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
  };

  VkSubpassDescription subpass = {
      .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorAttachmentRef,
  };

  VkRenderPassCreateInfo renderPassInfo = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .attachmentCount = 1,
      .pAttachments = &colorAttachment,
      .subpassCount = 1,
      .pSubpasses = &subpass,
  };

  vkCreateRenderPass(device, &renderPassInfo, NULL, &renderPass);
  handleError();
}

VkShaderModule createShaderModule(gchar *code, gsize codeSize) {
  VkShaderModuleCreateInfo shaderCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = codeSize,
      .pCode = (const uint32_t *)code,
  };

  VkShaderModule shaderModule;
  err = vkCreateShaderModule(device, &shaderCreateInfo, NULL, &shaderModule);
  handleError();

  return shaderModule;
}

gboolean readFile(const char *filename, gchar **contents, gsize *length) { return g_file_get_contents(filename, contents, length, NULL); }

void createGraphicsPipeline() {
  gchar *vertShaderCode;
  gchar *fragShaderCode;
  gsize lengthVertShaderCode;
  gsize lengthFragShaderCode;
  readFile("shaders/vert.spv", &vertShaderCode, &lengthVertShaderCode);
  readFile("shaders/frag.spv", &fragShaderCode, &lengthFragShaderCode);

  VkShaderModule vertShaderModule = createShaderModule(vertShaderCode, lengthVertShaderCode);
  VkShaderModule fragShaderModule = createShaderModule(fragShaderCode, lengthFragShaderCode);

  VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_VERTEX_BIT,
      .module = vertShaderModule,
      .pName = "main",
  };

  VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
      .module = fragShaderModule,
      .pName = "main",
  };

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

  VkPipelineVertexInputStateCreateInfo vertexInput = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = 0,
      .vertexAttributeDescriptionCount = 0,
  };

  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .primitiveRestartEnable = VK_FALSE,
  };

  VkPipelineViewportStateCreateInfo viewportState = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .scissorCount = 1,
  };

  VkPipelineRasterizationStateCreateInfo rasterizer = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .depthClampEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .lineWidth = 1.0f,
      .cullMode = VK_CULL_MODE_BACK_BIT,
      .frontFace = VK_FRONT_FACE_CLOCKWISE,
      .depthBiasEnable = VK_FALSE,
  };

  VkPipelineMultisampleStateCreateInfo multisampling = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .sampleShadingEnable = VK_FALSE,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
  };

  VkPipelineColorBlendAttachmentState colorBlendAttachment = {
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
      .blendEnable = VK_FALSE,
  };

  VkPipelineColorBlendStateCreateInfo colorBlending = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .logicOpEnable = VK_FALSE,
      .logicOp = VK_LOGIC_OP_COPY,
      .attachmentCount = 1,
      .pAttachments = &colorBlendAttachment,
      .blendConstants[0] = 0.0f,
      .blendConstants[1] = 0.0f,
      .blendConstants[2] = 0.0f,
      .blendConstants[3] = 0.0f,
  };

  // search for vkCmdSet... function calls
  VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamicState = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .pDynamicStates = dynamicStates,
      .dynamicStateCount = sizeof(dynamicStates) / sizeof(VkDynamicState),
  };

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 0,
      .pushConstantRangeCount = 0,
  };

  err = vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, &pipelineLayout);
  handleError();

  VkGraphicsPipelineCreateInfo pipelineInfo = {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .stageCount = 2,
      .pStages = shaderStages,
      .pVertexInputState = &vertexInput,
      .pInputAssemblyState = &inputAssembly,
      .pViewportState = &viewportState,
      .pRasterizationState = &rasterizer,
      .pMultisampleState = &multisampling,
      .pColorBlendState = &colorBlending,
      .pDynamicState = &dynamicState,
      .layout = pipelineLayout,
      .renderPass = renderPass,
      .subpass = 0,
      .basePipelineHandle = VK_NULL_HANDLE,
  };

  err = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &graphicsPipeline);
  handleError();

  vkDestroyShaderModule(device, fragShaderModule, NULL);
  vkDestroyShaderModule(device, vertShaderModule, NULL);
}

void createFramebuffers() {
  swapChainFramebuffers = malloc(swapChainImagesCount * sizeof(VkFramebuffer));
  for (size_t i = 0; i < swapChainImagesCount; i++) {
    VkImageView attachments[] = {swapChainImageViews[i]};

    // TODO: for documentation → renderPass is in the graphics pipeline and the framebuffers
    VkFramebufferCreateInfo framebufferInfo = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = renderPass,
        .attachmentCount = 1,
        .pAttachments = attachments,
        .width = swapChainExtent.width,
        .height = swapChainExtent.height,
        .layers = 1,
    };

    err = vkCreateFramebuffer(device, &framebufferInfo, NULL, &swapChainFramebuffers[i]);
    handleError();
  }
}

void createCommandPool() {
  VkCommandPoolCreateInfo poolInfo = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
      .queueFamilyIndex = fstGraphicsQueueFamilyIndex(),
  };

  err = vkCreateCommandPool(device, &poolInfo, NULL, &commandPool);
  handleError();
}

void createCommandBuffers() {
  commandBuffers = malloc(MAX_FRAMES_IN_FLIGHT * sizeof(VkCommandBuffer));

  VkCommandBufferAllocateInfo cmdBufferAllocInfo = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = commandPool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = (uint32_t)MAX_FRAMES_IN_FLIGHT,
  };

  err = vkAllocateCommandBuffers(device, &cmdBufferAllocInfo, commandBuffers);
  handleError();
}

void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
  VkCommandBufferBeginInfo beginInfo = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };

  err = vkBeginCommandBuffer(commandBuffer, &beginInfo);
  handleError();

  VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
  VkRenderPassBeginInfo renderPassBeginInfo = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .renderPass = renderPass,
      .framebuffer = swapChainFramebuffers[imageIndex],
      .renderArea.offset = {0, 0},
      .renderArea.extent = swapChainExtent,
      .clearValueCount = 1,
      .pClearValues = &clearColor,
  };

  vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

  // viewport was defined to be dynamic
  // search for "VkDynamicState"
  VkViewport viewport = {
      .x = 0.0f,
      .y = 0.0f,
      .width = swapChainExtent.width,
      .height = swapChainExtent.height,
      .minDepth = 0.0f,
      .maxDepth = 1.0f,
  };
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  // scissors were defined to be dynamic
  // search for "VkDynamicState"
  VkRect2D scissor = {
      .offset = {0, 0},
      .extent = swapChainExtent,
  };
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  // uses topology of the pipeline
  // search for topology 
  vkCmdDraw(commandBuffer, 3, 1, 0, 0);
  vkCmdEndRenderPass(commandBuffer);

  err = vkEndCommandBuffer(commandBuffer);
  handleError();
}

void createSyncObjects() {
  imageAvailableSemaphores = malloc(MAX_FRAMES_IN_FLIGHT * sizeof(VkSemaphore));
  finishedRenderingSemaphores = malloc(MAX_FRAMES_IN_FLIGHT * sizeof(VkSemaphore));
  inFlightFences = malloc(MAX_FRAMES_IN_FLIGHT * sizeof(VkFence));

  VkSemaphoreCreateInfo semaphoreInfo = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  };

  VkFenceCreateInfo fenceInfo = {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .flags = VK_FENCE_CREATE_SIGNALED_BIT,
  };

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    err = vkCreateSemaphore(device, &semaphoreInfo, NULL, &imageAvailableSemaphores[i]);
    handleError();
    err = vkCreateSemaphore(device, &semaphoreInfo, NULL, &finishedRenderingSemaphores[i]);
    handleError();
    err = vkCreateFence(device, &fenceInfo, NULL, &inFlightFences[i]);
    handleError();
  }
}

void drawFrame() {
  // wait for the previous frame to finish
  err = vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
  handleError();
  err = vkResetFences(device, 1, &inFlightFences[currentFrame]);
  handleError();

  // acquire an image from the swap chain
  uint32_t imageIndex;
  err = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
  handleError();

  // record command buffer which draws the scene onto acquired image
  err = vkResetCommandBuffer(commandBuffers[currentFrame], 0);
  handleError();
  recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

  VkSubmitInfo submitInfo = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
  };

  VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

  VkSemaphore signalSemaphores[] = {finishedRenderingSemaphores[currentFrame]};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  // submit recorded command buffer
  err = vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]);
  handleError();

  VkPresentInfoKHR presentInfo = {
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
  };

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  VkSwapchainKHR swapChains[] = {swapChain};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;

  presentInfo.pImageIndices = &imageIndex;

  // present swap chain image
  vkQueuePresentKHR(graphicsQueue, &presentInfo);

  currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void deviceWaitIdle() { vkDeviceWaitIdle(device); };

void cleanup() {
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(device, finishedRenderingSemaphores[i], NULL);
    vkDestroySemaphore(device, imageAvailableSemaphores[i], NULL);
    vkDestroyFence(device, inFlightFences[i], NULL);
  }

  vkDestroyCommandPool(device, commandPool, NULL);
  for (int i = 0; i < swapChainImagesCount; i++) {
    vkDestroyFramebuffer(device, swapChainFramebuffers[i], NULL);
  }
  vkDestroyPipeline(device, graphicsPipeline, NULL);
  vkDestroyPipelineLayout(device, pipelineLayout, NULL);
  vkDestroyRenderPass(device, renderPass, NULL);
  for (int i = 0; i < swapChainImagesCount; i++) {
    vkDestroyImageView(device, swapChainImageViews[i], NULL);
  }
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
  createImageViews();
  createRenderPass();
  createGraphicsPipeline();
  createFramebuffers();
  createCommandPool();
  createCommandBuffers();
  createSyncObjects();
}
