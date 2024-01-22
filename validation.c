#include "vkTutorial.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <vulkan/vulkan_core.h>

extern VkResult err;

const bool enableValidationLayers = true;

// required validation layers
const char *requiredValidationLayers[] = {"VK_LAYER_KHRONOS_validation"};
int numRequiredValidationLayers = sizeof(requiredValidationLayers) / sizeof(char *);

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
  for (int i = 0; i < numRequiredValidationLayers; i++) {
    for (int j = 0; j < layerCount; j++) {
      if (!strcmp(requiredValidationLayers[i], availableLayers[j].layerName)) {
        found++;
        break;
      }
    }
  }
  bool allLayersAvailable = found == numRequiredValidationLayers;
  err = allLayersAvailable ? VK_SUCCESS : VKT_ERROR_VALIDATIONLAYER_NOT_PRESENT;
  handleError();

  printf("\nAll validation layers are present: true\n");

  return allLayersAvailable;
}
