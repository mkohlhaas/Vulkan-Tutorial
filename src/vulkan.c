#define MAX_FRAMES_IN_FLIGHT 2
#define NANO 1000000000L

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "vkTutorial.h"
#include <bits/time.h>
#include <cglm/cglm.h>
#include <glib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <vulkan/vulkan_core.h>

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
struct timespec startTime;

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

VkBuffer vertexBuffer;
VkDeviceMemory vertexBufferMemory;

VkBuffer indexBuffer;
VkDeviceMemory indexBufferMemory;

VkBuffer uniformBuffers[MAX_FRAMES_IN_FLIGHT];
VkDeviceMemory uniformBuffersMemory[MAX_FRAMES_IN_FLIGHT];
void *uniformBuffersMapped[MAX_FRAMES_IN_FLIGHT];
VkDescriptorSetLayout descriptorSetLayout;
VkDescriptorPool descriptorPool;
VkDescriptorSet descriptorSets[MAX_FRAMES_IN_FLIGHT];

VkImage textureImage;
VkDeviceMemory textureImageMemory;
VkImageView textureImageView;
VkSampler textureSampler;

typedef struct UniformBufferObject {
  mat4 model;
  mat4 view;
  mat4 proj;
} UniformBufferObject;

uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }
  err = VKT_ERROR_NO_SUITABLE_MEMORY_AVAILABLE;
  handleError();
  return -1;
}

void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer *buffer, VkDeviceMemory *bufferMemory) {
  VkBufferCreateInfo bufferInfo = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = size,
      .usage = usage,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };

  err = vkCreateBuffer(device, &bufferInfo, NULL, buffer);
  handleError();

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device, *buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memRequirements.size,
      .memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties),
  };

  err = vkAllocateMemory(device, &allocInfo, NULL, bufferMemory);
  handleError();

  vkBindBufferMemory(device, *buffer, *bufferMemory, 0);
}

void CreateUniformBuffers() {
  VkDeviceSize bufferSize = sizeof(UniformBufferObject);
  int bufferUsageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  int memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    CreateBuffer(bufferSize, bufferUsageFlags, memoryProperties, &uniformBuffers[i], &uniformBuffersMemory[i]);
    vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
  }
}

void CreateDescriptorSetLayout() {
  VkDescriptorSetLayoutBinding uboLayoutBinding = {
      .binding = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .pImmutableSamplers = NULL,
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
  };

  VkDescriptorSetLayoutBinding samplerLayoutBinding = {
      .binding = 1,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .pImmutableSamplers = NULL,
      .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
  };

  VkDescriptorSetLayoutBinding bindings[] = {uboLayoutBinding, samplerLayoutBinding};
  VkDescriptorSetLayoutCreateInfo layoutInfo = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = sizeof(bindings) / sizeof(VkDescriptorSetLayoutBinding),
      .pBindings = bindings,
  };

  err = vkCreateDescriptorSetLayout(device, &layoutInfo, NULL, &descriptorSetLayout);
  handleError();
}

void CreateDescriptorPool() {
  VkDescriptorPoolSize poolSizes[] = {{
                                          .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                          .descriptorCount = MAX_FRAMES_IN_FLIGHT,
                                      },
                                      {
                                          .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                          .descriptorCount = MAX_FRAMES_IN_FLIGHT,
                                      }};

  VkDescriptorPoolCreateInfo poolInfo = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .poolSizeCount = sizeof(poolSizes) / sizeof(VkDescriptorPoolSize),
      .pPoolSizes = poolSizes,
      .maxSets = MAX_FRAMES_IN_FLIGHT,
  };

  err = vkCreateDescriptorPool(device, &poolInfo, NULL, &descriptorPool);
  handleError();
}

void CreateDescriptorSets() {
  VkDescriptorSetLayout layouts[MAX_FRAMES_IN_FLIGHT] = {descriptorSetLayout, descriptorSetLayout};
  VkDescriptorSetAllocateInfo allocInfo = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = descriptorPool,
      .descriptorSetCount = MAX_FRAMES_IN_FLIGHT,
      .pSetLayouts = layouts,
  };

  err = vkAllocateDescriptorSets(device, &allocInfo, descriptorSets);
  handleError();

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    VkDescriptorBufferInfo bufferInfo = {
        .buffer = uniformBuffers[i],
        .offset = 0,
        .range = sizeof(UniformBufferObject),
    };

    VkDescriptorImageInfo imageInfo = {
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .imageView = textureImageView,
        .sampler = textureSampler,
    };

    VkWriteDescriptorSet descriptorWrites[] = {{
                                                   .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                                   .dstSet = descriptorSets[i],
                                                   .dstBinding = 0,
                                                   .dstArrayElement = 0,
                                                   .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                   .descriptorCount = 1,
                                                   .pBufferInfo = &bufferInfo,
                                               },
                                               {
                                                   .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                                   .dstSet = descriptorSets[i],
                                                   .dstBinding = 1,
                                                   .dstArrayElement = 0,
                                                   .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                   .descriptorCount = 1,
                                                   .pImageInfo = &imageInfo,
                                               }};

    uint32_t descCount = sizeof(descriptorWrites) / sizeof(VkWriteDescriptorSet);
    vkUpdateDescriptorSets(device, descCount, (VkWriteDescriptorSet *)&descriptorWrites, 0, NULL);
  }
}

typedef struct Vertex {
  vec2 pos;
  vec3 color;
  vec2 texCoord;
} Vertex;

