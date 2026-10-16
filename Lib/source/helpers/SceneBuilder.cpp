#include "helpers/SceneBuilder.h"
#include "helpers/Utils.h"
#include "helpers/Profiler.h"

#include <cmath>
#include <memory>
#include <mutex>
#include <stdexcept>

Scene* SceneBuilder::scene2 = nullptr;
Ground* SceneBuilder::ground2 = nullptr;
std::atomic<int> SceneBuilder::NB_INSTANCE{1400};

std::mutex& SceneBuilder::registryMutex() {
  static std::mutex mutex;
  return mutex;
}

namespace {
// One vegetation stanza as data: asset paths, alpha GL filters,
// spawn transform and instance distribution. build() runs
// configureVegetation + createInstances for every entry, in the same
// order as the stanzas below used to run.
struct VegetationSpec {
	const char* objPath;
	const char* diffusePath;
	const char* alphaPath;
	int alphaMinFilter;
	int alphaMagFilter;
	glm::vec3 position;
	float scaleRdn;
	float heightRdn;
	float pourcentage;
	unsigned int instanceType;
	float rangeScale;
};

// Shared stanza body: same texture-unit handling, asset loading,
// material wiring and parameter assignment as the code it replaces.
// The old stanzas called setScale twice (a base scale, then (0,0,0));
// setScale is a plain assignment, so only the final (0,0,0) is kept.
std::unique_ptr<Mesh> configureVegetation(const VegetationSpec& spec) {
	auto diffuse = std::make_unique<Texture>(0u);
	auto alpha = std::make_unique<Texture>(1u);
  auto mesh = ObjLoader::loadObj(spec.objPath);
	diffuse->load(spec.diffusePath);
	diffuse->setFilters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
	diffuse->init();
	alpha->load(spec.alphaPath);
	alpha->setFilters(spec.alphaMinFilter, spec.alphaMagFilter);
	alpha->init();
	((MaterialBasic*)(mesh->getMaterial()))->setDiffuse(std::move(diffuse));
	((MaterialBasic*)(mesh->getMaterial()))->setAlpha(std::move(alpha));
	mesh->setPosition(spec.position);
	mesh->setScale(glm::vec3(0.0f, 0.0f, 0.0f));
	mesh->setScaleRdn(spec.scaleRdn);
	mesh->setHeightRdn(spec.heightRdn);
	mesh->setPourcentage(spec.pourcentage);
	mesh->setInstanceType(spec.instanceType);
	mesh->setRangeScale(spec.rangeScale);
	return mesh;
}

const VegetationSpec kVegetation[] = {
	{"data/resources/meshes/rock1/rock1.obj",
		"data/resources/maps/rock1/rock1_1k.png",
		"data/resources/maps/noalpha.png", GL_NEAREST, GL_NEAREST,
		glm::vec3(-3, 0.5, 0), 0.2f, 0.5f, 0.2f, Mesh::INSTANCE_ROCK, 0.4f},
	{"data/resources/meshes/palmtree/palmtree.obj",
		"data/resources/maps/palmtree/palmtree_1k.png",
		"data/resources/maps/palmtree/palmtree_1k_alpha.png",
		GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR,
		glm::vec3(0, 0, -10), 0.8f, -2.5f, 0.07f, Mesh::INSTANCE_PALM, 0.8f},
	{"data/resources/meshes/hightree/hightree.obj",
		"data/resources/maps/hightree/hightree_1k.png",
		"data/resources/maps/hightree/hightree_1k_alpha.png",
		GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR,
		glm::vec3(-6, 2, -16), 0.3f, -2.0f, 0.3f, Mesh::INSTANCE_HTREE, 0.4f},
	{"data/resources/meshes/bamboopalm/bamboopalm.obj",
		"data/resources/maps/bamboopalm/bamboopalm_1k.png",
		"data/resources/maps/bamboopalm/bamboopalm_1k_alpha.png",
		GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR,
		glm::vec3(-4, -2, 4), 0.3f, -2.0f, 0.15f, Mesh::INSTANCE_BTREE, 1.0f},
	{"data/resources/meshes/groundpalm/groundpalm.obj",
		"data/resources/maps/groundpalm/groundpalm_1k.png",
		"data/resources/maps/groundpalm/groundpalm_1k_alpha.png",
		GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR,
		glm::vec3(-3, -1, 0), 0.9f, -1.0f, 0.14f, Mesh::INSTANCE_GPALM, 0.4f},
	{"data/resources/meshes/bush1/bush1.obj",
		"data/resources/maps/bush1/bush1_1k.png",
		"data/resources/maps/bush1/bush1_1k_alpha.png",
		GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR,
		glm::vec3(-3, 0, 0), 0.3f, 0.0f, 0.14f, Mesh::INSTANCE_BUSH, 0.4f},
};
} // namespace

