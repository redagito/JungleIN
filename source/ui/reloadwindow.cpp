#include "reloadwindow.h"
#include <iostream>

ReloadWindow::ReloadWindow() {
  this->density = SceneBuilder::NB_INSTANCE;
  this->spacing = Ground::MAP_MAX;
}

void ReloadWindow::reGenerate() {
  SceneBuilder::reloadGo(this->density, this->spacing);
}

void ReloadWindow::setDensity(int d) { this->density = d; }

void ReloadWindow::setSpacing(int s) { this->spacing = s; }

int ReloadWindow::getDensity() { return this->density; }

int ReloadWindow::getSpacing() { return this->spacing; }