const Vertex vertices[] = {{{-1.25f, -1.25f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
                           {{+1.25f, -1.25f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
                           {{+1.25f, +1.25f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
                           {{-1.25f, +1.25f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}};


const uint16_t indices[] = {0, 1, 2, 2, 3, 0};

static VkVertexInputAttributeDescription *getAttributeDescriptions() {
  VkVertexInputAttributeDescription tmpDesc[] = {{
                                                     .binding = 0,
                                                     .location = 0,
                                                     .format = VK_FORMAT_R32G32_SFLOAT,
                                                     .offset = offsetof(Vertex, pos),
                                                 },
                                                 {
                                                     .binding = 0,
                                                     .location = 1,
                                                     .format = VK_FORMAT_R32G32B32_SFLOAT,
                                                     .offset = offsetof(Vertex, color),
                                                 },
                                                 {
                                                     .binding = 0,
                                                     .location = 2,
                                                     .format = VK_FORMAT_R32G32_SFLOAT,
                                                     .offset = offsetof(Vertex, texCoord),
                                                 }};
  int arraySize = sizeof(tmpDesc);
  VkVertexInputAttributeDescription *attributeDescriptions = malloc(arraySize);
  memcpy(attributeDescriptions, tmpDesc, arraySize);
  return attributeDescriptions;
}

static VkVertexInputBindingDescription *getBindingDescription() {
  VkVertexInputBindingDescription tmpDesc[] = {{
      .binding = 0,
      .stride = sizeof(Vertex),
      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
  }};
  int arraySize = sizeof(tmpDesc);
  VkVertexInputBindingDescription *bindingDescription = malloc(arraySize);
  memcpy(bindingDescription, tmpDesc, arraySize);
  return bindingDescription;
}

void CreateVertexBuffer() {
  VkDeviceSize bufferSize = sizeof(vertices);
  int usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  int memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  CreateBuffer(bufferSize, usageFlags, memoryProperties, &stagingBuffer, &stagingBufferMemory);

  void *data;
  vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, vertices, (size_t)bufferSize);
  vkUnmapMemory(device, stagingBufferMemory);

  usageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  CreateBuffer(bufferSize, usageFlags, memoryProperties, &vertexBuffer, &vertexBufferMemory);
  CopyBuffer(stagingBuffer, vertexBuffer, bufferSize);

  vkDestroyBuffer(device, stagingBuffer, NULL);
  vkFreeMemory(device, stagingBufferMemory, NULL);
}

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

void SetupDebugMessenger() {
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

bool CheckValidationLayerSupport() {
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

void CreateInstance() {
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

  // print instance information
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

void CreateSwapChain() {
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

bool isPhysicalDeviceSuitable(VkPhysicalDevice physicalDevice) {
  // get device properties and features
  VkPhysicalDeviceProperties physicalDeviceProperties;
  vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
  VkPhysicalDeviceFeatures physicalDeviceFeatures;
  vkGetPhysicalDeviceFeatures(physicalDevice, &physicalDeviceFeatures);

  // print physical device info
  // clang-format off
  fprintf(stderr, "\nPhysical Device:\n");
  uint32_t  vId = physicalDeviceProperties.vendorID;
  fprintf(stderr, "  Vendor: %s\n",  vId == 0x1002 ? "AMD" : vId == 0x1010 ? "ImgTec" : vId == 0x10DE ? "Nvidia" : vId == 0x13B5 ? "ARM" : vId == 0x5143 ? "Qualcomm" : vId == 0x8086 ? "Intel" : "Unknown");
  fprintf(stderr, "  Device Name: %s\n",  physicalDeviceProperties.deviceName);
  fprintf(stderr, "  Limits:\n");
  fprintf(stderr, "    maxImageDimension1D: %u\n", physicalDeviceProperties.limits.maxImageDimension1D);
  fprintf(stderr, "    maxImageDimension2D: %u\n", physicalDeviceProperties.limits.maxImageDimension2D);
  fprintf(stderr, "    maxImageDimension3D: %u\n", physicalDeviceProperties.limits.maxImageDimension3D);
  fprintf(stderr, "    maxImageDimensionCube: %u\n", physicalDeviceProperties.limits.maxImageDimensionCube);
  fprintf(stderr, "    maxImageArrayLayers: %u\n", physicalDeviceProperties.limits.maxImageArrayLayers);
  fprintf(stderr, "    maxTexelBufferElements: %u\n", physicalDeviceProperties.limits.maxTexelBufferElements);
  fprintf(stderr, "    maxUniformBufferRange: %u\n", physicalDeviceProperties.limits.maxUniformBufferRange);
  fprintf(stderr, "    maxStorageBufferRange: %u\n", physicalDeviceProperties.limits.maxStorageBufferRange);
  fprintf(stderr, "    maxPushConstantsSize: %u\n", physicalDeviceProperties.limits.maxPushConstantsSize);
  fprintf(stderr, "    maxMemoryAllocationCount: %u\n", physicalDeviceProperties.limits.maxMemoryAllocationCount);
  fprintf(stderr, "    maxSamplerAllocationCount: %u\n", physicalDeviceProperties.limits.maxSamplerAllocationCount);
  fprintf(stderr, "    bufferImageGranularity: %lu\n", physicalDeviceProperties.limits.bufferImageGranularity);
  fprintf(stderr, "    sparseAddressSpaceSize: %lu\n", physicalDeviceProperties.limits.sparseAddressSpaceSize);
  fprintf(stderr, "    maxBoundDescriptorSets: %u\n", physicalDeviceProperties.limits.maxBoundDescriptorSets);
  fprintf(stderr, "    maxPerStageDescriptorSamplers: %u\n", physicalDeviceProperties.limits.maxPerStageDescriptorSamplers);
  fprintf(stderr, "    maxPerStageDescriptorUniformBuffers: %u\n", physicalDeviceProperties.limits.maxPerStageDescriptorUniformBuffers);
  fprintf(stderr, "    maxPerStageDescriptorStorageBuffers: %u\n", physicalDeviceProperties.limits.maxPerStageDescriptorStorageBuffers);
  fprintf(stderr, "    maxPerStageDescriptorSampledImages: %u\n", physicalDeviceProperties.limits.maxPerStageDescriptorSampledImages);
  fprintf(stderr, "    maxPerStageDescriptorStorageImages: %u\n", physicalDeviceProperties.limits.maxPerStageDescriptorStorageImages);
  fprintf(stderr, "    maxPerStageDescriptorInputAttachments: %u\n", physicalDeviceProperties.limits.maxPerStageDescriptorInputAttachments);
  fprintf(stderr, "    maxPerStageResources: %u\n", physicalDeviceProperties.limits.maxPerStageResources);
  fprintf(stderr, "    maxDescriptorSetSamplers: %u\n", physicalDeviceProperties.limits.maxDescriptorSetSamplers);
  fprintf(stderr, "    maxDescriptorSetUniformBuffers: %u\n", physicalDeviceProperties.limits.maxDescriptorSetUniformBuffers);
  fprintf(stderr, "    maxDescriptorSetUniformBuffersDynamic: %u\n", physicalDeviceProperties.limits.maxDescriptorSetUniformBuffersDynamic);
  fprintf(stderr, "    maxDescriptorSetStorageBuffers: %u\n", physicalDeviceProperties.limits.maxDescriptorSetStorageBuffers);
  fprintf(stderr, "    maxDescriptorSetStorageBuffersDynamic: %u\n", physicalDeviceProperties.limits.maxDescriptorSetStorageBuffersDynamic);
  fprintf(stderr, "    maxDescriptorSetSampledImages: %u\n", physicalDeviceProperties.limits.maxDescriptorSetSampledImages);
  fprintf(stderr, "    maxDescriptorSetStorageImages: %u\n", physicalDeviceProperties.limits.maxDescriptorSetStorageImages);
  fprintf(stderr, "    maxDescriptorSetInputAttachments: %u\n", physicalDeviceProperties.limits.maxDescriptorSetInputAttachments);
  fprintf(stderr, "    maxVertexInputAttributes: %u\n", physicalDeviceProperties.limits.maxVertexInputAttributes);
  fprintf(stderr, "    maxVertexInputBindings: %u\n", physicalDeviceProperties.limits.maxVertexInputBindings);
  fprintf(stderr, "    maxVertexInputAttributeOffset: %u\n", physicalDeviceProperties.limits.maxVertexInputAttributeOffset);
  fprintf(stderr, "    maxVertexInputBindingStride: %u\n", physicalDeviceProperties.limits.maxVertexInputBindingStride);
  fprintf(stderr, "    maxVertexOutputComponents: %u\n", physicalDeviceProperties.limits.maxVertexOutputComponents);
  fprintf(stderr, "    maxTessellationGenerationLevel: %u\n", physicalDeviceProperties.limits.maxTessellationGenerationLevel);
  fprintf(stderr, "    maxTessellationPatchSize: %u\n", physicalDeviceProperties.limits.maxTessellationPatchSize);
  fprintf(stderr, "    maxTessellationControlPerVertexInputComponents: %u\n", physicalDeviceProperties.limits.maxTessellationControlPerVertexInputComponents);
  fprintf(stderr, "    maxTessellationControlPerVertexOutputComponents: %u\n", physicalDeviceProperties.limits.maxTessellationControlPerVertexOutputComponents);
  fprintf(stderr, "    maxTessellationControlPerPatchOutputComponents: %u\n", physicalDeviceProperties.limits.maxTessellationControlPerPatchOutputComponents);
  fprintf(stderr, "    maxTessellationControlTotalOutputComponents: %u\n", physicalDeviceProperties.limits.maxTessellationControlTotalOutputComponents);
  fprintf(stderr, "    maxTessellationEvaluationInputComponents: %u\n", physicalDeviceProperties.limits.maxTessellationEvaluationInputComponents);
  fprintf(stderr, "    maxTessellationEvaluationOutputComponents: %u\n", physicalDeviceProperties.limits.maxTessellationEvaluationOutputComponents);
  fprintf(stderr, "    maxGeometryShaderInvocations: %u\n", physicalDeviceProperties.limits.maxGeometryShaderInvocations);
  fprintf(stderr, "    maxGeometryInputComponents: %u\n", physicalDeviceProperties.limits.maxGeometryInputComponents);
  fprintf(stderr, "    maxGeometryOutputComponents: %u\n", physicalDeviceProperties.limits.maxGeometryOutputComponents);
  fprintf(stderr, "    maxGeometryOutputVertices: %u\n", physicalDeviceProperties.limits.maxGeometryOutputVertices);
  fprintf(stderr, "    maxGeometryTotalOutputComponents: %u\n", physicalDeviceProperties.limits.maxGeometryTotalOutputComponents);
  fprintf(stderr, "    maxFragmentInputComponents: %u\n", physicalDeviceProperties.limits.maxFragmentInputComponents);
  fprintf(stderr, "    maxFragmentOutputAttachments: %u\n", physicalDeviceProperties.limits.maxFragmentOutputAttachments);
  fprintf(stderr, "    maxFragmentDualSrcAttachments: %u\n", physicalDeviceProperties.limits.maxFragmentDualSrcAttachments);
  fprintf(stderr, "    maxFragmentCombinedOutputResources: %u\n", physicalDeviceProperties.limits.maxFragmentCombinedOutputResources);
  fprintf(stderr, "    maxComputeSharedMemorySize: %u\n", physicalDeviceProperties.limits.maxComputeSharedMemorySize);
  fprintf(stderr, "    maxComputeWorkGroupCount(: %u, %u, %u)\n", physicalDeviceProperties.limits.maxComputeWorkGroupCount[0], physicalDeviceProperties.limits.maxComputeWorkGroupCount[1], physicalDeviceProperties.limits.maxComputeWorkGroupCount[2]);
  fprintf(stderr, "    maxComputeWorkGroupInvocationsmaxComputeWorkGroupInvocations: %u\n", physicalDeviceProperties.limits.maxComputeWorkGroupInvocations);
  fprintf(stderr, "    maxComputeWorkGroupSize(: %u, %u, %u)\n", physicalDeviceProperties.limits.maxComputeWorkGroupSize[0], physicalDeviceProperties.limits.maxComputeWorkGroupSize[1], physicalDeviceProperties.limits.maxComputeWorkGroupSize[2]);
  fprintf(stderr, "    subPixelPrecisionBits: %u\n", physicalDeviceProperties.limits.subPixelPrecisionBits);
  fprintf(stderr, "    subTexelPrecisionBits: %u\n", physicalDeviceProperties.limits.subTexelPrecisionBits);
  fprintf(stderr, "    mipmapPrecisionBits: %u\n", physicalDeviceProperties.limits.mipmapPrecisionBits);
  fprintf(stderr, "    maxDrawIndexedIndexValue: %u\n", physicalDeviceProperties.limits.maxDrawIndexedIndexValue);
  fprintf(stderr, "    maxDrawIndirectCount: %u\n", physicalDeviceProperties.limits.maxDrawIndirectCount);
  fprintf(stderr, "    maxSamplerLodBias: %f\n", physicalDeviceProperties.limits.maxSamplerLodBias);
  fprintf(stderr, "    maxSamplerAnisotropy: %f\n", physicalDeviceProperties.limits.maxSamplerAnisotropy);
  fprintf(stderr, "    maxViewports: %u\n", physicalDeviceProperties.limits.maxViewports);
  fprintf(stderr, "    maxViewportDimensions(: %u, %u)\n", physicalDeviceProperties.limits.maxViewportDimensions[0], physicalDeviceProperties.limits.maxViewportDimensions[1]);
  fprintf(stderr, "    viewportBoundsRange(: %f, %f)\n", physicalDeviceProperties.limits.viewportBoundsRange[0], physicalDeviceProperties.limits.viewportBoundsRange[1]);
  fprintf(stderr, "    viewportSubPixelBits: %u\n", physicalDeviceProperties.limits.viewportSubPixelBits);
  fprintf(stderr, "    minMemoryMapAlignment: %zu\n", physicalDeviceProperties.limits.minMemoryMapAlignment);
  fprintf(stderr, "    minTexelBufferOffsetAlignment: %lu\n", physicalDeviceProperties.limits.minTexelBufferOffsetAlignment);
  fprintf(stderr, "    minUniformBufferOffsetAlignment: %lu\n", physicalDeviceProperties.limits.minUniformBufferOffsetAlignment);
  fprintf(stderr, "    minStorageBufferOffsetAlignment: %lu\n", physicalDeviceProperties.limits.minStorageBufferOffsetAlignment);
  fprintf(stderr, "    minTexelOffset: %d\n", physicalDeviceProperties.limits.minTexelOffset);
  fprintf(stderr, "    maxTexelOffset: %u\n", physicalDeviceProperties.limits.maxTexelOffset);
  fprintf(stderr, "    minTexelGatherOffset: %d\n", physicalDeviceProperties.limits.minTexelGatherOffset);
  fprintf(stderr, "    maxTexelGatherOffset: %u\n", physicalDeviceProperties.limits.maxTexelGatherOffset);
  fprintf(stderr, "    minInterpolationOffset: %f\n", physicalDeviceProperties.limits.minInterpolationOffset);
  fprintf(stderr, "    maxInterpolationOffset: %f\n", physicalDeviceProperties.limits.maxInterpolationOffset);
  fprintf(stderr, "    subPixelInterpolationOffsetBits: %u\n", physicalDeviceProperties.limits.subPixelInterpolationOffsetBits);
  fprintf(stderr, "    maxFramebufferWidth: %u\n", physicalDeviceProperties.limits.maxFramebufferWidth);
  fprintf(stderr, "    maxFramebufferHeight: %u\n", physicalDeviceProperties.limits.maxFramebufferHeight);
  fprintf(stderr, "    maxFramebufferLayers: %u\n", physicalDeviceProperties.limits.maxFramebufferLayers);
  fprintf(stderr, "    framebufferColorSampleCounts: %u\n", physicalDeviceProperties.limits.framebufferColorSampleCounts);
  fprintf(stderr, "    framebufferDepthSampleCounts: %u\n", physicalDeviceProperties.limits.framebufferDepthSampleCounts);
  fprintf(stderr, "    framebufferStencilSampleCounts: %u\n", physicalDeviceProperties.limits.framebufferStencilSampleCounts);
  fprintf(stderr, "    framebufferNoAttachmentsSampleCounts: %u\n", physicalDeviceProperties.limits.framebufferNoAttachmentsSampleCounts);
  fprintf(stderr, "    maxColorAttachments: %u\n", physicalDeviceProperties.limits.maxColorAttachments);
  fprintf(stderr, "    sampledImageColorSampleCounts: %u\n", physicalDeviceProperties.limits.sampledImageColorSampleCounts);
  fprintf(stderr, "    sampledImageIntegerSampleCounts: %u\n", physicalDeviceProperties.limits.sampledImageIntegerSampleCounts);
  fprintf(stderr, "    sampledImageDepthSampleCounts: %u\n", physicalDeviceProperties.limits.sampledImageDepthSampleCounts);
  fprintf(stderr, "    sampledImageStencilSampleCounts: %u\n", physicalDeviceProperties.limits.sampledImageStencilSampleCounts);
  fprintf(stderr, "    storageImageSampleCounts: %u\n", physicalDeviceProperties.limits.storageImageSampleCounts);
  fprintf(stderr, "    maxSampleMaskWords: %u\n", physicalDeviceProperties.limits.maxSampleMaskWords);
  fprintf(stderr, "    timestampComputeAndGraphics: %s\n", physicalDeviceProperties.limits.timestampComputeAndGraphics ? "true" : "false");
  fprintf(stderr, "    timestampPeriod: %f\n", physicalDeviceProperties.limits.timestampPeriod);
  fprintf(stderr, "    maxClipDistances: %u\n", physicalDeviceProperties.limits.maxClipDistances);
  fprintf(stderr, "    maxCullDistances: %u\n", physicalDeviceProperties.limits.maxCullDistances);
  fprintf(stderr, "    maxCombinedClipAndCullDistances: %u\n", physicalDeviceProperties.limits.maxCombinedClipAndCullDistances);
  fprintf(stderr, "    discreteQueuePriorities: %u\n", physicalDeviceProperties.limits.discreteQueuePriorities);
  fprintf(stderr, "    pointSizeRange(: %f, %f)\n", physicalDeviceProperties.limits.pointSizeRange[0], physicalDeviceProperties.limits.pointSizeRange[1]);
  fprintf(stderr, "    lineWidthRange(: %f, %f)\n", physicalDeviceProperties.limits.lineWidthRange[0], physicalDeviceProperties.limits.lineWidthRange[1]);
  fprintf(stderr, "    pointSizeGranularity: %f\n", physicalDeviceProperties.limits.pointSizeGranularity);
  fprintf(stderr, "    lineWidthGranularity: %f\n", physicalDeviceProperties.limits.lineWidthGranularity);
  fprintf(stderr, "    strictLines: %s\n", physicalDeviceProperties.limits.strictLines? "true" : "false");
  fprintf(stderr, "    standardSampleLocations: %s\n", physicalDeviceProperties.limits.standardSampleLocations? "true" : "false");
  fprintf(stderr, "    optimalBufferCopyOffsetAlignment: %lu\n", physicalDeviceProperties.limits.optimalBufferCopyOffsetAlignment);
  fprintf(stderr, "    optimalBufferCopyRowPitchAlignment: %lu\n", physicalDeviceProperties.limits.optimalBufferCopyRowPitchAlignment);
  fprintf(stderr, "    nonCoherentAtomSize: %lu\n", physicalDeviceProperties.limits.nonCoherentAtomSize);
  fprintf(stderr, "  Features:\n");
  fprintf(stderr, "    robustBufferAccess: %s\n", physicalDeviceFeatures.robustBufferAccess ? "true" : "false");
  fprintf(stderr, "    fullDrawIndexUint32: %s\n", physicalDeviceFeatures.fullDrawIndexUint32 ? "true" : "false");
  fprintf(stderr, "    imageCubeArray: %s\n", physicalDeviceFeatures.imageCubeArray ? "true" : "false");
  fprintf(stderr, "    independentBlend: %s\n", physicalDeviceFeatures.independentBlend ? "true" : "false");
  fprintf(stderr, "    geometryShader: %s\n", physicalDeviceFeatures.geometryShader ? "true" : "false");
  fprintf(stderr, "    tessellationShader: %s\n", physicalDeviceFeatures.tessellationShader ? "true" : "false");
  fprintf(stderr, "    sampleRateShading: %s\n", physicalDeviceFeatures.sampleRateShading ? "true" : "false");
  fprintf(stderr, "    dualSrcBlend: %s\n", physicalDeviceFeatures.dualSrcBlend ? "true" : "false");
  fprintf(stderr, "    logicOp: %s\n", physicalDeviceFeatures.logicOp ? "true" : "false");
  fprintf(stderr, "    multiDrawIndirect: %s\n", physicalDeviceFeatures.multiDrawIndirect ? "true" : "false");
  fprintf(stderr, "    drawIndirectFirstInstance: %s\n", physicalDeviceFeatures.drawIndirectFirstInstance ? "true" : "false");
  fprintf(stderr, "    depthClamp: %s\n", physicalDeviceFeatures.depthClamp ? "true" : "false");
  fprintf(stderr, "    depthBiasClamp: %s\n", physicalDeviceFeatures.depthBiasClamp ? "true" : "false");
  fprintf(stderr, "    fillModeNonSolid: %s\n", physicalDeviceFeatures.fillModeNonSolid ? "true" : "false");
  fprintf(stderr, "    depthBounds: %s\n", physicalDeviceFeatures.depthBounds ? "true" : "false");
  fprintf(stderr, "    wideLines: %s\n", physicalDeviceFeatures.wideLines ? "true" : "false");
  fprintf(stderr, "    largePoints: %s\n", physicalDeviceFeatures.largePoints ? "true" : "false");
  fprintf(stderr, "    alphaToOne: %s\n", physicalDeviceFeatures.alphaToOne ? "true" : "false");
  fprintf(stderr, "    multiViewport: %s\n", physicalDeviceFeatures.multiViewport ? "true" : "false");
  fprintf(stderr, "    samplerAnisotropy: %s\n", physicalDeviceFeatures.samplerAnisotropy ? "true" : "false");
  fprintf(stderr, "    textureCompressionETC2: %s\n", physicalDeviceFeatures.textureCompressionETC2 ? "true" : "false");
  fprintf(stderr, "    textureCompressionASTC_LDR: %s\n", physicalDeviceFeatures.textureCompressionASTC_LDR ? "true" : "false");
  fprintf(stderr, "    textureCompressionBC: %s\n", physicalDeviceFeatures.textureCompressionBC ? "true" : "false");
  fprintf(stderr, "    occlusionQueryPrecise: %s\n", physicalDeviceFeatures.occlusionQueryPrecise ? "true" : "false");
  fprintf(stderr, "    pipelineStatisticsQuery: %s\n", physicalDeviceFeatures.pipelineStatisticsQuery ? "true" : "false");
  fprintf(stderr, "    vertexPipelineStoresAndAtomics: %s\n", physicalDeviceFeatures.vertexPipelineStoresAndAtomics ? "true" : "false");
  fprintf(stderr, "    fragmentStoresAndAtomics: %s\n", physicalDeviceFeatures.fragmentStoresAndAtomics ? "true" : "false");
  fprintf(stderr, "    shaderTessellationAndGeometryPointSize: %s\n", physicalDeviceFeatures.shaderTessellationAndGeometryPointSize ? "true" : "false");
  fprintf(stderr, "    shaderImageGatherExtended: %s\n", physicalDeviceFeatures.shaderImageGatherExtended ? "true" : "false");
  fprintf(stderr, "    shaderStorageImageExtendedFormats: %s\n", physicalDeviceFeatures.shaderStorageImageExtendedFormats ? "true" : "false");
  fprintf(stderr, "    shaderStorageImageMultisample: %s\n", physicalDeviceFeatures.shaderStorageImageMultisample ? "true" : "false");
  fprintf(stderr, "    shaderStorageImageReadWithoutFormat: %s\n", physicalDeviceFeatures.shaderStorageImageReadWithoutFormat ? "true" : "false");
  fprintf(stderr, "    shaderStorageImageWriteWithoutFormat: %s\n", physicalDeviceFeatures.shaderStorageImageWriteWithoutFormat ? "true" : "false");
  fprintf(stderr, "    shaderUniformBufferArrayDynamicIndexing: %s\n", physicalDeviceFeatures.shaderUniformBufferArrayDynamicIndexing ? "true" : "false");
  fprintf(stderr, "    shaderSampledImageArrayDynamicIndexing: %s\n", physicalDeviceFeatures.shaderSampledImageArrayDynamicIndexing ? "true" : "false");
  fprintf(stderr, "    shaderStorageBufferArrayDynamicIndexing: %s\n", physicalDeviceFeatures.shaderStorageBufferArrayDynamicIndexing ? "true" : "false");
  fprintf(stderr, "    shaderStorageImageArrayDynamicIndexing: %s\n", physicalDeviceFeatures.shaderStorageImageArrayDynamicIndexing ? "true" : "false");
  fprintf(stderr, "    shaderClipDistance: %s\n", physicalDeviceFeatures.shaderClipDistance ? "true" : "false");
  fprintf(stderr, "    shaderCullDistance: %s\n", physicalDeviceFeatures.shaderCullDistance ? "true" : "false");
  fprintf(stderr, "    shaderFloat64: %s\n", physicalDeviceFeatures.shaderFloat64 ? "true" : "false");
  fprintf(stderr, "    shaderInt64: %s\n", physicalDeviceFeatures.shaderInt64 ? "true" : "false");
  fprintf(stderr, "    shaderInt16: %s\n", physicalDeviceFeatures.shaderInt16 ? "true" : "false");
  fprintf(stderr, "    shaderResourceResidency: %s\n", physicalDeviceFeatures.shaderResourceResidency ? "true" : "false");
  fprintf(stderr, "    shaderResourceMinLod: %s\n", physicalDeviceFeatures.shaderResourceMinLod ? "true" : "false");
  fprintf(stderr, "    sparseBinding: %s\n", physicalDeviceFeatures.sparseBinding ? "true" : "false");
  fprintf(stderr, "    sparseResidencyBuffer: %s\n", physicalDeviceFeatures.sparseResidencyBuffer ? "true" : "false");
  fprintf(stderr, "    sparseResidencyImage2D: %s\n", physicalDeviceFeatures.sparseResidencyImage2D ? "true" : "false");
  fprintf(stderr, "    sparseResidencyImage3D: %s\n", physicalDeviceFeatures.sparseResidencyImage3D ? "true" : "false");
  fprintf(stderr, "    sparseResidency2Samples: %s\n", physicalDeviceFeatures.sparseResidency2Samples ? "true" : "false");
  fprintf(stderr, "    sparseResidency4Samples: %s\n", physicalDeviceFeatures.sparseResidency4Samples ? "true" : "false");
  fprintf(stderr, "    sparseResidency8Samples: %s\n", physicalDeviceFeatures.sparseResidency8Samples ? "true" : "false");
  fprintf(stderr, "    sparseResidency16Samples: %s\n", physicalDeviceFeatures.sparseResidency16Samples ? "true" : "false");
  fprintf(stderr, "    sparseResidencyAliased: %s\n", physicalDeviceFeatures.sparseResidencyAliased ? "true" : "false");
  fprintf(stderr, "    variableMultisampleRate: %s\n", physicalDeviceFeatures.variableMultisampleRate ? "true" : "false");
  fprintf(stderr, "    inheritedQueries: %s\n", physicalDeviceFeatures.inheritedQueries ? "true" : "false");
  // clang-format on

  // get device extensions
  uint32_t extensionCount;
  vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &extensionCount, NULL);
  VkExtensionProperties availableDeviceExtensions[extensionCount];
  vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &extensionCount, availableDeviceExtensions);

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
  return physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ||
         physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
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

void PrintQueueFamilies() {
  uint32_t queueFamilyCount;
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
}

void PickPhysicalDevice() {
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

void CreateLogicalDevice() {
  int queueFamilyIndex = fstGraphicsQueueFamilyIndex();
  float queuePriority = 1.0f;
  VkDeviceQueueCreateInfo deviceQueueCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = queueFamilyIndex,
      .queueCount = 1,
      .pQueuePriorities = &queuePriority,
  };

  VkPhysicalDeviceFeatures deviceFeatures = {
      deviceFeatures.samplerAnisotropy = VK_TRUE,
  };

  VkDeviceCreateInfo deviceCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .queueCreateInfoCount = 1,
      .pQueueCreateInfos = &deviceQueueCreateInfo,
      .enabledExtensionCount = requiredDeviceExtensionsCount,
      .ppEnabledExtensionNames = requiredDeviceExtensions,
      .pEnabledFeatures = &deviceFeatures,
  };

  err = vkCreateDevice(physicalDevice, &deviceCreateInfo, NULL, &device);
  handleError();
  vkGetDeviceQueue(device, queueFamilyIndex, 0, &graphicsQueue);
}

void CreateSurface() {
  err = glfwCreateWindowSurface(instance, window, NULL, &surface);
  handleError();
}

VkImageView CreateImageView(VkImage image, VkFormat format) {
  VkImageViewCreateInfo viewInfo = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = format,
      .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .subresourceRange.baseMipLevel = 0,
      .subresourceRange.levelCount = 1,
      .subresourceRange.baseArrayLayer = 0,
      .subresourceRange.layerCount = 1,
  };

  VkImageView imageView;
  err = vkCreateImageView(device, &viewInfo, nullptr, &imageView);
  handleError();

  return imageView;
}

void createImageViews() {
  swapChainImageViews = malloc(swapChainImagesCount * sizeof(VkImageView));

  for (size_t i = 0; i < swapChainImagesCount; i++) {
    swapChainImageViews[i] = CreateImageView(swapChainImages[i], swapChainImageFormat);
  }
}

void CreateImageViews() {
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

void CreateRenderPass() {
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

  VkSubpassDependency dependency = {
      .srcSubpass = VK_SUBPASS_EXTERNAL,
      .dstSubpass = 0,
      .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      .srcAccessMask = 0,
      .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
  };

  VkRenderPassCreateInfo renderPassInfo = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .attachmentCount = 1,
      .pAttachments = &colorAttachment,
      .subpassCount = 1,
      .pSubpasses = &subpass,
      .dependencyCount = 1,
      .pDependencies = &dependency,
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

void CreateGraphicsPipeline() {
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
      .vertexBindingDescriptionCount = 1,
      .pVertexBindingDescriptions = getBindingDescription(),
      .vertexAttributeDescriptionCount = 3,
      .pVertexAttributeDescriptions = getAttributeDescriptions(),
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
      .setLayoutCount = 1,
      .pSetLayouts = &descriptorSetLayout,
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

void CreateFramebuffers() {
  swapChainFramebuffers = malloc(swapChainImagesCount * sizeof(VkFramebuffer));
  for (size_t i = 0; i < swapChainImagesCount; i++) {
    VkImageView attachments[] = {swapChainImageViews[i]};

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

void CreateCommandPool() {
  VkCommandPoolCreateInfo poolInfo = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
      .queueFamilyIndex = fstGraphicsQueueFamilyIndex(),
  };

  err = vkCreateCommandPool(device, &poolInfo, NULL, &commandPool);
  handleError();
}

void CreateCommandBuffers() {
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

// vkCmd...s
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

  VkBuffer vertexBuffers[] = {vertexBuffer};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
  vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, NULL);

  vkCmdDrawIndexed(commandBuffer, sizeof(indices) / sizeof(*indices), 1, 0, 0, 0);

  vkCmdEndRenderPass(commandBuffer);

  err = vkEndCommandBuffer(commandBuffer);
  handleError();
}

void CreateSyncObjects() {
  imageAvailableSemaphores = malloc(MAX_FRAMES_IN_FLIGHT * sizeof(VkSemaphore));
  finishedRenderingSemaphores = malloc(MAX_FRAMES_IN_FLIGHT * sizeof(VkSemaphore));
  inFlightFences = malloc(MAX_FRAMES_IN_FLIGHT * sizeof(VkFence));

  VkSemaphoreCreateInfo semaphoreInfo = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  };

  VkFenceCreateInfo fenceInfo = {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .flags = VK_FENCE_CREATE_SIGNALED_BIT, // the fence says "job is finished" as initial state
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

void CleanupSwapChain() {
  for (int i = 0; i < swapChainImagesCount; i++) {
    vkDestroyFramebuffer(device, swapChainFramebuffers[i], NULL);
  }
  for (int i = 0; i < swapChainImagesCount; i++) {
    vkDestroyImageView(device, swapChainImageViews[i], NULL);
  }

  vkDestroySwapchainKHR(device, swapChain, NULL);
}

void RecreateSwapChain() {
  // int width;
  // int height;
  // glfwGetFramebufferSize(window, &width, &height);
  vkDeviceWaitIdle(device);
  CleanupSwapChain();
  CreateSwapChain();
  CreateImageViews();
  CreateFramebuffers();
}

void UpdateUniformBuffer(uint32_t currentImage) {
  // running time
  struct timespec endTime;
  clock_gettime(CLOCK_MONOTONIC, &endTime);
  float diffTime = NANO * (endTime.tv_sec - startTime.tv_sec) + endTime.tv_nsec - startTime.tv_nsec;
  diffTime /= NANO;
  fprintf(stderr, "========================================\n");
  fprintf(stderr, "elapsed time = %f seconds\n", diffTime);
  fprintf(stderr, "========================================\n");

  // uniform buffer object
  UniformBufferObject ubo;
  // model
  mat4 model;
  glm_mat4_identity(model);
  vec3 v1 = {0.0f, 0.0f, 1.0f};
  glm_rotate(model, diffTime * glm_rad(90.0f), v1);

  // view
  vec3 v2 = {2.0f, 2.0f, 2.0f};
  vec3 v3 = {0.0f, 0.0f, 0.0f};
  vec3 v4 = {0.0f, 0.0f, 1.0f};
  mat4 view;
  glm_lookat(v2, v3, v4, (vec4 *)&view);

  // projection
  mat4 proj;
  glm_perspective(glm_rad(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f, (vec4 *)&proj);

  glm_mat4_print(model, stderr);
  glm_mat4_print(view, stderr);
  glm_mat4_print(proj, stderr);

  glm_mat4_copy(model, ubo.model);
  glm_mat4_copy(view, ubo.view);
  glm_mat4_copy(proj, ubo.proj);

  memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void drawFrame() {
  // wait for the previous frame to finish
  err = vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
  handleError();
  // acquire an image from the swap chain
  uint32_t imageIndex;
  err = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
  if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR) {
    RecreateSwapChain();
    return;
  }
  handleError();

  UpdateUniformBuffer(currentFrame);

  err = vkResetFences(device, 1, &inFlightFences[currentFrame]);
  handleError();

  // record command buffer which draws the scene onto acquired image
  err = vkResetCommandBuffer(commandBuffers[currentFrame], 0);
  handleError();
  recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

  VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
  VkSemaphore signalSemaphores[] = {finishedRenderingSemaphores[currentFrame]};

  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  VkSubmitInfo submitInfo = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = waitSemaphores,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = signalSemaphores, // will be signaled once the command buffers finished executing
      .pWaitDstStageMask = waitStages,
      .commandBufferCount = 1,
      .pCommandBuffers = &commandBuffers[currentFrame],
  };

  // submit recorded command buffer
  err = vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]);
  handleError();

  VkSwapchainKHR swapChains[] = {swapChain};
  VkPresentInfoKHR presentInfo = {
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = signalSemaphores,
      .swapchainCount = 1,
      .pSwapchains = swapChains,
      .pImageIndices = &imageIndex,
  };

  // present swap chain image
  err = vkQueuePresentKHR(graphicsQueue, &presentInfo);
  if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR) {
    RecreateSwapChain();
  } else {
    handleError();
  }
  currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void DeviceWaitIdle() { vkDeviceWaitIdle(device); };

void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                 VkImage *image, VkDeviceMemory *imageMemory) {

  VkImageCreateInfo imageInfo = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D,
      .extent.width = width,
      .extent.height = height,
      .extent.depth = 1,
      .mipLevels = 1,
      .arrayLayers = 1,
      .format = format,
      .tiling = tiling,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .usage = usage,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };

  err = vkCreateImage(device, &imageInfo, NULL, image);
  handleError();

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(device, *image, &memRequirements);

  VkMemoryAllocateInfo allocInfo = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memRequirements.size,
      .memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties),
  };

  err = vkAllocateMemory(device, &allocInfo, NULL, imageMemory);
  handleError();

  err = vkBindImageMemory(device, *image, *imageMemory, 0);
  handleError();
}

VkCommandBuffer beginSingleTimeCommands() {
  VkCommandBufferAllocateInfo allocInfo = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandPool = commandPool,
      .commandBufferCount = 1,
  };

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };

  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  return commandBuffer;
}

void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers = &commandBuffer,
  };

  vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(graphicsQueue);

  vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkBufferCopy copyRegion = {
      .size = size,
  };
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

  endSingleTimeCommands(commandBuffer);
}

void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  // image memory barrier (pipeline barrier)
  VkImageMemoryBarrier barrier = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .oldLayout = oldLayout,
      .newLayout = newLayout,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = image,
      .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .subresourceRange.baseMipLevel = 0,
      .subresourceRange.levelCount = 1,
      .subresourceRange.baseArrayLayer = 0,
      .subresourceRange.layerCount = 1,
  };

  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else {
    err = VKT_ERROR_UNSUPPORTED_LAYOUT_TRANSITION;
    handleError();
  }

  vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, NULL, 0, NULL, 1, &barrier);

  endSingleTimeCommands(commandBuffer);
}

