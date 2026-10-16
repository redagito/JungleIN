#include "ui/ReloadWindow.h"
#include "helpers/Logger.h"

#include <stdexcept>

ReloadWindow::ReloadWindow() {
  this->density = SceneBuilder::NB_INSTANCE;
  // ground2 is null before SceneBuilder::build (e.g. standalone tests):
  // fall back to the Ground default instead of dereferencing null.
  this->spacing = SceneBuilder::ground2 != nullptr
      ? static_cast<unsigned int>(SceneBuilder::ground2->getMapMax())
      : static_cast<unsigned int>(Ground::kDefaultMapMax);
}

void ReloadWindow::reGenerate() {
  SceneBuilder::reloadGo(this->density, this->spacing);
}

void ReloadWindow::setDensity(int d) {
  if (d < 0)
    throw std::runtime_error("ReloadWindow density must not be negative");
  this->density = static_cast<unsigned int>(d);
}

void ReloadWindow::setSpacing(int s) {
  if (s < 0)
    throw std::runtime_error("ReloadWindow spacing must not be negative");
  this->spacing = static_cast<unsigned int>(s);
}

int ReloadWindow::getDensity() { return this->density; }

int ReloadWindow::getSpacing() { return this->spacing; }
