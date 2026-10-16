#include "objects/Scene.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

Scene::Scene() : Object3D() {}

Scene::~Scene() {
  // Members own themselves (unique_ptr); currentCamera is a non-owning
  // alias into cameras and must not be deleted here.
  currentCamera = nullptr;
}

void Scene::init() {
  if (sky == nullptr || sun == nullptr || ground == nullptr)
    throw std::runtime_error("Scene::init without sky, sun or ground");
  for (const auto& m : meshes) {
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

std::vector<Mesh *> Scene::getMeshes() {
  std::vector<Mesh *> view;
  for (const auto& mesh : meshes)
    view.push_back(mesh.get());
  return view;
}

Sun *Scene::getSun() { return sun.get(); }

std::vector<Camera *> Scene::getCameras() {
  std::vector<Camera *> view;
  for (const auto& camera : cameras)
    view.push_back(camera.get());
  return view;
}

SkyBox *Scene::getSky() { return sky.get(); }

Ground *Scene::getGround() { return ground.get(); }

Mesh *Scene::getMeshAt(int index) { return meshes.at(index).get(); }

Camera *Scene::getCameraAt(int index) { return cameras.at(index).get(); }

Camera *Scene::getCurrentCamera() { return currentCamera; }

void Scene::setCurrentCamera(Camera *camera) { currentCamera = camera; }

void Scene::addMesh(std::unique_ptr<Mesh> mesh) { meshes.push_back(std::move(mesh)); }

void Scene::setSun(std::unique_ptr<Sun> v) { sun = std::move(v); }

void Scene::addCamera(std::unique_ptr<Camera> camera) { cameras.push_back(std::move(camera)); }

void Scene::setSky(std::unique_ptr<SkyBox> v) { sky = std::move(v); }

void Scene::setGround(std::unique_ptr<Ground> v) { ground = std::move(v); }

bool Scene::getRenderAble() { return this->renderAble; }

void Scene::setRenderAble(bool renderable) { this->renderAble = renderable; }

const glm::vec3 &Scene::getCameraZ() { return cameraZ; }