void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkBufferImageCopy region = {
      .bufferOffset = 0,
      .bufferRowLength = 0,
      .bufferImageHeight = 0,
      .imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .imageSubresource.mipLevel = 0,
      .imageSubresource.baseArrayLayer = 0,
      .imageSubresource.layerCount = 1,
      .imageOffset = {0, 0, 0},
      .imageExtent = {width, height, 1},
  };

  vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  endSingleTimeCommands(commandBuffer);
}

void CreateTextureImage() {
  int texWidth, texHeight, texChannels;
  stbi_uc *pixels = stbi_load("textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
  VkDeviceSize imageSize = texWidth * texHeight * 4;
  fprintf(stderr, "image width: %d height: %d size: %lu\n", texWidth, texHeight, imageSize);

  if (!pixels) {
    err = VKT_ERROR_NO_TEXTURE_FILE;
    handleError();
  }

  // staging area
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  int bufferUsageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  int memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  CreateBuffer(imageSize, bufferUsageFlags, memoryProperties, &stagingBuffer, &stagingBufferMemory);

  // transfer image to mapped staging area
  void *data;
  vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
  memcpy(data, pixels, imageSize);
  vkUnmapMemory(device, stagingBufferMemory);

  // create device local image (VkImage)
  CreateImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &textureImage, &textureImageMemory);

  // transition layout and copy staging area to image
  VkImageLayout oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  VkImageLayout newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  TransitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, oldLayout, newLayout);

  CopyBufferToImage(stagingBuffer, textureImage, texWidth, texHeight);

  oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  TransitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, oldLayout, newLayout);

  // cleanup
  stbi_image_free(pixels);
  vkDestroyBuffer(device, stagingBuffer, NULL);
  vkFreeMemory(device, stagingBufferMemory, NULL);
}

