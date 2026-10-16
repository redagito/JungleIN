#include "ui/Window.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <sstream>
#include <stdexcept>

#include "helpers/Logger.h"
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_inverse.hpp>

#include <algorithm>

#include "helpers/Utils.h"
#include "helpers/Profiler.h"

namespace {
constexpr int kDefaultWidth = 1600;
constexpr int kDefaultHeight = 900;

// Overlay ruler: distance from the camera to the ground along the view
// ray (exponential march for a bracket, then bisection on the exact
// plane math). Overlay-only cost. nullopt looking level/up, off-map, or
// on any ground query failure. Forward comes straight from the view
// matrix so no yaw/pitch sign convention can sneak in.
std::optional<float> measureGroundDistance(Camera *camera, Ground *ground) {
  if (camera == nullptr || ground == nullptr)
    return std::nullopt;
  const glm::vec3 pos = camera->getPosition();
  const glm::vec3 fwd = glm::normalize(glm::vec3(
    glm::inverse(camera->getMatrix()) * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));
  if (!std::isfinite(fwd.x + fwd.y + fwd.z) || fwd.y >= -0.001f)
    return std::nullopt;
  auto heightAbove = [&](float t) -> std::optional<float> {
    try {
      return (pos.y + fwd.y * t) -
        ground->getY(pos.x + fwd.x * t, pos.z + fwd.z * t);
    }
    catch (...) {
      return std::nullopt;
    }
  };
  const std::optional<float> h0 = heightAbove(0.0f);
  if (!h0 || *h0 <= 0.0f)
    return std::nullopt;
  float lo = 0.0f;
  float hi = 0.5f;
  while (hi <= 600.0f) {
    const std::optional<float> h = heightAbove(hi);
    if (!h)
      return std::nullopt;
    if (*h <= 0.0f)
      break;
    lo = hi;
    hi *= 2.0f;
  }
  if (hi > 600.0f)
    return std::nullopt;
  for (int i = 0; i < 24; ++i) {
    const float mid = (lo + hi) * 0.5f;
    const std::optional<float> h = heightAbove(mid);
    if (!h)
      return std::nullopt;
    if (*h <= 0.0f)
      hi = mid;
    else
      lo = mid;
  }
  return (lo + hi) * 0.5f;
}
} // namespace

static void APIENTRY glDebugOutput(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei /*length*/,
	const GLchar* message, const void* /*userParam*/)
{
	// ignore non-significant error/warning codes
	if (id == 131169 || id == 131185 || id == 131218 || id == 131204)
		return;

	std::stringstream ss;
	ss << "Debug message (" << id << "): " << message << std::endl;

	switch (source)
	{
	case GL_DEBUG_SOURCE_API:
		ss << "Source: API";
		break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
		ss << "Source: Window System";
		break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER:
		ss << "Source: Shader Compiler";
		break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:
		ss << "Source: Third Party";
		break;
	case GL_DEBUG_SOURCE_APPLICATION:
		ss << "Source: Application";
		break;
	case GL_DEBUG_SOURCE_OTHER:
		ss << "Source: Other";
		break;
	}
	ss << std::endl;

	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:
		ss << "Type: Error";
		break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		ss << "Type: Deprecated Behaviour";
		break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		ss << "Type: Undefined Behaviour";
		break;
	case GL_DEBUG_TYPE_PORTABILITY:
		ss << "Type: Portability";
		break;
	case GL_DEBUG_TYPE_PERFORMANCE:
		ss << "Type: Performance";
		break;
	case GL_DEBUG_TYPE_MARKER:
		ss << "Type: Marker";
		break;
	case GL_DEBUG_TYPE_PUSH_GROUP:
		ss << "Type: Push Group";
		break;
	case GL_DEBUG_TYPE_POP_GROUP:
		ss << "Type: Pop Group";
		break;
	case GL_DEBUG_TYPE_OTHER:
		ss << "Type: Other";
		break;
	}
	ss << std::endl;

	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:
		ss << "Severity: high";
		break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		ss << "Severity: medium";
		break;
	case GL_DEBUG_SEVERITY_LOW:
		ss << "Severity: low";
		break;
	case GL_DEBUG_SEVERITY_NOTIFICATION:
		ss << "Severity: notification";

		break;
	}
	if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
		return;

	// Real GL errors fail fast so the global handler in main logs them.
	// Anything thrown here unwinds through the calling GL command.
	if (type == GL_DEBUG_TYPE_ERROR)
		throw std::runtime_error(ss.str());

	logi("{}", ss.str());
}