std::unique_ptr<Scene> SceneBuilder::build() {

	auto camera = std::make_unique<Camera>();

	camera->setMouse(0, 0);
	camera->reset();

	auto skyBox = std::make_unique<SkyBox>(
		std::make_unique<MaterialSkyBox>(
			std::make_unique<TextureCube>("data/resources/cubemaps/miramar/", ".jpg")));

	skyBox->setPosition(camera->getPosition());
	// Phase 4: spawn position only; Scene::update recenters the sky on
	// the camera every frame (and Renderer::drawSky enforces it), so the
	// 1 m cube never drifts out of view.

	// SUN
	auto sunDiffuse = std::make_unique<Texture>(0u);
	auto sunAlpha = std::make_unique<Texture>(1u);

	sunDiffuse->load("data/resources/maps/sun/sun_1k.png");
	sunDiffuse->setFilters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
	sunDiffuse->init();

	sunAlpha->load("data/resources/maps/sun/sun_1k_alpha.png");
	sunAlpha->setFilters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
	sunAlpha->init();

	auto sunMat = std::make_unique<MaterialSun>(std::move(sunDiffuse),
		std::move(sunAlpha));

	auto sun = std::make_unique<Sun>(std::move(sunMat));

	// GROUND
	// The six ground maps are not seamless-tileable (measured opposite
	// edges differ strongly), and ground UVs tile them on a world grid,
	// so GL_REPEAT blends unrelated edges into hairline seams every
	// tile. MIRRORED_REPEAT blends each edge with itself instead.
	// diffuses
	auto groundMoss = std::make_unique<Texture>(0u);
	auto groundEarth = std::make_unique<Texture>(1u);
	auto groundShatter = std::make_unique<Texture>(2u);
	groundMoss->load("data/resources/maps/ground/moss.png");
	groundMoss->setFilters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
	groundMoss->setWrap(GL_MIRRORED_REPEAT, GL_MIRRORED_REPEAT);
	groundMoss->init();
	groundEarth->load("data/resources/maps/ground/earth.png");
	groundEarth->setFilters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
	groundEarth->setWrap(GL_MIRRORED_REPEAT, GL_MIRRORED_REPEAT);
	groundEarth->init();
	groundShatter->load("data/resources/maps/ground/shatter.png");
	groundShatter->setFilters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
	groundShatter->setWrap(GL_MIRRORED_REPEAT, GL_MIRRORED_REPEAT);
	groundShatter->init();

	// normals
	auto groundNormalMoss = std::make_unique<Texture>(3u);
	auto groundNormalEarth = std::make_unique<Texture>(4u);
	auto groundNormalShatter = std::make_unique<Texture>(5u);
	groundNormalMoss->load("data/resources/maps/ground/moss_normal.png");
	groundNormalMoss->setFilters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
	groundNormalMoss->setWrap(GL_MIRRORED_REPEAT, GL_MIRRORED_REPEAT);
	groundNormalMoss->init();
	groundNormalEarth->load("data/resources/maps/ground/earth_normal.png");
	groundNormalEarth->setFilters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
	groundNormalEarth->setWrap(GL_MIRRORED_REPEAT, GL_MIRRORED_REPEAT);
	groundNormalEarth->init();
	groundNormalShatter->load("data/resources/maps/ground/shatter_normal.png");
	groundNormalShatter->setFilters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
	groundNormalShatter->setWrap(GL_MIRRORED_REPEAT, GL_MIRRORED_REPEAT);
	groundNormalShatter->init();

	// init
	auto groundMat = std::make_unique<MaterialGround>(
		std::move(groundMoss), std::move(groundEarth),
		std::move(groundShatter), std::move(groundNormalMoss),
		std::move(groundNormalEarth), std::move(groundNormalShatter));
	auto ground = std::make_unique<Ground>(
		"data/resources/heightmaps/heightmap.png", std::move(groundMat));
	Profiler::mark("ground built");

	// Vegetation: one table-driven stanza per mesh (rock, palm, htree,
	// btree, gpalm, bush), run in the original order so texture units
	// and the instance random stream are unchanged.
	std::unique_ptr<Mesh> veg[6];
	for (int i = 0; i < 6; ++i) {
		veg[i] = configureVegetation(kVegetation[i]);
		createInstances(veg[i].get(), ground.get(), static_cast<int>(std::lround(NB_INSTANCE.load() * veg[i]->getPourcentage())));
	}

	// SMALL PLANT 1
	/*Texture* splant1Diffuse = new Texture(0u);
	Texture* splant1Alpha = new Texture(1u);
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
	splant1->setPosition(glm::vec3(2,4,-8));
	splant1->setScale(glm::vec3(0.6,0.6,0.6));
	splant1->castsShadows(true);*/

	// (bush1 is veg[5] from the vegetation loop above)

	auto scene = std::make_unique<Scene>();
	scene->setCurrentCamera(camera.get());
	scene->addCamera(std::move(camera));
	scene->setSky(std::move(skyBox));
	scene->setSun(std::move(sun));
	scene->setGround(std::move(ground));

	// addMesh order is load-bearing for the scene snapshot (rock, palm,
	// gpalm, htree, btree, bush): keep it exactly.
	scene->addMesh(std::move(veg[0]));
	scene->addMesh(std::move(veg[1]));
	scene->addMesh(std::move(veg[4]));
	// scene->addMesh(splant1);
	scene->addMesh(std::move(veg[2]));
	scene->addMesh(std::move(veg[3]));
	scene->addMesh(std::move(veg[5]));

  // Borrowed aliases for regeneration; Scene owns everything.
  {
    std::lock_guard<std::mutex> lock(registryMutex());
    scene2 = scene.get();
    ground2 = scene->getGround();
  }

  return scene;
}