void CreateTextureImageView() { textureImageView = CreateImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB); }

void CreateTextureSampler() {
  VkPhysicalDeviceProperties properties = {};
  vkGetPhysicalDeviceProperties(physicalDevice, &properties);

  VkSamplerCreateInfo samplerInfo = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .magFilter = VK_FILTER_LINEAR,
      .minFilter = VK_FILTER_LINEAR,
      .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .anisotropyEnable = VK_TRUE,
      .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
      .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
      .unnormalizedCoordinates = VK_FALSE,
      .compareEnable = VK_FALSE,
      .compareOp = VK_COMPARE_OP_ALWAYS,
      .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
  };

  err = vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler);
  handleError();
}

void cleanup() {
  CleanupSwapChain();
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(device, finishedRenderingSemaphores[i], NULL);
    vkDestroySemaphore(device, imageAvailableSemaphores[i], NULL);
    vkDestroyFence(device, inFlightFences[i], NULL);
  }
  vkDestroyCommandPool(device, commandPool, NULL);
  vkDestroyPipeline(device, graphicsPipeline, NULL);
  vkDestroyPipelineLayout(device, pipelineLayout, NULL);
  vkDestroyRenderPass(device, renderPass, NULL);
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroyBuffer(device, uniformBuffers[i], NULL);
    vkFreeMemory(device, uniformBuffersMemory[i], NULL);
  }
  vkDestroyDescriptorPool(device, descriptorPool, NULL);
  vkDestroySampler(device, textureSampler, NULL);
  vkDestroyImageView(device, textureImageView, NULL);
  vkDestroyImage(device, textureImage, NULL);
  vkFreeMemory(device, textureImageMemory, NULL);
  vkDestroyDescriptorSetLayout(device, descriptorSetLayout, NULL);
  vkDestroyBuffer(device, indexBuffer, NULL);
  vkFreeMemory(device, indexBufferMemory, NULL);
  vkDestroyBuffer(device, vertexBuffer, NULL);
  vkFreeMemory(device, vertexBufferMemory, NULL);
  vkDestroyDevice(device, NULL);
  destroyDebugUtilsMessenger(instance, debugMessenger, NULL);
  vkDestroySurfaceKHR(instance, surface, NULL);
  vkDestroyInstance(instance, NULL);
  glfwDestroyWindow(window);
  glfwTerminate();
}

