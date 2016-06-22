#include <iostream>

#include "ui/window.h"

int main(int argc, char *argv[]) {
  Window window;
  window.initializeGL();

  window.run();
  return 0;
}
