#include "vkTutorial.h"
#include <stdio.h>
#include <stdlib.h>

int err;

// prints error message and exits application
void _handleError(const char *fileName, int lineNumber) {
  if (err) {
    switch (err) {
    case VK_SUCCESS:
      fprintf(stderr, "Error: success\n");
      break;
    case VK_NOT_READY:
      fprintf(stderr, "Error: not ready\n");
      break;
    case VK_TIMEOUT:
      fprintf(stderr, "Error: timeout\n");
      break;
    case VK_EVENT_SET:
      fprintf(stderr, "Error: event set\n");
      break;
    case VK_EVENT_RESET:
      fprintf(stderr, "Error: event reset\n");
      break;
    case VK_INCOMPLETE:
      fprintf(stderr, "Error: incomplete\n");
      break;
    case VK_ERROR_OUT_OF_HOST_MEMORY:
      fprintf(stderr, "Error: out of host memory\n");
      break;
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
      fprintf(stderr, "Error: out of device memory\n");
      break;
    case VK_ERROR_INITIALIZATION_FAILED:
      fprintf(stderr, "Error: initialization failed\n");
      break;
    case VK_ERROR_DEVICE_LOST:
      fprintf(stderr, "Error: device lost\n");
      break;
    case VK_ERROR_MEMORY_MAP_FAILED:
      fprintf(stderr, "Error: memory map failed\n");
      break;
    case VK_ERROR_LAYER_NOT_PRESENT:
      fprintf(stderr, "Error: layer not present\n");
      break;
    case VK_ERROR_EXTENSION_NOT_PRESENT:
      fprintf(stderr, "Error: extension not present\n");
      break;
    case VK_ERROR_FEATURE_NOT_PRESENT:
      fprintf(stderr, "Error: feature not present\n");
      break;
    case VK_ERROR_INCOMPATIBLE_DRIVER:
      fprintf(stderr, "Error: incompatible driver\n");
      break;
    case VK_ERROR_TOO_MANY_OBJECTS:
      fprintf(stderr, "Error: too many objects\n");
      break;
    case VK_ERROR_FORMAT_NOT_SUPPORTED:
      fprintf(stderr, "Error: format not supported\n");
      break;
    case VK_ERROR_FRAGMENTED_POOL:
      fprintf(stderr, "Error: fragmented pool\n");
      break;
    case VK_ERROR_UNKNOWN:
      fprintf(stderr, "Error: unknown\n");
      break;
    case VK_ERROR_OUT_OF_POOL_MEMORY:
      fprintf(stderr, "Error: out of pool memory\n");
      break;
    case VK_ERROR_INVALID_EXTERNAL_HANDLE:
      fprintf(stderr, "Error: invalid external handle\n");
      break;
    case VK_ERROR_FRAGMENTATION:
      fprintf(stderr, "Error: fragmentation\n");
      break;
    case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
      fprintf(stderr, "Error: invalid opaque capture address\n");
      break;
    case VK_PIPELINE_COMPILE_REQUIRED:
      fprintf(stderr, "Error: pipeline compile required\n");
      break;
    case VK_ERROR_SURFACE_LOST_KHR:
      fprintf(stderr, "Error: surface lost khr\n");
      break;
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
      fprintf(stderr, "Error: native window in use khr\n");
      break;
    case VK_SUBOPTIMAL_KHR:
      fprintf(stderr, "Error: suboptimal khr\n");
      break;
    case VK_ERROR_OUT_OF_DATE_KHR:
      fprintf(stderr, "Error: out of date khr\n");
      break;
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
      fprintf(stderr, "Error: incompatible display khr\n");
      break;
    case VK_ERROR_VALIDATION_FAILED_EXT:
      fprintf(stderr, "Error: validation failed ext\n");
      break;
    case VK_ERROR_INVALID_SHADER_NV:
      fprintf(stderr, "Error: invalid shader nv\n");
      break;
    case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:
      fprintf(stderr, "Error: image usage not supported khr\n");
      break;
    case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:
      fprintf(stderr, "Error: video picture layout not supported khr\n");
      break;
    case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR:
      fprintf(stderr, "Error: video profile operation not supported khr\n");
      break;
    case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR:
      fprintf(stderr, "Error: video profile format not supported khr\n");
      break;
    case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR:
      fprintf(stderr, "Error: video profile codec not supported khr\n");
      break;
    case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR:
      fprintf(stderr, "Error: video std version not supported khr\n");
      break;
    case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
      fprintf(stderr, "Error: invalid drm format modifier plane layout ext\n");
      break;
    case VK_ERROR_NOT_PERMITTED_KHR:
      fprintf(stderr, "Error: not permitted khr\n");
      break;
    case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
      fprintf(stderr, "Error: full screen exclusive mode lost ext\n");
      break;
    case VK_THREAD_IDLE_KHR:
      fprintf(stderr, "Error: thread idle khr\n");
      break;
    case VK_THREAD_DONE_KHR:
      fprintf(stderr, "Error: thread done khr\n");
      break;
    case VK_OPERATION_DEFERRED_KHR:
      fprintf(stderr, "Error: operation deferred khr\n");
      break;
    case VK_OPERATION_NOT_DEFERRED_KHR:
      fprintf(stderr, "Error: operation not deferred khr\n");
      break;
    case VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR:
      fprintf(stderr, "Error: invalid video std parameters khr\n");
      break;
    case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
      fprintf(stderr, "Error: compression exhausted ext\n");
      break;
    case VK_ERROR_INCOMPATIBLE_SHADER_BINARY_EXT:
      fprintf(stderr, "Error: incompatible shader binary ext\n");
      break;
    case VK_RESULT_MAX_ENUM:
      fprintf(stderr, "Error: result max enum\n");
      break;
    case VKT_ERROR_VALIDATIONLAYER_NOT_AVAILABLE:
      fprintf(stderr, "Error: Validation layer not present\n");
      break;
    case VKT_ERROR_NO_VULKAN_DEVICE_AVAILABLE:
      fprintf(stderr, "Error: Failed to find GPUs with Vulkan support\n");
      break;
    case VKT_ERROR_NO_SUITABLE_GPU_AVAILABLE:
      fprintf(stderr, "Error: Failed to find a suitable GPU\n");
      break;
    case VKT_ERROR_DEVICE_EXTENSIONS_NOT_AVAILABLE:
      fprintf(stderr, "Error: Extension not present\n");
      break;
    case VKT_ERROR_SWAP_CHAIN_NOT_ADEQUATE:
      fprintf(stderr, "Error: Swap chain not adequate\n");
      break;
    case VKT_ERROR_NO_SUITABLE_MEMORY_AVAILABLE:
      fprintf(stderr, "Error: failed to find suitable memory\n");
      break;
    };
    fprintf(stderr, "in file %s, line %d, error code %d\n", fileName, lineNumber, err);
    exit(EXIT_FAILURE);
  };
}