Window* Window::s_window = nullptr;

Window::Window() {
	if (s_window != nullptr)
		throw std::runtime_error("Already has a window");

	s_window = this;
}

Window::~Window() {
	// GL teardown must run while the context is still current: free every
	// owner in dependency order first (renderer borrows scene, so scene
	// dies last), then the shared shaders, then GLFW.
	audio.reset();
	loading.reset();
	overlay.reset();
	renderer.reset();
	scene.reset();
	ShaderLibrary::clear();
	if (window != nullptr) {
		glfwDestroyWindow(window);
		window = nullptr;
		Utils::window.store(nullptr);
	}
	if (s_window == this)
		s_window = nullptr;
	glfwTerminate();
}

void Window::run() {
	using clock = std::chrono::steady_clock;
	auto last = clock::now();
	while (glfwWindowShouldClose(Utils::window.load()) == 0) {
		glfwPollEvents();

		if (audio != nullptr) {
			if (!audioStarted && pendingClip.valid() &&
				pendingClip.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
				audio->playLoop(pendingClip.get());
				audioStarted = true;
				logi("Ambient loop started");
			}
			audio->update();
		}

		paintGL();

		// Real frame dt drives the overlay FPS EMA and the profiler ring.
		const auto now = clock::now();
		const double dtSeconds =
			std::chrono::duration<double>(now - last).count();
		last = now;
		Profiler::recordFrame(dtSeconds * 1000.0);
		if (overlay != nullptr) {
			Profiler::Scope overlayScope("frame.overlay");
			overlay->update(dtSeconds);
			// Measurement readout, overlay-visible only (the ray march is
			// cheap but pointless when nobody reads it).
			float camX = 0.0f;
			float camY = 0.0f;
			float camZ = 0.0f;
			float lookDist = 0.0f;
			bool lookValid = false;
			if (overlay->isVisible() && scene != nullptr &&
				scene->getCurrentCamera() != nullptr) {
				const glm::vec3 pos =
					scene->getCurrentCamera()->getPosition();
				camX = pos.x;
				camY = pos.y;
				camZ = pos.z;
				const std::optional<float> hit = measureGroundDistance(
					scene->getCurrentCamera(), scene->getGround());
				if (hit) {
					lookDist = *hit;
					lookValid = true;
				}
			}
			std::vector<std::string> lines = overlayStatusLines(
				renderer->getPostComposer()->isFXAA(),
				renderer->getPostComposer()->isLenseFlares(),
				renderer->getPostComposer()->isAutoExposure(),
				exposureBias, vsyncOn,
				renderer->getPostComposer()->isGodrays(),
				renderer->getPostComposer()->isBloom(),
				renderer->getPostComposer()->isDistord(),
				renderer->isShadows(), renderer->isWireframe(), camX, camY,
				camZ, lookDist, lookValid);
			if (overlay->isVisible()) {
				// Section means only: computed on demand for the visible
				// overlay, never logged per frame.
				const std::vector<std::string> profile =
					overlayProfileLines(Profiler::sectionStats(), 10);
				lines.insert(lines.end(), profile.begin(), profile.end());
			}
			overlay->setLines(lines);
			int fbWidth = 0;
			int fbHeight = 0;
			glfwGetFramebufferSize(Utils::window.load(), &fbWidth, &fbHeight);
			overlay->draw(fbWidth, fbHeight);
		}

		{
			// Swap blocks on vsync: a fat swap section means
			// GPU/vsync-bound, not CPU-bound.
			Profiler::Scope scope("frame.swap");
			glfwSwapBuffers(Utils::window.load());
		}

		static bool firstFrame = true;
		if (firstFrame) {
			firstFrame = false;
			Profiler::mark("first frame presented");
		}
	}
	// Dump-on-close: one CSV next to the working dir (same place as log.txt).
	if (!Profiler::dumpFrameStats("profiling.csv"))
		loge("Failed to write profiling.csv");
	if (!Profiler::dumpSectionStats("profiling_sections.csv"))
		loge("Failed to write profiling_sections.csv");
}

