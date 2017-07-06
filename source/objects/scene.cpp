#include "scene.h"

#include <cmath>

#include "helpers/utils.h"

Scene::Scene() : Object3D() {}

void Scene::init() {
  for (Mesh *m : meshes) {
    m->initVBO();
  }
  sky->initVBO();
  sun->initVBO();
  ground->initVBO();
}

void Scene::update(float delta) {
  float groundY = ground->getYApprox(currentCamera->getPosition().x,
                                     currentCamera->getPosition().z,
                                     currentCamera->getPosition().y);

  double x, y;
  glfwGetCursorPos(Utils::window, &x, &y);
  currentCamera->setMouse(std::floor(x), std::floor(y));
  // currentCamera->setMouse(QCursor::pos().x(), QCursor::pos().y());
  currentCamera->update(groundY, delta);

  float yaw = currentCamera->getYaw();
  cameraZ.set(std::sin(yaw), 0, -std::cos(yaw));
  Vector3 cameraPostion = currentCamera->getPosition();

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

void Scene::setSun(Sun *sun) { this->sun = sun; }

void Scene::addCamera(Camera *camera) { cameras.push_back(camera); }

void Scene::setSky(SkyBox *sky) { this->sky = sky; }

void Scene::setGround(Ground *ground) { this->ground = ground; }

// void Scene::removeMesh(Mesh *mesh) { meshes.removeOne(mesh); }

// void Scene::removeCamera(Camera *camera) { cameras.removeOne(camera); }

// void Scene::removeMeshAt(int index) { meshes.removeAt(index); }

// void Scene::removeCameraAt(int index) { cameras.removeAt(index); }

bool Scene::getRenderAble() { return this->renderAble; }

void Scene::setRenderAble(bool renderable) { this->renderAble = renderable; }

const Vector3 &Scene::getCameraZ() { return cameraZ; }
