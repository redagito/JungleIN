#include "scenebuilder.h"

#include "render/Debug.h"

Scene *SceneBuilder::scene2 = NULL;
Ground *SceneBuilder::ground2 = NULL;
int SceneBuilder::NB_INSTANCE = 1400;

Scene *SceneBuilder::build() {
	PRINT_GL_ERROR();
  Camera *camera = new Camera();
  PRINT_GL_ERROR();
  // camera->setMouse(QCursor::pos().x(), QCursor::pos().y());
  camera->setMouse(0, 0);
  camera->reset();
  PRINT_GL_ERROR();

  SkyBox *skyBox = new SkyBox(
      new MaterialSkyBox(new TextureCube("data/resources/cubemaps/miramar/")));
  PRINT_GL_ERROR();
  skyBox->setPosition(camera->getPosition());
  PRINT_GL_ERROR();

  // SUN
  Texture::resetUnit();
  PRINT_GL_ERROR();
  Texture *sunDiffuse = Texture::newFromNextUnit();
  PRINT_GL_ERROR();
  Texture *sunAlpha = Texture::newFromNextUnit();
  PRINT_GL_ERROR();
  sunDiffuse->load("data/resources/maps/sun/sun_1k.png");
  PRINT_GL_ERROR();
  sunDiffuse->setFilters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
  PRINT_GL_ERROR();
  sunDiffuse->init();
  PRINT_GL_ERROR();
  sunAlpha->load("data/resources/maps/sun/sun_1k_alpha.png");
  PRINT_GL_ERROR();
  sunAlpha->setFilters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
  PRINT_GL_ERROR();
  sunAlpha->init();
  PRINT_GL_ERROR();
  MaterialSun *sunMat = new MaterialSun(sunDiffuse, sunAlpha);
  PRINT_GL_ERROR();
  Sun *sun = new Sun(sunMat);
  PRINT_GL_ERROR();

  // GROUND
  Texture::resetUnit();
  // diffuses
  Texture *groundMoss = Texture::newFromNextUnit();
  Texture *groundEarth = Texture::newFromNextUnit();
  Texture *groundShatter = Texture::newFromNextUnit();
  groundMoss->load("data/resources/maps/ground/moss.png");
  groundMoss->setFilters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
  groundMoss->init();
  groundEarth->load("data/resources/maps/ground/earth.png");
  groundEarth->setFilters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
  groundEarth->init();
  groundShatter->load("data/resources/maps/ground/shatter.png");
  groundShatter->setFilters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
  groundShatter->init();
  PRINT_GL_ERROR();
  // normals
  Texture *groundNormalMoss = Texture::newFromNextUnit();
  Texture *groundNormalEarth = Texture::newFromNextUnit();
  Texture *groundNormalShatter = Texture::newFromNextUnit();
  groundNormalMoss->load("data/resources/maps/ground/moss_normal.png");
  groundNormalMoss->setFilters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
  groundNormalMoss->init();
  groundNormalEarth->load("data/resources/maps/ground/earth_normal.png");
  groundNormalEarth->setFilters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
  groundNormalEarth->init();
  groundNormalShatter->load("data/resources/maps/ground/shatter_normal.png");
  groundNormalShatter->setFilters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
  groundNormalShatter->init();
  PRINT_GL_ERROR();
  // init
  MaterialGround *groundMat = new MaterialGround(
      groundMoss, groundEarth, groundShatter, groundNormalMoss,
      groundNormalEarth, groundNormalShatter);
  Ground *ground =
      new Ground("data/resources/heightmaps/heightmap.png", groundMat);
  PRINT_GL_ERROR();

  // ROCK 1
  Texture::resetUnit();
  PRINT_GL_ERROR();
  Texture *rock1Diffuse = Texture::newFromNextUnit();
  Texture *rock1Alpha = Texture::newFromNextUnit();
  Mesh *rock1 = ObjLoader::loadObj("data/resources/meshes/rock1/rock1.obj");
  rock1Diffuse->load("data/resources/maps/rock1/rock1_1k.png");
  rock1Diffuse->setFilters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
  rock1Diffuse->init();
  rock1Alpha->load("data/resources/maps/noalpha.png");
  rock1Alpha->setFilters(GL_NEAREST, GL_NEAREST);
  rock1Alpha->init();
  ((MaterialBasic *)(rock1->getMaterial()))->setDiffuse(rock1Diffuse);
  ((MaterialBasic *)(rock1->getMaterial()))->setAlpha(rock1Alpha);
  rock1->setPosition(Vector3(-3, 0.5, 0));
  rock1->setScale(Vector3(0.6, 0.6, 0.6));
  rock1->setScale(Vector3(0.0, 0.0, 0.0));
  rock1->setScaleRdn(0.2);
  rock1->setHeightRdn(0.5);
  rock1->setPourcentage(0.2);
  rock1->setInstanceType(Mesh::INSTANCE_ROCK);
  rock1->setRangeScale(0.4);
  createInstances(rock1, ground, NB_INSTANCE * rock1->getPourcentage());
  PRINT_GL_ERROR();

  // PALM TREE
  Texture::resetUnit();
  Texture *palmDiffuse = Texture::newFromNextUnit();
  Texture *palmAlpha = Texture::newFromNextUnit();
  Mesh *palm =
      ObjLoader::loadObj("data/resources/meshes/palmtree/palmtree.obj");
  palmDiffuse->load("data/resources/maps/palmtree/palmtree_1k.png");
  palmDiffuse->setFilters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
  palmDiffuse->init();
  palmAlpha->load("data/resources/maps/palmtree/palmtree_1k_alpha.png");
  palmAlpha->setFilters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
  palmAlpha->init();
  ((MaterialBasic *)(palm->getMaterial()))->setDiffuse(palmDiffuse);
  ((MaterialBasic *)(palm->getMaterial()))->setAlpha(palmAlpha);
  palm->setPosition(Vector3(0, 0, -10));
  palm->setScale(Vector3(2, 2, 2));
  palm->setScale(Vector3(0, 0, 0));
  palm->setScaleRdn(0.8);
  palm->setHeightRdn(-2.5);
  palm->setPourcentage(0.07);
  palm->setInstanceType(Mesh::INSTANCE_PALM);
  palm->setRangeScale(0.8);
  createInstances(palm, ground, NB_INSTANCE * palm->getPourcentage());
  PRINT_GL_ERROR();

  // HIGH TREE
  Texture::resetUnit();
  Texture *htreeDiffuse = Texture::newFromNextUnit();
  Texture *htreeAlpha = Texture::newFromNextUnit();
  Mesh *htree =
      ObjLoader::loadObj("data/resources/meshes/hightree/hightree.obj");
  htreeDiffuse->load("data/resources/maps/hightree/hightree_1k.png");
  htreeDiffuse->setFilters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
  htreeDiffuse->init();
  htreeAlpha->load("data/resources/maps/hightree/hightree_1k_alpha.png");
  htreeAlpha->setFilters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
  htreeAlpha->init();
  ((MaterialBasic *)(htree->getMaterial()))->setDiffuse(htreeDiffuse);
  ((MaterialBasic *)(htree->getMaterial()))->setAlpha(htreeAlpha);
  htree->setPosition(Vector3(-6, 2, -16));
  htree->setScale(Vector3(0.6, 0.6, 0.6));
  htree->setScale(Vector3(0, 0, 0));
  htree->setScaleRdn(0.3);
  htree->setHeightRdn(-2.0);
  htree->setPourcentage(0.3);
  htree->setInstanceType(Mesh::INSTANCE_HTREE);
  htree->setRangeScale(0.4);
  createInstances(htree, ground, NB_INSTANCE * htree->getPourcentage());
  PRINT_GL_ERROR();

  // BAMBOO PALM
  Texture::resetUnit();
  Texture *btreeDiffuse = Texture::newFromNextUnit();
  Texture *btreeAlpha = Texture::newFromNextUnit();
  Mesh *btree =
      ObjLoader::loadObj("data/resources/meshes/bamboopalm/bamboopalm.obj");
  btreeDiffuse->load("data/resources/maps/bamboopalm/bamboopalm_1k.png");
  btreeDiffuse->setFilters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
  btreeDiffuse->init();
  btreeAlpha->load("data/resources/maps/bamboopalm/bamboopalm_1k_alpha.png");
  btreeAlpha->setFilters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
  btreeAlpha->init();
  ((MaterialBasic *)(btree->getMaterial()))->setDiffuse(btreeDiffuse);
  ((MaterialBasic *)(btree->getMaterial()))->setAlpha(btreeAlpha);
  btree->setPosition(Vector3(-4, -2, 4));
  btree->setScale(Vector3(0.6, 0.6, 0.6));
  btree->setScale(Vector3(0, 0, 0));
  btree->setScaleRdn(0.3);
  btree->setHeightRdn(-2.0);
  btree->setPourcentage(0.15);
  btree->setInstanceType(Mesh::INSTANCE_BTREE);
  btree->setRangeScale(1.0);
  createInstances(btree, ground, NB_INSTANCE * btree->getPourcentage());
  PRINT_GL_ERROR();

  // GROUND PALM
  Texture::resetUnit();
  Texture *gpalmDiffuse = Texture::newFromNextUnit();
  Texture *gpalmAlpha = Texture::newFromNextUnit();
  Mesh *gpalm =
      ObjLoader::loadObj("data/resources/meshes/groundpalm/groundpalm.obj");
  gpalmDiffuse->load("data/resources/maps/groundpalm/groundpalm_1k.png");
  gpalmDiffuse->setFilters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
  gpalmDiffuse->init();
  gpalmAlpha->load("data/resources/maps/groundpalm/groundpalm_1k_alpha.png");
  gpalmAlpha->setFilters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
  gpalmAlpha->init();
  ((MaterialBasic *)(gpalm->getMaterial()))->setDiffuse(gpalmDiffuse);
  ((MaterialBasic *)(gpalm->getMaterial()))->setAlpha(gpalmAlpha);
  gpalm->setPosition(Vector3(-3, -1, 0));
  gpalm->setScaleRdn(0.9);
  gpalm->setScale(Vector3(0, 0, 0));
  gpalm->setHeightRdn(-1.0);
  gpalm->setPourcentage(0.14);
  gpalm->setInstanceType(Mesh::INSTANCE_GPALM);
  gpalm->setRangeScale(0.4);
  createInstances(gpalm, ground, NB_INSTANCE * gpalm->getPourcentage());
  PRINT_GL_ERROR();

  // SMALL PLANT 1
  /*Texture::resetUnit();
  Texture* splant1Diffuse = Texture::newFromNextUnit();
  Texture* splant1Alpha = Texture::newFromNextUnit();
  Mesh* splant1 =
  ObjLoader::loadObj("data/resources/meshes/smallplant1/smallplant1.obj");
  splant1Diffuse->load("data/resources/maps/smallplant1/smallplant1.png");
  splant1Diffuse->setFilters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_LINEAR);
  splant1Diffuse->init();
  splant1Alpha->load("data/resources/maps/smallplant1/smallplant1_alpha.png");
  splant1Alpha->setFilters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_LINEAR);
  splant1Alpha->init();
  ((MaterialBasic*)(splant1->getMaterial()))->setDiffuse(splant1Diffuse);
  ((MaterialBasic*)(splant1->getMaterial()))->setAlpha(splant1Alpha);
  splant1->setPosition(Vector3(2,4,-8));
  splant1->setScale(Vector3(0.6,0.6,0.6));
  splant1->castsShadows(true);*/

  // BUSH 1
  Texture::resetUnit();
  Texture *bush1Diffuse = Texture::newFromNextUnit();
  Texture *bush1Alpha = Texture::newFromNextUnit();
  Mesh *bush1 = ObjLoader::loadObj("data/resources/meshes/bush1/bush1.obj");
  bush1Diffuse->load("data/resources/maps/bush1/bush1_1k.png");
  bush1Diffuse->setFilters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
  bush1Diffuse->init();
  bush1Alpha->load("data/resources/maps/bush1/bush1_1k_alpha.png");
  bush1Alpha->setFilters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
  bush1Alpha->init();
  ((MaterialBasic *)(bush1->getMaterial()))->setDiffuse(bush1Diffuse);
  ((MaterialBasic *)(bush1->getMaterial()))->setAlpha(bush1Alpha);
  bush1->setPosition(Vector3(-3, 0, 0));
  bush1->setScale(Vector3(0.6, 0.6, 0.6));
  bush1->setScale(Vector3(0, 0, 0));
  bush1->setScaleRdn(0.3);
  bush1->setHeightRdn(0.0);
  bush1->setPourcentage(0.14);
  bush1->setInstanceType(Mesh::INSTANCE_BUSH);
  bush1->setRangeScale(0.4);
  createInstances(bush1, ground, NB_INSTANCE * bush1->getPourcentage());
  PRINT_GL_ERROR();

  Scene *scene = new Scene();
  scene->addCamera(camera);
  scene->setCurrentCamera(camera);
  scene->setSky(skyBox);
  scene->setSun(sun);
  scene->setGround(ground);
  PRINT_GL_ERROR();

  scene->addMesh(rock1);
  scene->addMesh(palm);
  scene->addMesh(gpalm);
  // scene->addMesh(splant1);
  scene->addMesh(htree);
  scene->addMesh(btree);
  scene->addMesh(bush1);
  PRINT_GL_ERROR();

  scene2 = scene;
  ground2 = ground;
  PRINT_GL_ERROR();
  return scene;
}

