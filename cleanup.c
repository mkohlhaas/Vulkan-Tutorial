#include "vkTutorial.h"

extern GLFWwindow *window;
extern VkInstance instance;

void cleanup() {
  vkDestroyInstance(instance, NULL);
  glfwDestroyWindow(window);
  glfwTerminate();
}
