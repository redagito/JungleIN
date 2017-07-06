#include "window.h"

#include <iostream>

#include <flextGL.h>
#include <GLFW/glfw3.h>

#include "helpers/utils.h"
#include "render/Debug.h"

void APIENTRY errorCallback(GLenum source, GLenum type, GLuint id,
                            GLenum severity, GLsizei length,
                            const GLchar *message, const void *userParam) {
  std::cout << "GL Error: " << (const char *)message << std::endl;
}

Window::Window() {
  //  startTimer(30);
  //  resize(1024, 768);

  //  musique = new QSound("ambiant.wav");
  //  musique->setLoops(-1);
  //  musique->play();

  //  setFocusPolicy(Qt::WheelFocus);
  //  setWindowTitle("JungleIN");
  //
  //  Utils::WINDOW = this;
}

Window::~Window() {}

void Window::run() {
  while (glfwWindowShouldClose(Utils::window) == 0) {
    glfwPollEvents();

    paintGL();
	PRINT_GL_ERROR();

    glfwSwapBuffers(Utils::window);
  }
}

// void Window::timerEvent(QTimerEvent *) { updateGL(); }

void Window::initializeGL() {
  if (!glfwInit()) {
    std::cout << "Failed to initialize GLFW." << std::endl;
    return;
  }

  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, FLEXT_MAJOR_VERSION);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, FLEXT_MINOR_VERSION);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // Create window from parameters
  std::cout << "Creating GLFW window for OpenGL version " << FLEXT_MAJOR_VERSION
            << ", " << FLEXT_MINOR_VERSION << std::endl;
  auto window = glfwCreateWindow(1024, 768, "JungleIN", NULL, NULL);
  if (window == nullptr) {
    std::cout << "Failed to create GLFW window." << std::endl;
    glfwTerminate();
    return;
  }

  glfwMakeContextCurrent(window);
  glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_FALSE);

  // Load OpenGL extensions
  if (flextInit() != GL_TRUE) {
    glfwTerminate();
    std::cout << "Failed to initialize flextGL." << std::endl;
    return;
  }

  PRINT_GL_ERROR();
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  PRINT_GL_ERROR();
  glDebugMessageCallback(&errorCallback, nullptr);
  PRINT_GL_ERROR();

  Utils::window = window;
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
  PRINT_GL_ERROR();
}

void Window::initializeObjects() {
  renderer = new Renderer();
  PRINT_GL_ERROR();
  renderer->loadShaders();
  PRINT_GL_ERROR();

  scene = SceneBuilder::build();
  PRINT_GL_ERROR();
  scene->init();
  PRINT_GL_ERROR();
  scene->setRenderAble(true);
  PRINT_GL_ERROR();

  renderer->init(scene, 1024, 768);
  PRINT_GL_ERROR();
  renderer->start();
  PRINT_GL_ERROR();
}

void Window::paintGL() { renderer->render(); }

void Window::resizeGL(int w, int h) { renderer->resize(w, h); }

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

void Window::exQuality(int value) {
  Utils::QUALITY = value;
  renderer->getPostComposer()->resizeHalfBuffers();
  PRINT_GL_ERROR();
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
  ReloadWindow *test = new ReloadWindow();
  test->reGenerate();
  // test->show();
}