void Window::initializeGL() {
	Profiler::mark("initializeGL start");
	if (!glfwInit())
	{
		throw std::runtime_error{"Failed to initialize GLFW."};
	}
	Profiler::mark("GLFW initialized");

	glfwSetErrorCallback([](int code, const char* error) {
		loge("GLFW error {}: {}", code, error != nullptr ? error : "(null)");
		std::abort();
	});
	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_FALSE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

	// Create window from parameters
	window = glfwCreateWindow(kDefaultWidth, kDefaultHeight, "JungleIN", nullptr, nullptr);
	if (window == nullptr) {
		glfwTerminate();
		throw std::runtime_error{"Failed to create GLFW window."};
	}
	Profiler::mark("window created");

	glfwMakeContextCurrent(window);

	// Load opengl functions
	if (gladLoadGL((GLADloadfunc)glfwGetProcAddress) == 0)
	{
		throw std::runtime_error{"Failed to init GL context"};
	}
	Profiler::mark("GL functions loaded");

	// Black splash until the first jungle frame is ready.
	loading = std::make_unique<LoadingScreen>(window);
	loading->show("data/splash.png");

	// Debug context
	int flags;
	glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
	if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
	{
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(glDebugOutput, nullptr);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
	}

	// 
	Utils::window.store(window);

	// User input
	// Key callback
	glfwSetKeyCallback(window,
		[](GLFWwindow* /*w*/, int key, int code, int action, int mods)
		{
			if (Window::s_window == nullptr)
				return;
			Window::s_window->onKeyPress(key, code, action, mods);
		});

	// Mouse move callback
	glfwSetCursorPosCallback(window,
		[](GLFWwindow* /*w*/, double x, double y)
		{
			if (Window::s_window == nullptr)
				return;
			Window::s_window->onCursorMove(x, y);
		});

	// Mouse button callback (press-and-hold to look around)
	glfwSetMouseButtonCallback(window,
		[](GLFWwindow* /*w*/, int button, int action, int mods)
		{
			if (Window::s_window == nullptr)
				return;
			Window::s_window->onMouseButton(button, action, mods);
		});

	// Window resize callback (aspect, FBOs and shadow maps follow).
	// Renders a frame synchronously so resizing stays live while the
	// window edge is dragged; exceptions must not escape into GLFW.
	glfwSetFramebufferSizeCallback(window,
		[](GLFWwindow* /*w*/, int width, int height)
		{
			if (Window::s_window == nullptr || width <= 0 || height <= 0)
				return;
			if (Window::s_window->renderer == nullptr)
				return;
			try {
				Window::s_window->resizeGL(width, height);
				Window::s_window->paintGL();
				glfwSwapBuffers(Window::s_window->window);
			}
			catch (const std::exception& e) {
				// Cannot throw through the GLFW callback: log and shut
				// down cleanly instead of presenting a half-resized chain.
				loge("Resize render failed: {}", e.what());
				glfwSetWindowShouldClose(Window::s_window->window,
					GLFW_TRUE);
			}
		});

	glfwShowWindow(window);

	// glGetString(GL_EXTENSIONS) is removed on core profiles; probe with
	// glGetStringi instead. Only query the driver maximum when the
	// extension is actually present, so unsupported drivers never raise a
	// GL error here (the debug callback throws on those).
	Utils::USE_ANISO.store(false);
	Utils::MAX_ANISO.store(1.0f);
	GLint extensionCount = 0;
	glGetIntegerv(GL_NUM_EXTENSIONS, &extensionCount);
	for (GLint i = 0; i < extensionCount; ++i) {
		const char* extension = reinterpret_cast<const char*>(
			glGetStringi(GL_EXTENSIONS, static_cast<GLuint>(i)));
		if (extension != nullptr &&
			std::strcmp(extension, "GL_EXT_texture_filter_anisotropic") == 0) {
			GLfloat maxAniso = 1.0f;
			glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
			if (maxAniso >= 2.0f) {
				Utils::USE_ANISO.store(true);
				Utils::MAX_ANISO.store(maxAniso);
			}
			break;
		}
	}

	initializeObjects();
}

