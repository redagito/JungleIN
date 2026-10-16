#include "objects/Scene.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

Scene::Scene() : Object3D() {}

Scene::~Scene() {
  for (Mesh *m : meshes) {
    delete m;
  }
  meshes.clear();
  for (Camera *c : cameras) {
    delete c;
  }
  cameras.clear();
  // currentCamera aliases an entry of cameras; do not delete it twice.
  currentCamera = nullptr;
  delete sun;
  sun = nullptr;
  delete sky;
  sky = nullptr;
  delete ground;
  ground = nullptr;
}

void Scene::init() {
  if (sky == nullptr || sun == nullptr || ground == nullptr)
    throw std::runtime_error("Scene::init without sky, sun or ground");
  for (Mesh *m : meshes) {
    m->initVBO();
  }
  sky->initVBO();
  sun->initVBO();
  ground->initVBO();
}

void Scene::update(float delta) {
  if (ground == nullptr || currentCamera == nullptr)
    throw std::runtime_error("Scene::update without ground or camera");
  // Gameplay boundary (not an error): the heightfield only answers
  // inside +-MAP_SIZE/2, so clamp the camera into bounds before querying.
  // getY still throws on genuine bugs; this keeps normal edge-walking
  // from ever reaching it.
  const float half = Ground::MAP_SIZE / 2.0f - 1.0f;
  glm::vec3 &camPos = currentCamera->getPosition();
  camPos.x = std::clamp(camPos.x, -half, half);
  camPos.z = std::clamp(camPos.z, -half, half);
  float groundY = ground->getYApprox(currentCamera->getPosition().x,
                                     currentCamera->getPosition().z,
                                     currentCamera->getPosition().y);

  // Mouse deltas arrive via Window::onCursorMove; applying them here would
  // overwrite the event-sourced accel with zero.
  currentCamera->update(groundY, delta);

  float yaw = currentCamera->getYaw();
  cameraZ = glm::vec3(std::sin(yaw), 0, -std::cos(yaw));
  glm::vec3 cameraPostion = currentCamera->getPosition();

  sky->setPosition(cameraPostion);
  sun->update(cameraPostion, cameraZ);
}

std::vector<Mesh *> Scene::getMeshes() { return meshes; }

Sun *Scene::getSun() { return sun; }

std::vector<Camera *> Scene::getCameras() { return cameras; }

SkyBox *Scene::getSky() { return sky; }

Ground *Scene::getGround() { return ground; }

Mesh *Scene::getMeshAt(int index) { return meshes.at(index); }

Camera *Scene::getCameraAt(int index) { return cameras.at(index); }

Camera *Scene::getCurrentCamera() { return currentCamera; }

void Scene::setCurrentCamera(Camera *camera) { currentCamera = camera; }

void Scene::addMesh(Mesh *mesh) { meshes.push_back(mesh); }

void Scene::setSun(Sun *v) { this->sun = v; }

void Scene::addCamera(Camera *camera) { cameras.push_back(camera); }

void Scene::setSky(SkyBox *v) { this->sky = v; }

void Scene::setGround(Ground *v) { this->ground = v; }

bool Scene::getRenderAble() { return this->renderAble; }

void Scene::setRenderAble(bool renderable) { this->renderAble = renderable; }

const glm::vec3 &Scene::getCameraZ() { return cameraZ; }