void CreateIndexBuffer() {
  VkDeviceSize bufferSize = sizeof(indices);

  int usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  int memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  CreateBuffer(bufferSize, usageFlags, memoryProperties, &stagingBuffer, &stagingBufferMemory);

  void *data;
  vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, indices, bufferSize);
  vkUnmapMemory(device, stagingBufferMemory);

  usageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
  memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  CreateBuffer(bufferSize, usageFlags, memoryProperties, &indexBuffer, &indexBufferMemory);

  CopyBuffer(stagingBuffer, indexBuffer, bufferSize);

  vkDestroyBuffer(device, stagingBuffer, NULL);
  vkFreeMemory(device, stagingBufferMemory, NULL);
}

void initVulkan() {
  CreateInstance();
  CheckValidationLayerSupport();
  SetupDebugMessenger();
  CreateSurface();
  PickPhysicalDevice();
  PrintQueueFamilies();
  CreateLogicalDevice();
  CreateSwapChain();
  CreateImageViews();
  CreateRenderPass();
  CreateDescriptorSetLayout();
  CreateGraphicsPipeline();
  CreateFramebuffers();
  CreateCommandPool();
  CreateTextureImage();
  CreateTextureImageView();
  CreateTextureSampler();
  CreateVertexBuffer();
  CreateIndexBuffer();
  CreateUniformBuffers();
  CreateDescriptorPool();
  CreateDescriptorSets();
  CreateCommandBuffers();
  CreateSyncObjects();

  // set start time of program
  struct timespec ts1;
  clock_gettime(CLOCK_MONOTONIC, &startTime); /* mark start time */
}