void Window::initializeObjects() {
	Profiler::mark("initializeObjects start");
	renderer = std::make_unique<Renderer>();

	renderer->loadShaders();
	Profiler::mark("shaders loaded");

	// Debug overlay stays hidden until F3; its atlas texture uploads
	// lazily on the first visible draw. Status lines refresh every
	// frame in run() so toggle states are always live.
	overlay = std::make_unique<DebugOverlay>();

	scene = SceneBuilder::build();
	Profiler::mark("scene built");

	scene->init();
	Profiler::mark("scene VBOs initialized");

	scene->setRenderAble(true);


	renderer->init(scene.get(), kDefaultWidth, kDefaultHeight);

	renderer->start();
	Profiler::mark("renderer started");

	audio = std::make_unique<AudioEngine>();
	audio->init();
	// Decode off the main thread: a full MP3 would freeze startup.
	pendingClip = std::async(std::launch::async, AudioClip::decodeMP3,
		std::string("data/resources/sounds/ambiant.mp3"));
	audioStarted = false;

}

void Window::paintGL() { renderer->render(); }

void Window::resizeGL(int w, int h) {
	// Iconified windows report zero area; there is nothing to resize to.
	if (renderer == nullptr || w <= 0 || h <= 0)
		return;
	renderer->resize(w, h);
}

void Window::exQuality(int value) {
	Utils::QUALITY.store(value);
	renderer->getPostComposer()->resizeHalfBuffers();

}

void Window::exResetCamera() { scene->getCurrentCamera()->reset(); }

void Window::exPostFXAA(bool active) {
	renderer->getPostComposer()->toggleFXAA(active);
}

void Window::exPostDistord(bool active) {
	renderer->getPostComposer()->toggleDistord(active);
}

void Window::exPostLenseFlares(bool active) {
	renderer->getPostComposer()->toggleLenseFlares(active);
}

void Window::exPostGodrays(bool active) {
	renderer->getPostComposer()->toggleGodrays(active);
}

void Window::exPostBloom(bool active) {
	renderer->getPostComposer()->toggleBloom(active);
}

// HDR Phase 3: auto-exposure switch and manual bias, mirroring the
// other exPost toggles. Bias range 0.25-4.0 matches the composer
// clamp headroom (exposure itself clamps to 1e-3..10).
void Window::exPostAutoExposure(bool active) {
	renderer->getPostComposer()->toggleAutoExposure(active);
}

void Window::exExposureBias(float value) {
	exposureBias = std::clamp(value, 0.25f, 4.0f);
	renderer->getPostComposer()->setExposureBias(exposureBias);
}

void Window::exFlareDisp(int value) {
	renderer->getPostComposer()->setFlareDispersal(value / 100.0f);
}

void Window::exFlareIntensity(int value) {
	renderer->getPostComposer()->setFlareIntensity(value / 100.0f);
}

void Window::exSunSpeed(int value) { scene->getSun()->setSpeed(value / 10.0f); }

void Window::exGamma(int value) {
	renderer->getPostComposer()->setGamma(value / 100.0f);
}

void Window::exAnisotropy(bool active) {
	Utils::USE_ANISO.store(active);
	Texture::needsUpdate.store(true);
}

void Window::exShadowCascade(bool active) {
	renderer->toggleShadowCascade(active);
}

void Window::exRenderRatio(int value) { renderer->setRatio(value / 10.0f); }

void Window::exDebugOverlay(bool active) {
	if (overlay != nullptr)
		overlay->setVisible(active);
}

void Window::exVSync(bool active) {
	// Swap interval applies to this context on call; uncapped (0) is for
	// profiling runs, vsync (1) is the default.
	vsyncOn = active;
	glfwSwapInterval(active ? 1 : 0);
}

void Window::exShadows(bool active) {
	renderer->toggleShadows(active);
}

void Window::exWireframe(bool active) {
	renderer->toggleWireframe(active);
}