void SceneBuilder::createInstances(Mesh* mesh, Ground* ground, int nbInstance) {
	for (int i = 0; i < nbInstance; ++i) {
		double scale = Utils::random(mesh->getScaleRdn(),
			mesh->getScaleRdn() + mesh->getRangeScale());
		Instance* instance = mesh->newInstance();
		const std::optional<glm::vec3> spot =
			mesh->getInstanceType() == Mesh::INSTANCE_ROCK
				? ground->randomMapPos(mesh->getHeightRdn(), true)
				: ground->randomMapPos(mesh->getHeightRdn(), false);
		if (!spot) {
			mesh->discardLastInstance();
			continue;
		}
		instance->setPosition(*spot);
		instance->setRotation(glm::vec3(0, static_cast<float>(Utils::random(0.0, 360.0)), 0));
		instance->setScale(glm::vec3(static_cast<float>(scale), static_cast<float>(scale), static_cast<float>(scale)));
	}
}

void SceneBuilder::reloadGo(int density, int spacing) {
  std::lock_guard<std::mutex> lock(registryMutex());
  if (ground2 == nullptr || scene2 == nullptr)
    throw std::runtime_error("SceneBuilder::reloadGo without a scene");

  ground2->removeRockPos();

  ground2->setMapBounds(static_cast<float>(-spacing),
    static_cast<float>(spacing));
  scene2->setRenderAble(false);

  for (Mesh* mesh : scene2->getMeshes()) {
    if (mesh->getInstanceType() != Mesh::INSTANCE_NONE) {
      mesh->clearInstances();
      createInstances(mesh, ground2, static_cast<int>(std::lround(density * mesh->getPourcentage())));
    }
  }

  NB_INSTANCE.store(density);
  scene2->setRenderAble(true);
}

void SceneBuilder::regenerateFromCurrentBounds() {  Ground* ground = nullptr;
  int density = 0;
  {
    std::lock_guard<std::mutex> lock(registryMutex());
    ground = ground2;
    density = NB_INSTANCE.load();
  }
  if (ground == nullptr)
    throw std::runtime_error("Window::exRegenerate without a scene");
  reloadGo(density, static_cast<int>(ground->getMapMax()));
}

void SceneBuilder::clearRegistry() {
  std::lock_guard<std::mutex> lock(registryMutex());
  scene2 = nullptr;
  ground2 = nullptr;
}
