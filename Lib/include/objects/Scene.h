#pragma once

#include <glm/glm.hpp>

#include <memory>
#include <vector>
#include "objects/Mesh.h"
#include "objects/Sun.h"
#include "objects/Camera.h"
#include "objects/Skybox.h"
#include "objects/Ground.h"

// Ownership: meshes, cameras, sun, sky and ground are owned here
// (unique_ptr). Getters return borrowed raw pointers/views; passing an
// object in transfers ownership to the Scene. currentCamera is a
// non-owning alias into cameras (never deleted here).
class Scene : public Object3D {
public:
  Scene();
  ~Scene();

  // Borrowed snapshot views; do not store (rebuilt per call).
  std::vector<Mesh *> getMeshes();
  Sun *getSun();
  std::vector<Camera *> getCameras();
  SkyBox *getSky();
  Ground *getGround();

  Mesh *getMeshAt(int index);
  Camera *getCameraAt(int index);

  Camera *getCurrentCamera();
  void setCurrentCamera(Camera *camera);

  void addMesh(std::unique_ptr<Mesh> mesh);
  void setSun(std::unique_ptr<Sun> sun);
  void addCamera(std::unique_ptr<Camera> camera);
  void setSky(std::unique_ptr<SkyBox> sky);
  void setGround(std::unique_ptr<Ground> ground);

  void update(float delta);
  void init();

  bool getRenderAble();
  void setRenderAble(bool renderAble);

  const glm::vec3 &getCameraZ();

private:
  std::vector<std::unique_ptr<Mesh>> meshes;
  std::unique_ptr<Sun> sun;
  std::vector<std::unique_ptr<Camera>> cameras;
  std::unique_ptr<SkyBox> sky;
  std::unique_ptr<Ground> ground;

  Camera *currentCamera = nullptr;
  glm::vec3 cameraZ;
  bool renderAble = false;
};
