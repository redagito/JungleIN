#include "window.h"

#include <sstream>
#include <stdexcept>

#include <fmtlog/fmtlog.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "helpers/utils.h"

static void APIENTRY glDebugOutput(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
	const GLchar* message, const void* userParam)
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
	loge("{}", ss.str());
}

Window* Window::s_window = nullptr;

Window::Window() {
	if (s_window != nullptr)
		throw std::runtime_error("Already has a window");

	s_window = this;
}

Window::~Window() {}

void Window::run() {
	while (glfwWindowShouldClose(Utils::window) == 0) {
		glfwPollEvents();

		paintGL();

		glfwSwapBuffers(Utils::window);
	}
}

// void Window::timerEvent(QTimerEvent *) { updateGL(); }

void Window::initializeGL() {
	if (!glfwInit())
	{
		loge("Failed to initialize GLFW.");
		return;
	}

	glfwSetErrorCallback([](int code, const char* error) { throw std::runtime_error{error}; });
	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_FALSE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

	// Create window from parameters
	window = glfwCreateWindow(1024, 768, "JungleIN", NULL, NULL);
	if (window == nullptr) {
		loge("Failed to create GLFW window.");
		glfwTerminate();
		return;
	}

	glfwMakeContextCurrent(window);

	// Load opengl functions
	if (gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) == 0)
	{
		throw std::runtime_error{"Failed to init GL context"};
	}

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
	Utils::window = window;

	// User input
	// Key callback
	glfwSetKeyCallback(window,
		[](GLFWwindow* w, int key, int code, int action, int mods)
		{
			if (Window::s_window == nullptr)
				return;
			Window::s_window->onKeyPress(key, code, action, mods);
		});

	// Mouse move callback
	glfwSetCursorPosCallback(window,
		[](GLFWwindow* w, double x, double y)
		{
			if (Utils::window == nullptr)
				return;
			Window::s_window->onCursorMove(x, y);
		});

	glfwShowWindow(window);

	//
	//  if (!strstr((char *)glGetString(GL_EXTENSIONS),
	//              "GL_EXT_texture_filter_anisotropic")) {
	Utils::USE_ANISO = false;
	Utils::MAX_ANISO = 0x0000;
	//  } else {
	//    Utils::USE_ANISO = true;
	//    float maxAniso;
	//    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
	//    Utils::MAX_ANISO = maxAniso;
	//  }

	initializeObjects();
}

void Window::initializeObjects() {
	renderer = new Renderer();

	renderer->loadShaders();


	scene = SceneBuilder::build();

	scene->init();

	scene->setRenderAble(true);


	renderer->init(scene, 1024, 768);

	renderer->start();

}

void Window::paintGL() { renderer->render(); }

void Window::resizeGL(int w, int h) { renderer->resize(w, h); }

void Window::exQuality(int value) {
	Utils::QUALITY = value;
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

void Window::exFlareDisp(int value) {
	renderer->getPostComposer()->setFlareDispersal(value / 100.0);
}

void Window::exFlareIntensity(int value) {
	renderer->getPostComposer()->setFlareIntensity(value / 100.0);
}

void Window::exSunSpeed(int value) { scene->getSun()->setSpeed(value / 10.0); }

void Window::exGamma(int value) {
	renderer->getPostComposer()->setGamma(value / 100.0);
}

void Window::exAnisotropy(bool active) {
	Utils::USE_ANISO = active;
	Texture::needsUpdate = true;
}

void Window::exShadowCascade(bool active) {
	renderer->toggleShadowCascade(active);
}

void Window::exRenderRatio(int value) { renderer->setRatio(value / 10.0); }

void Window::playBtn(bool active) {
	//  if (active)
	//    musique->play();
	//  else
	//    musique->stop();
}

void Window::reGenerate() {
	ReloadWindow* test = new ReloadWindow();
	test->reGenerate();
	// test->show();
}

void Window::onCursorMove(double x, double y)
{
	// Update camera here
}

void Window::onKeyPress(int key, int code, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_UP:
			scene->getCurrentCamera()->setUpKey(true);
			break;
		case GLFW_KEY_DOWN:
			scene->getCurrentCamera()->setDownKey(true);
			break;
		case GLFW_KEY_LEFT:
			scene->getCurrentCamera()->setLeftKey(true);
			break;
		case GLFW_KEY_RIGHT:
			scene->getCurrentCamera()->setRightKey(true);
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
			scene->getCurrentCamera()->setUpKey(false);
			break;
		case GLFW_KEY_DOWN:
			scene->getCurrentCamera()->setDownKey(false);
			break;
		case GLFW_KEY_LEFT:
			scene->getCurrentCamera()->setLeftKey(false);
			break;
		case GLFW_KEY_RIGHT:
			scene->getCurrentCamera()->setRightKey(false);
			break;
		default:
			break;
		}
	}
}

// void Window::keyPressEvent(QKeyEvent *event) {
//  switch (event->key()) {
//  case Qt::Key_Left:
//    scene->getCurrentCamera()->setLeftKey(true);
//    break;
//  case Qt::Key_Right:
//    scene->getCurrentCamera()->setRightKey(true);
//    break;
//  case Qt::Key_Down:
//    scene->getCurrentCamera()->setDownKey(true);
//    break;
//  case Qt::Key_Up:
//    scene->getCurrentCamera()->setUpKey(true);
//    break;
//  default:
//    break;
//  }
//}
//
// void Window::keyReleaseEvent(QKeyEvent *event) {
//  switch (event->key()) {
//  case Qt::Key_Left:
//    scene->getCurrentCamera()->setLeftKey(false);
//    event->accept();
//    break;
//  case Qt::Key_Right:
//    scene->getCurrentCamera()->setRightKey(false);
//    event->accept();
//    break;
//  case Qt::Key_Down:
//    scene->getCurrentCamera()->setDownKey(false);
//    event->accept();
//    break;
//  case Qt::Key_Up:
//    scene->getCurrentCamera()->setUpKey(false);
//    event->accept();
//    break;
//  case Qt::Key_Escape:
//    Utils::MAIN->showNormal();
//    break;
//  case Qt::Key_O:
//    ((MainWindow *)(Utils::MAIN))->getDock()->isHidden()
//        ? ((MainWindow *)(Utils::MAIN))->getDock()->show()
//        : ((MainWindow *)(Utils::MAIN))->getDock()->hide();
//    break;
//  default:
//    break;
//  }
//}
//
// void Window::mousePressEvent(QMouseEvent *) {
//  scene->getCurrentCamera()->activeLook(true);
//}
//
// void Window::mouseReleaseEvent(QMouseEvent *) {
//  scene->getCurrentCamera()->activeLook(false);
//}