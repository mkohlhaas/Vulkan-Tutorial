#include "vkTutorial.h"

int main(void) {
  initGLFW();
  initVulkan();
  mainloop();
  cleanupVulkan();
  cleanupGLFW();
}
