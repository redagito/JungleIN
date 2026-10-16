#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "helpers/Utils.h"
#include "helpers/Profiler.h"
#include "helpers/SceneBuilder.h"
#include "objects/Scene.h"
#include "render/Renderer.h"
#include "render/LoadingScreen.h"
#include "render/DebugOverlay.h"
#include "audio/AudioClip.h"
#include "audio/AudioEngine.h"

#include <future>
#include <memory>
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
	void exShadows(bool active);
	void exWireframe(bool active);
	void exRegenerate();

	// Pure status-line builder for the overlay cheat-sheet (unit-tested).
	// Shows live toggle states plus the measurement readout (camera
	// position, view-ray ground distance, --- when no hit); movement
	// keys are deliberately excluded.
  static std::vector<std::string> overlayStatusLines(bool fxaa,
    bool flares, bool autoExp, float bias, bool vsync, bool godrays,
    bool bloom, bool distord, bool shadows, bool wireframe, float camX,
    float camY, float camZ, float lookDist, bool lookValid);

  // Profile line builder for the overlay (unit-tested): the topN
  // hottest sections with their mean ms, hottest first, five entries
  // per line. Empty stats produce no lines. Pure formatting.
  static std::vector<std::string> overlayProfileLines(
    const std::vector<Profiler::SectionStats>& stats, std::size_t topN);

	// Pure input policy, unit-tested: releasing the left button ends
	// hold-to-look, but must not kill F1-captured look (cursor disabled).
	static bool keepLookOnButtonRelease(int cursorMode);

	void playBtn(bool active);

	void onCursorMove(double x, double y);
	void onMouseButton(int button, int action, int mods);
	void onKeyPress(int key, int code, int action, int mods);

private:
	static Window* s_window;
	GLFWwindow* window = nullptr;

	// Owned subsystems (unique_ptr), freed explicitly in dependency order
	// in ~Window (renderer borrows scene, so scene dies last).
	std::unique_ptr<Scene> scene;
	std::unique_ptr<Renderer> renderer;
	std::unique_ptr<LoadingScreen> loading;
	std::unique_ptr<DebugOverlay> overlay;
	std::unique_ptr<AudioEngine> audio;
	// HDR Phase 3: last exposure bias sent to the composer (keys 7/8
	// step it multiplicatively from here; composer default is 1.0).
	float exposureBias = 1.0f;
	// Swap interval state: vsync on (1) by default, F4 toggles uncapped.
	bool vsyncOn = true;
	std::future<AudioClip> pendingClip;
	bool audioStarted = false;
};