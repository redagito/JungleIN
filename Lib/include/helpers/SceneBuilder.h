#pragma once

#include <atomic>
#include <memory>
#include <mutex>

#include "objects/Scene.h"
#include "objects/Ground.h"
#include "helpers/ObjLoader.h"
#include "helpers/Geometries.h"
#include "materials/MaterialBasic.h"
#include "materials/MaterialSun.h"
#include "materials/MaterialGround.h"

class SceneBuilder
{
public:
  // Builds the whole scene; ownership transfers to the caller.
  // Every asset is unique_ptr-owned from birth, so a mid-build throw
  // (missing file, bad image) unwinds cleanly with nothing leaked.
  static std::unique_ptr<Scene> build();
  static void reloadGo(int density, int spacing);
  // Rescatter with the build defaults (current map bounds). Fail-fast
  // when build() has not run yet. Locked: safe from any thread, though
  // the app only calls it from the R-key handler.
  static void regenerateFromCurrentBounds();
  static std::atomic<int> NB_INSTANCE;
  // Test seam: forget the borrowed build aliases (the Scene owns them;
  // call after destroying the built scene so a later reloadGo cannot
  // run on dangling pointers).
  static void clearRegistry();

private:
  static void createInstances(Mesh* mesh, Ground* ground, int nbInstance);
  static std::mutex& registryMutex();
  static Scene* scene2;
  static Ground* ground2;
};