void SceneBuilder::createInstances(Mesh *mesh, Ground *ground, int nbInstance) {
  for (int i = 0; i < nbInstance; ++i) {
    double scale = Utils::random(mesh->getScaleRdn(),
                                 mesh->getScaleRdn() + mesh->getRangeScale());
    Instance *instance = mesh->newInstance();
    if (mesh->getInstanceType() == Mesh::INSTANCE_ROCK)
      instance->setPosition(ground->randomMapPos(mesh->getHeightRdn(), true));
    else
      instance->setPosition(ground->randomMapPos(mesh->getHeightRdn(), false));
    instance->setRotation(Vector3(0, Utils::random(0.0, 360.0), 0));
    instance->setScale(Vector3(scale, scale, scale));
  }
}

void SceneBuilder::reloadGo(int density, int spacing) {

  ground2->removeRockPos();

  Ground::MAP_MAX = spacing;
  Ground::MAP_MIN = -spacing;
  scene2->setRenderAble(false);

  for (Mesh *mesh : scene2->getMeshes()) {
    if (mesh->getInstanceType() != Mesh::INSTANCE_NONE) {
      mesh->clearInstances();
      createInstances(mesh, ground2, density * mesh->getPourcentage());
    }
  }

  NB_INSTANCE = density;
  scene2->setRenderAble(true);
}
