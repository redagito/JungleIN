#ifndef WINDOW_H
#define WINDOW_H

#include <flextGL.h>

#include <iostream>

#include "helpers/utils.h"
#include "helpers/scenebuilder.h"
#include "objects/scene.h"
#include "render/renderer.h"
#include "reloadwindow.h"

/*!
 * Window extends QGLWidget
 * Widget d'affichage opengl qui sera int�gr� au QMainWindow plutard.
 */
class Window {

public:
  Window();
  ~Window();

  void run();

  //  void keyPressEvent(QKeyEvent *event);
  //  void keyReleaseEvent(QKeyEvent *event);
  //
  //  void mousePressEvent(QMouseEvent *);
  //  void mouseReleaseEvent(QMouseEvent *);

  void initializeGL();
  void initializeObjects();
  void paintGL();
  void resizeGL(int w, int h);

  void exResetCamera();
  void exQuality(int value);
  void exPostFXAA(bool active);
  void exPostLenseFlares(bool active);
  void exPostDistord(bool active);
  void exPostGodrays(bool active);
  void exPostBloom(bool active);
  void exFlareDisp(int value);
  void exFlareIntensity(int value);
  void exSunSpeed(int value);
  void exGamma(int value);
  void exAnisotropy(bool active);
  void exShadowCascade(bool active);
  void exRenderRatio(int value);
  void playBtn(bool active);
  void reGenerate();

private:
  //  void timerEvent(QTimerEvent *);

  Scene *scene;
  Renderer *renderer;
};

#endif // WINDOW_H
