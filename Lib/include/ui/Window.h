#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "helpers/Utils.h"
#include "helpers/SceneBuilder.h"
#include "objects/Scene.h"
#include "render/Renderer.h"
#include "render/LoadingScreen.h"
#include "render/DebugOverlay.h"
#include "audio/AudioClip.h"
#include "audio/AudioEngine.h"
#include "ui/ReloadWindow.h"

#include <future>
#include <string>
#include <vector>

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
	void exPostAutoExposure(bool active);
	void exExposureBias(float value);
	void exFlareDisp(int value);
	void exFlareIntensity(int value);
	void exSunSpeed(int value);
	void exGamma(int value);
	void exAnisotropy(bool active);
	void exShadowCascade(bool active);
	void exRenderRatio(int value);
	void exDebugOverlay(bool active);
	void exVSync(bool active);

	// Pure status-line builder for the overlay cheat-sheet (unit-tested).
	// Shows live toggle states; movement keys are deliberately excluded.
	static std::vector<std::string> overlayStatusLines(bool fxaa,
		bool flares, bool autoExp, float bias, bool vsync);

	// Pure input policy, unit-tested: releasing the left button ends
	// hold-to-look, but must not kill F1-captured look (cursor disabled).
	static bool keepLookOnButtonRelease(int cursorMode);

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
	LoadingScreen* loading = nullptr;
	DebugOverlay* overlay = nullptr;
	AudioEngine* audio = nullptr;
	// HDR Phase 3: last exposure bias sent to the composer (keys 7/8
	// step it multiplicatively from here; composer default is 1.0).
	float exposureBias = 1.0f;
	// Swap interval state: vsync on (1) by default, F4 toggles uncapped.
	bool vsyncOn = true;
	std::future<AudioClip> pendingClip;
	bool audioStarted = false;
};