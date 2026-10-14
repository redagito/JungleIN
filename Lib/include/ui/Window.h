#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "helpers/Utils.h"
#include "helpers/SceneBuilder.h"
#include "objects/Scene.h"
#include "render/Renderer.h"
#include "ui/ReloadWindow.h"

/*!
 * Window extends QGLWidget
 * OpenGL display widget to be integrated into the QMainWindow later.
 */
class Window
{

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

	void onCursorMove(double x, double y);
	void onMouseButton(int button, int action, int mods);
	void onKeyPress(int key, int code, int action, int mods);

private:
	//  void timerEvent(QTimerEvent *);

	static Window* s_window;
	GLFWwindow* window = nullptr;

	Scene* scene = nullptr;
	Renderer* renderer = nullptr;
};