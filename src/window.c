#include "vkTutorial.h"
#include <stdio.h>
#include <stdlib.h>

// window size
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

extern const bool enableValidationLayers;
extern VkResult err;
extern VkInstance instance;
extern VkSurfaceKHR surface;

GLFWwindow *window;

// print error messages
static void error_callback(int error, const char *description) { debugPrint("GLFW error: %s\nError Code: %d\n", description, error); }

// pressing ESC or Q key closes window
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
  if ((key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q) && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
  }
}

void initGLFW() {
  // error handling
  glfwSetErrorCallback(error_callback);

  // init glfw
  if (!glfwInit()) {
    exit(EXIT_FAILURE);
  }

  // use Vullkan API (not OpenGL)
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  // create window
  window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Tutorial", nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  // key callback
  glfwSetKeyCallback(window, key_callback);
}

const char **getRequiredExtensions(uint32_t *requiredExtensionsCount) {
  // get required GLFW extensions
  const char **glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(requiredExtensionsCount);

  // print GLFW extensions
  debugPrint("GLFW:\n");
  debugPrint("  Extensions:\n");
  for (int i = 0; i < *requiredExtensionsCount; i++) {
    debugPrint("    %s\n", glfwExtensions[i]);
  }

  // setup required extensions (make space for VK_EXT_DEBUG_UTILS_EXTENSION_NAME)
  uint32_t addSize = 0;
  if (enableValidationLayers) {
    addSize++;
  };
  const char **requiredExtensions = malloc((*requiredExtensionsCount + addSize) * sizeof(char *));

  // copy GLFW extensions into required extensions
  for (int i = 0; i < *requiredExtensionsCount; i++) {
    requiredExtensions[i] = glfwExtensions[i];
  }

  // add debug extension as last element
  if (enableValidationLayers) {
    requiredExtensions[*requiredExtensionsCount] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    (*requiredExtensionsCount)++;
  }

  return requiredExtensions;
}

void CreateSurface() {
  err = glfwCreateWindowSurface(instance, window, nullptr, &surface);
  handleError();
}

void cleanupGLFW() {
  glfwDestroyWindow(window);
  glfwTerminate();
}

void mainloop() {
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    drawFrame();
  }
  DeviceWaitIdle();
}
