#include "vkTutorial.h"

void cleanup() {
  GLFWwindow *window = getWindow();
  glfwDestroyWindow(window);
  glfwTerminate();
}
