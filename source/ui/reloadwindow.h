#ifndef RELOADWINDOW_H
#define RELOADWINDOW_H

#include "window.h"
#include "../helpers/scenebuilder.h"
#include "../objects/ground.h"

class ReloadWindow {

public:
  ReloadWindow();
  int getDensity();
  int getSpacing();

  void setDensity(int d);
  void setSpacing(int s);
  void reGenerate();

private:
  unsigned int density = 1000;
  unsigned int spacing = 20;
};

#endif // RELOADWINDOW_H
