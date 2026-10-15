#include "objects/Scene.h"

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

// void Scene::removeMesh(Mesh *mesh) { meshes.removeOne(mesh); }

// void Scene::removeCamera(Camera *camera) { cameras.removeOne(camera); }

// void Scene::removeMeshAt(int index) { meshes.removeAt(index); }

// void Scene::removeCameraAt(int index) { cameras.removeAt(index); }

bool Scene::getRenderAble() { return this->renderAble; }

void Scene::setRenderAble(bool renderable) { this->renderAble = renderable; }

const glm::vec3 &Scene::getCameraZ() { return cameraZ; }
