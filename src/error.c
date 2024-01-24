#include "vkTutorial.h"
#include <stdio.h>
#include <stdlib.h>

int err;

void handleError() {
  if (err) {
    switch (err) {
    case VK_SUCCESS:
      printf("Error: success\n");
      break;
    case VK_NOT_READY:
      printf("Error: not ready\n");
      break;
    case VK_TIMEOUT:
      printf("Error: timeout\n");
      break;
    case VK_EVENT_SET:
      printf("Error: event set\n");
      break;
    case VK_EVENT_RESET:
      printf("Error: event reset\n");
      break;
    case VK_INCOMPLETE:
      printf("Error: incomplete\n");
      break;
    case VK_ERROR_OUT_OF_HOST_MEMORY:
      printf("Error: out of host memory\n");
      break;
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
      printf("Error: out of device memory\n");
      break;
    case VK_ERROR_INITIALIZATION_FAILED:
      printf("Error: initialization failed\n");
      break;
    case VK_ERROR_DEVICE_LOST:
      printf("Error: device lost\n");
      break;
    case VK_ERROR_MEMORY_MAP_FAILED:
      printf("Error: memory map failed\n");
      break;
    case VK_ERROR_LAYER_NOT_PRESENT:
      printf("Error: layer not present\n");
      break;
    case VK_ERROR_EXTENSION_NOT_PRESENT:
      printf("Error: extension not present\n");
      break;
    case VK_ERROR_FEATURE_NOT_PRESENT:
      printf("Error: feature not present\n");
      break;
    case VK_ERROR_INCOMPATIBLE_DRIVER:
      printf("Error: incompatible driver\n");
      break;
    case VK_ERROR_TOO_MANY_OBJECTS:
      printf("Error: too many objects\n");
      break;
    case VK_ERROR_FORMAT_NOT_SUPPORTED:
      printf("Error: format not supported\n");
      break;
    case VK_ERROR_FRAGMENTED_POOL:
      printf("Error: fragmented pool\n");
      break;
    case VK_ERROR_UNKNOWN:
      printf("Error: unknown\n");
      break;
    case VK_ERROR_OUT_OF_POOL_MEMORY:
      printf("Error: out of pool memory\n");
      break;
    case VK_ERROR_INVALID_EXTERNAL_HANDLE:
      printf("Error: invalid external handle\n");
      break;
    case VK_ERROR_FRAGMENTATION:
      printf("Error: fragmentation\n");
      break;
    case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
      printf("Error: invalid opaque capture address\n");
      break;
    case VK_PIPELINE_COMPILE_REQUIRED:
      printf("Error: pipeline compile required\n");
      break;
    case VK_ERROR_SURFACE_LOST_KHR:
      printf("Error: surface lost khr\n");
      break;
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
      printf("Error: native window in use khr\n");
      break;
    case VK_SUBOPTIMAL_KHR:
      printf("Error: suboptimal khr\n");
      break;
    case VK_ERROR_OUT_OF_DATE_KHR:
      printf("Error: out of date khr\n");
      break;
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
      printf("Error: incompatible display khr\n");
      break;
    case VK_ERROR_VALIDATION_FAILED_EXT:
      printf("Error: validation failed ext\n");
      break;
    case VK_ERROR_INVALID_SHADER_NV:
      printf("Error: invalid shader nv\n");
      break;
    case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:
      printf("Error: image usage not supported khr\n");
      break;
    case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:
      printf("Error: video picture layout not supported khr\n");
      break;
    case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR:
      printf("Error: video profile operation not supported khr\n");
      break;
    case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR:
      printf("Error: video profile format not supported khr\n");
      break;
    case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR:
      printf("Error: video profile codec not supported khr\n");
      break;
    case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR:
      printf("Error: video std version not supported khr\n");
      break;
    case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
      printf("Error: invalid drm format modifier plane layout ext\n");
      break;
    case VK_ERROR_NOT_PERMITTED_KHR:
      printf("Error: not permitted khr\n");
      break;
    case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
      printf("Error: full screen exclusive mode lost ext\n");
      break;
    case VK_THREAD_IDLE_KHR:
      printf("Error: thread idle khr\n");
      break;
    case VK_THREAD_DONE_KHR:
      printf("Error: thread done khr\n");
      break;
    case VK_OPERATION_DEFERRED_KHR:
      printf("Error: operation deferred khr\n");
      break;
    case VK_OPERATION_NOT_DEFERRED_KHR:
      printf("Error: operation not deferred khr\n");
      break;
    case VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR:
      printf("Error: invalid video std parameters khr\n");
      break;
    case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
      printf("Error: compression exhausted ext\n");
      break;
    case VK_ERROR_INCOMPATIBLE_SHADER_BINARY_EXT:
      printf("Error: incompatible shader binary ext\n");
      break;
    case VK_RESULT_MAX_ENUM:
      printf("Error: result max enum\n");
      break;
    case VKT_ERROR_VALIDATIONLAYER_NOT_AVAILABLE:
      printf("Error: Validation layer not present\n");
      break;
    case VKT_ERROR_NO_VULKAN_DEVICE_AVAILABLE:
      printf("Error: Failed to find GPUs with Vulkan support\n");
      break;
    case VKT_ERROR_NO_SUITABLE_GPU_AVAILABLE:
      printf("Error: Failed to find a suitable GPU\n");
      break;
    case VKT_ERROR_DEVICE_EXTENSIONS_NOT_AVAILABLE:
      printf("Error: Extension not present\n");
      break;
    case VKT_ERROR_SWAP_CHAIN_NOT_ADEQUATE:
      printf("Error: Swap chain not adequate\n");
      break;
    };
    exit(EXIT_FAILURE);
  }
}
