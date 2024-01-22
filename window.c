#include "vkTutorial.h"
#include <stdio.h>
#include <stdlib.h>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

GLFWwindow *window;

// print error messages
static void error_callback(int error, const char *description) {
  fprintf(stderr, "Error: %s\nError Code: %d\n", description, error);
}

// pressing ESC key closes window
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void initWindow() {
  // error handling
  glfwSetErrorCallback(error_callback);

  // init glfw
  if (!glfwInit()) {
    exit(EXIT_FAILURE);
  }

  // create window
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  window = glfwCreateWindow(WIDTH, HEIGHT, "", NULL, NULL);
  if (!window) {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  // key callback
  glfwSetKeyCallback(window, key_callback);
}

void mainloop() {
  // loop until ESC key is pressed
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }
}
