#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include "objects/Mesh.h"
#include "objects/Sun.h"
#include "objects/Camera.h"
#include "objects/Skybox.h"
#include "objects/Ground.h"

class Scene : public Object3D {
public:
  Scene();
  ~Scene();

  std::vector<Mesh *> getMeshes();
  Sun *getSun();
  std::vector<Camera *> getCameras();
  SkyBox *getSky();
  Ground *getGround();

  Mesh *getMeshAt(int index);
  Camera *getCameraAt(int index);

  Camera *getCurrentCamera();
  void setCurrentCamera(Camera *camera);

  void addMesh(Mesh *mesh);
  void setSun(Sun *sun);
  void addCamera(Camera *camera);
  void setSky(SkyBox *sky);
  void setGround(Ground *ground);

  // void removeMesh(Mesh *mesh);
  // void removeCamera(Camera *camera);

  // void removeMeshAt(int index);
  // void removeCameraAt(int index);

  void update(float delta);
  void init();

  bool getRenderAble();
  void setRenderAble(bool renderAble);

  const Vector3 &getCameraZ();

private:
  std::vector<Mesh *> meshes;
  Sun *sun = nullptr;
  std::vector<Camera *> cameras;
  SkyBox *sky = nullptr;
  Ground *ground = nullptr;

  Camera *currentCamera = nullptr;
  Vector3 cameraZ;
  bool renderAble = false;
};

#endif // SCENE_H
