#include "vkTutorial.h"

extern GLFWwindow *window;
extern VkInstance instance;
extern VkDebugUtilsMessengerEXT debugMessenger;

void cleanup() {
  destroyDebugUtilsMessenger(instance, debugMessenger, NULL);
  vkDestroyInstance(instance, NULL);
  glfwDestroyWindow(window);
  glfwTerminate();
}
