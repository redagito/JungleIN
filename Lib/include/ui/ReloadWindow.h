#ifndef RELOADWINDOW_H
#define RELOADWINDOW_H

#include "ui/Window.h"
#include "helpers/SceneBuilder.h"
#include "objects/Ground.h"

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