std::vector<std::string> Window::overlayStatusLines(bool fxaa, bool flares,
	bool autoExp, float bias, bool vsync, bool godrays, bool bloom,
	bool distord, bool shadows, bool wireframe, float camX, float camY,
	float camZ, float lookDist, bool lookValid) {
	auto onOff = [](bool v) { return v ? "on" : "off"; };
	char biasText[32];
	std::snprintf(biasText, sizeof(biasText), "%.2f", bias);
	char camText[64];
	std::snprintf(camText, sizeof(camText), "Cam %.1f %.1f %.1f", camX,
		camY, camZ);
	char lookText[32];
	if (lookValid)
		std::snprintf(lookText, sizeof(lookText), "Look %.1fm", lookDist);
	else
		std::snprintf(lookText, sizeof(lookText), "Look ---");
	return {
		std::string("1 FXAA ") + onOff(fxaa) +
			"  2 Flares " + onOff(flares),
		std::string("3 AutoExp ") + onOff(autoExp) +
			"  7/8 Bias " + biasText,
		std::string("4 Rays ") + onOff(godrays) +
			"  5 Bloom " + onOff(bloom) +
			"  6 Dist " + onOff(distord),
		std::string("F1 Mouse  F3 Overlay  F4 VSync ") + onOff(vsync),
		std::string("R Regen  9 Shad ") + onOff(shadows) +
			"  0 Wire " + onOff(wireframe),
		camText,
		lookText,
	};
}

void Window::playBtn(bool /*active*/) {
}

std::vector<std::string> Window::overlayProfileLines(
  const std::vector<Profiler::SectionStats>& stats, std::size_t topN) {
  if (stats.empty() || topN == 0)
    return {};
  // Up to kPerLine entries per line: ten sections on one line would run
  // off the screen, and one line per section would flood the overlay.
  constexpr std::size_t kPerLine = 5;
  std::vector<std::string> lines;
  std::string line;
  std::size_t shown = 0;
  std::size_t onLine = 0;
  for (const Profiler::SectionStats& s : stats) {
    if (shown >= topN)
      break;
    char cell[64];
    std::snprintf(cell, sizeof(cell), "%s %.1fms", s.name.c_str(), s.mean);
    if (onLine > 0)
      line += "  ";
    line += cell;
    ++shown;
    ++onLine;
    if (onLine == kPerLine) {
      lines.push_back(line);
      line.clear();
      onLine = 0;
    }
  }
  if (!line.empty())
    lines.push_back(line);
  return lines;
}

void Window::exRegenerate() {
	// Rescatter vegetation with the build defaults: ReloadWindow (and its
	// settings dialog) is gone, so regeneration reuses the current map
	// bounds. Regenerating before the scene exists is a bug: fail fast.
	SceneBuilder::regenerateFromCurrentBounds();
}

void Window::onCursorMove(double x, double y)
{
	if (scene == nullptr)
		return;
	scene->getCurrentCamera()->setMouse(static_cast<int>(std::floor(x)), static_cast<int>(std::floor(y)));
}

void Window::onMouseButton(int button, int action, int /*mods*/)
{
	if (scene == nullptr)
		return;
	if (button != GLFW_MOUSE_BUTTON_LEFT)
		return;
	if (action == GLFW_PRESS) {
		scene->getCurrentCamera()->activeLook(true);
		return;
	}
	// Release: hold-to-look ends, but F1-captured look (cursor disabled)
	// stays on. Unconditionally clearing here is what stranded the view
	// after every click-drag under capture until the next press.
	if (!keepLookOnButtonRelease(
		glfwGetInputMode(window, GLFW_CURSOR)))
		scene->getCurrentCamera()->activeLook(false);
}

bool Window::keepLookOnButtonRelease(int cursorMode) {
	return cursorMode == GLFW_CURSOR_DISABLED;
}

void Window::onKeyPress(int key, int /*code*/, int action, int /*mods*/)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_ESCAPE:
			glfwSetWindowShouldClose(window, GLFW_TRUE);
			break;

			// Movement
		case GLFW_KEY_UP:
		case GLFW_KEY_W:
			scene->getCurrentCamera()->setUpKey(true);
			break;
		case GLFW_KEY_DOWN:
		case GLFW_KEY_S:
			scene->getCurrentCamera()->setDownKey(true);
			break;
		case GLFW_KEY_LEFT:
		case GLFW_KEY_A:
			scene->getCurrentCamera()->setLeftKey(true);
			break;
		case GLFW_KEY_RIGHT:
		case GLFW_KEY_D:
			scene->getCurrentCamera()->setRightKey(true);
			break;
		case GLFW_KEY_LEFT_SHIFT:
		case GLFW_KEY_RIGHT_SHIFT:
			scene->getCurrentCamera()->setSprintKey(true);
			break;

			// Movement up / down
		case GLFW_KEY_E:
			scene->getCurrentCamera()->setEKey(true);
			break;
		case GLFW_KEY_Q:
			scene->getCurrentCamera()->setQKey(true);
			break;

			// Features: single-key toggles (multi-state settings rotate on
		// one key when they appear; no on/off key pairs).
		case GLFW_KEY_1: {
			PostComposer* composer = renderer->getPostComposer();
			exPostFXAA(!composer->isFXAA());
			break;
		}
		case GLFW_KEY_2: {
			PostComposer* composer = renderer->getPostComposer();
			exPostLenseFlares(!composer->isLenseFlares());
			break;
		}
		case GLFW_KEY_3: {
			PostComposer* composer = renderer->getPostComposer();
			exPostAutoExposure(!composer->isAutoExposure());
			break;
		}
		case GLFW_KEY_4: {
			PostComposer* composer = renderer->getPostComposer();
			exPostGodrays(!composer->isGodrays());
			break;
		}
		case GLFW_KEY_5: {
			PostComposer* composer = renderer->getPostComposer();
			exPostBloom(!composer->isBloom());
			break;
		}
		case GLFW_KEY_6: {
			PostComposer* composer = renderer->getPostComposer();
			exPostDistord(!composer->isDistord());
			break;
		}
		case GLFW_KEY_7:
			exExposureBias(exposureBias * 0.8f);
			break;
		case GLFW_KEY_8:
			exExposureBias(exposureBias * 1.25f);
			break;
		case GLFW_KEY_9:
			exShadows(!renderer->isShadows());
			break;
		case GLFW_KEY_0:
			exWireframe(!renderer->isWireframe());
			break;
		case GLFW_KEY_R:
			exRegenerate();
			break;

			// Mouse capture: F1 toggles (no F1/F2 pair).
		case GLFW_KEY_F1:
			if (glfwGetInputMode(window, GLFW_CURSOR) ==
				GLFW_CURSOR_DISABLED) {
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				scene->getCurrentCamera()->activeLook(false);
			}
			else {
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				scene->getCurrentCamera()->activeLook(true);
			}
			break;
		case GLFW_KEY_F3:
			exDebugOverlay(overlay == nullptr || !overlay->isVisible());
			break;
		case GLFW_KEY_F4:
			exVSync(!vsyncOn);
			break;
		default:
			break;
		}
	}
	else if (action == GLFW_RELEASE)
	{
		switch (key)
		{
		case GLFW_KEY_UP:
		case GLFW_KEY_W:
			scene->getCurrentCamera()->setUpKey(false);
			break;
		case GLFW_KEY_DOWN:
		case GLFW_KEY_S:
			scene->getCurrentCamera()->setDownKey(false);
			break;
		case GLFW_KEY_LEFT:
		case GLFW_KEY_A:
			scene->getCurrentCamera()->setLeftKey(false);
			break;
		case GLFW_KEY_RIGHT:
		case GLFW_KEY_D:
			scene->getCurrentCamera()->setRightKey(false);
			break;
		case GLFW_KEY_LEFT_SHIFT:
		case GLFW_KEY_RIGHT_SHIFT:
			scene->getCurrentCamera()->setSprintKey(false);
			break;

		case GLFW_KEY_E:
			scene->getCurrentCamera()->setEKey(false);
			break;
		case GLFW_KEY_Q:
			scene->getCurrentCamera()->setQKey(false);
			break;

		default:
			break;
		}
	}
}