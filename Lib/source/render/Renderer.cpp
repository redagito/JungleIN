#include "render/Renderer.h"

#include <cassert>
#include <stdexcept>

#include <glad/gl.h>

#include "helpers/Logger.h"

// Single shadow-map size: quality levels historically selected between
// two defines that held the same value, so the branch was dead.
#define MAX_SHADOW_WIDTH 2048

namespace {
// Debug-only: after binding a target, the GL viewport must match that
// target exactly. Catches passes that render with a leftover viewport.
void AssertViewport(unsigned int width, unsigned int height) {
#ifndef NDEBUG
	GLint viewport[4] = { 0, 0, 0, 0 };
	glGetIntegerv(GL_VIEWPORT, viewport);
	assert(viewport[2] == static_cast<GLint>(width) &&
		viewport[3] == static_cast<GLint>(height));
#else
	(void)width;
	(void)height;
#endif
}

// Shared shadow-transmit prologue for drawGround/drawMeshes/
// drawMeshesShadow. Lit call sites pass litMaterial == nullptr and get
// the shadow-map bind + cascade select + shadowQuality; the shadow
// call site passes the lit material and gets the alpha cutout state
// (the cascade/quality guards no-op on the shadow shader, which
// declares neither, and the shadow-map bind no-ops there too).
// Values match the legacy inline code exactly: every lit shader
// declares shadowQuality, and the shadow shader declares useAlpha,
// texAlpha and alphaCutoff.
void TransmitShadowState(Shader* shader, ShadowBuffer* shadowFBO,
	ShadowBuffer* cascadeFBO, bool useCascade, Material* litMaterial) {
	shadowFBO->bindAndTransmitTextures(shader);
	// Cascade shadows are an optional shader capability: only drive
	// the cascade uniforms when declared. Anything transmitted below
	// without a declaration throws instead.
	if (shader->hasUniform("cascadeShadow")) {
		if (useCascade) {
			cascadeFBO->bindAndTransmitTextures(shader);
			shader->transmitUniform("cascadeShadow", 1);
		}
		else {
			shader->transmitUniform("cascadeShadow", 0);
		}
	}
	if (shader->hasUniform("shadowQuality"))
		shader->transmitUniform("shadowQuality", Utils::QUALITY);
	if (litMaterial != nullptr && shader->hasUniform("useAlpha")) {
		if (litMaterial->hasAlpha()) {
			Texture* alpha = litMaterial->getAlpha();
			alpha->bind();
			shader->transmitUniform("texAlpha", alpha);
			shader->transmitUniform("useAlpha", 1);
			// Same per-material cutoff as the lit pass: a hardcoded
			// shadow cutoff would disagree with shaded cutout.
			shader->transmitUniform("alphaCutoff",
				litMaterial->getAlphaCutoff());
		}
		else {
			shader->transmitUniform("useAlpha", 0);
		}
	}
}
} // namespace

Renderer::Renderer() {
	active = false;
	delta = 0;

	width = 1024;
	height = 768;
	finalWidth = width;
	finalHeight = height;
	ratio = 1.0;

	mainFBO = 0;
	shadowFBO = 0;
	shadowMat = 0;
	postComposer = 0;

	__camera = 0;
	__material = 0;
	__shader = 0;
	__locPosition = 0;
	__locColor = 0;
	__locNormal = 0;
	__locTexcoord = 0;
	__locMVP = 0;

	useShadowCascade = true;
}

Renderer::~Renderer() {
	// postComposer borrows mainFBO; free it first (it never deletes input).
	delete postComposer;
	postComposer = nullptr;
	delete mainFBO;
	mainFBO = nullptr;
	delete shadowFBO;
	shadowFBO = nullptr;
	delete shadowFBOCascade;
	shadowFBOCascade = nullptr;
	delete shadowMat;
	shadowMat = nullptr;
	delete wireMat;
	wireMat = nullptr;
	// scene and __ observers belong to Window/Scene; do not delete.
	scene = nullptr;
	__camera = nullptr;
	__material = nullptr;
	__shader = nullptr;
	__skybox = nullptr;
	__sun = nullptr;
	__ground = nullptr;
}

void Renderer::init(Scene* _scene, unsigned int w, unsigned int h) {
	assert(w > 0 && h > 0);
	shadowMat = new MaterialShadow();
	wireMat = new MaterialWire();

	width = w;
	height = h;

	mainFBO = new FrameBuffer(std::vector<std::string>() << "texScene"
		<< "texThreshold",
		w, h, true);

	mainFBO->init(0, true);

	shadowFBO =
		new ShadowBuffer("texShadow", MAX_SHADOW_WIDTH, MAX_SHADOW_WIDTH);

	shadowFBO->init(7);

	shadowFBOCascade = new ShadowBuffer("texShadowCascade", MAX_SHADOW_WIDTH,
		MAX_SHADOW_WIDTH);

	shadowFBOCascade->init(8);

	postComposer = new PostComposer(mainFBO);

	scene = _scene;
	__camera = scene->getCurrentCamera();
	__skybox = scene->getSky();
	__sun = scene->getSun();
	__ground = scene->getGround();

	matrixProj = __camera->getProjection();
	matrixView = __camera->getMatrix();
	matrixModel = glm::mat4(1.0f);
	xform.push(matrixModel);

}

void Renderer::start() {
	active = true;
	timer.start();
}

void Renderer::stop() { active = false; }

void Renderer::render() {

	if (!active || !scene->getRenderAble())
		return;

	// Single source of truth for the movement time scale (see
	// Utils::movementDelta): do not "fix" the scale without retuning
	// every movement speed.
	delta = static_cast<float>(Utils::movementDelta(timer.restart()));

	scene->update(delta);

	__lightDir = __sun->getLight()->getDirection();
	__lightIntensity = __sun->getLight()->getIntensity();

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(1.0f);

	renderShadow();

	mainFBO->bind();
	mainFBO->resizeViewport();
	mainFBO->clear();

	matrixProj = __camera->getProjection();
	matrixView = __camera->getMatrix();

	drawSky();

	// Sun before opaque: the billboard sits 6 m out, so painter order is
	// what makes it behave as infinitely distant (everything opaque covers
	// it, the sky never does). It writes no depth (see MaterialSun).
	drawSun();

	pushMatrix(scene->getMatrix());

	// Diagnostic wireframe (key 0): ground + meshes draw as flat red
	// line triangles instead of the shaded fill, via the unlit override
	// material (rebinding the lit materials would shade the lines
	// identically to the surface and hide them). Sky, sun and shadow
	// maps always stay filled.
	if (useWireframe) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		materialOverride = wireMat;
	}

	drawGround();
	drawMeshes();

	if (useWireframe) {
		materialOverride = nullptr;
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	popMatrix();

	mainFBO->unbind();

	// Renderer::delta is seconds / TARGET_FPS (co-tuned with movement
	// speeds), so scale back to real frame seconds for the exposure
	// adaptation time constants. PostComposer clamps spikes itself.
	postComposer->render(delta * static_cast<float>(Utils::TARGET_FPS),
		!useWireframe);

	Texture::needsUpdate = false;
	Utils::calculFPS();
}

void Renderer::renderShadow() {
	// Shadow pass state: front-face culling reduces self-acne, polygon
	// offset pushes depth back without touching the lit-pass shaders.
	// Both are restored below so the lit pass is unaffected.
	GLboolean cullWasEnabled = glIsEnabled(GL_CULL_FACE);
	GLint cullMode = GL_BACK;
	glGetIntegerv(GL_CULL_FACE_MODE, &cullMode);
	GLboolean polyOffsetWasEnabled = glIsEnabled(GL_POLYGON_OFFSET_FILL);
	GLfloat polyFactor = 0.0f;
	GLfloat polyUnits = 0.0f;
	glGetFloatv(GL_POLYGON_OFFSET_FACTOR, &polyFactor);
	glGetFloatv(GL_POLYGON_OFFSET_UNITS, &polyUnits);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(2.0f, 4.0f);

	shadowFBO->bindAndViewport();
	AssertViewport(shadowFBO->getWidth(), shadowFBO->getHeight());
	shadowFBO->clear();

	matrixProj = __sun->getLight()->getProjMatrix();
	matrixView = __sun->getLight()->getViewMatrix();

	pushMatrix(scene->getMatrix());

	drawMeshesShadow();

	popMatrix();

	shadowFBO->unbind();

	if (useShadowCascade) {
		shadowFBOCascade->bindAndViewport();
		AssertViewport(shadowFBOCascade->getWidth(),
			shadowFBOCascade->getHeight());
		shadowFBOCascade->clear();

		matrixProj = __sun->getLight()->getProjMatrixCascade();
		matrixView = __sun->getLight()->getViewMatrix();

		pushMatrix(scene->getMatrix());
		drawMeshesShadow();
		popMatrix();

		shadowFBOCascade->unbind();
	}

	if (cullWasEnabled == GL_FALSE)
		glDisable(GL_CULL_FACE);
	else
		glCullFace(cullMode);
	if (polyOffsetWasEnabled == GL_FALSE)
		glDisable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(polyFactor, polyUnits);
}

void Renderer::drawSky() {
	// Phase 4: skybox follows the camera every frame (skyPos = camPos).
	// Scene::update already recenters it, but enforcing it here keeps
	// the draw correct even if update order ever changes. Drawn first
	// with LEQUAL + no depth write (see MaterialSkyBox), so opaque
	// geometry always covers it and the sky never punches holes.
	__skybox->setPosition(__camera->getPosition());
	pushMatrix(__skybox->getMatrix());

	__material = __skybox->getMaterial();
	__shader = __material->bind();

	calcMVP();
	transmitMVP();

	__skybox->drawWithVBO();

	popMatrix();
}

void Renderer::drawSun() {
	pushMatrix(__sun->getMatrix());

	__material = __sun->getMaterial();
	__shader = __material->bind();

	calcMVP();
	transmitMVP();

	// Clip coords of the sun origin are the MVP translation column.
	// Perspective-divide by clip w, then map NDC [-1,1] to UV [0,1].
	// Guard w <= 0 (sun behind camera): park the anchor off-screen so
	// the god-rays pass discards instead of firing from screen centre.
	// Column-major: the translation column is __mvp[3].
	float clipW = __mvp[3][3];
	glm::vec3 lightPosScreen(-1.0f);
	if (clipW > 0.0f) {
		lightPosScreen = glm::vec3(
			__mvp[3][0] / clipW * 0.5f + 0.5f,
			__mvp[3][1] / clipW * 0.5f + 0.5f,
			__mvp[3][2] / clipW * 0.5f + 0.5f);
	}
	postComposer->setLightPosScreen(lightPosScreen);

	__sun->drawWithVBO();

	popMatrix();
}

void Renderer::drawGround() {
	pushMatrix(__ground->getMatrix());

	// Wireframe overlay pass (key 0) binds the flat unlit material and
	// transmits MVP only; everything below is the lit-material path.
	__material = (materialOverride != nullptr) ? materialOverride
		: __ground->getMaterial();
	__shader = __material->bind();

	calcMVP();
	transmitMVP();
	if (materialOverride == nullptr) {
		TransmitShadowState(__shader, shadowFBO, shadowFBOCascade,
			useShadowCascade, nullptr);

		if (__shader->hasUniform("normalMatrix"))
			transmitNormalMatrix();
		if (__shader->hasUniform("viewMatrix"))
			transmitViewMatrix();
		if (__shader->hasUniform("mv"))
			__shader->transmitUniform("mv", __mv);
		transmitShadowMatrix(__shader->hasUniform("shadowMatrixCascade"));
		__shader->transmitUniform("lightDir", __lightDir);
		if (__shader->hasUniform("lightIntensity"))
			__shader->transmitUniform("lightIntensity", __lightIntensity);
		__shader->transmitUniform("shadowsOn", useShadows ? 1 : 0);
		__shader->transmitUniform("shadowQuality", Utils::QUALITY);
	}

	__ground->drawWithVBO();

	popMatrix();
}

bool Renderer::cull(Instance* m, float offset) {
	// Squared-distance threshold in m^2 (~22 m radius for 500), tight on
	// purpose for 1400+ instances. Compares against squared distance:
	// passing plain meters here would disable culling almost entirely.
	// The forward-plane margin is in plain meters.
	constexpr float kCullDistSq = 500.0f;
	constexpr float kCullForwardMargin = 20.0f;
	glm::vec3 meshPos = m->getPosition();
	glm::vec3 camPos = __camera->getPosition();
	glm::vec3 camZ = scene->getCameraZ();
	glm::vec3 toMesh = meshPos - camPos;
	return (glm::dot(toMesh, toMesh) > kCullDistSq + offset &&
		glm::dot(meshPos - (camPos - camZ * kCullForwardMargin), camZ) < 0);
}

void Renderer::drawMeshes() {
	for (Mesh* m : scene->getMeshes()) {
		__material = (materialOverride != nullptr) ? materialOverride
			: m->getMaterial();
		__shader = __material->bind();

		calcMVP();
		transmitMVP();
		if (materialOverride == nullptr) {
			TransmitShadowState(__shader, shadowFBO, shadowFBOCascade,
				useShadowCascade, nullptr);

			__shader->transmitUniform("lightDir", __lightDir);
			if (__shader->hasUniform("lightIntensity"))
				__shader->transmitUniform("lightIntensity", __lightIntensity);
			__shader->transmitUniform("shadowsOn", useShadows ? 1 : 0);
		}

		pushMatrix(m->getMatrix());
		calcMVP();
		transmitMVP();
		if (materialOverride == nullptr) {
			transmitNormalMatrix();
			transmitViewMatrix();
			if (__shader->hasUniform("mv"))
				__shader->transmitUniform("mv", __mv);
			transmitShadowMatrix();
		}

		m->drawWithVBO();

		popMatrix();

		for (Instance* i : m->getInstances()) {
			if (cull(i, 250)) {
				continue;
			}

			pushMatrix(i->getMatrix());
			calcMVP();
			transmitMVP();
			if (materialOverride == nullptr) {
				transmitNormalMatrix();
				transmitViewMatrix();
				if (__shader->hasUniform("mv"))
					__shader->transmitUniform("mv", __mv);
				transmitShadowMatrix();
			}

			m->drawWithVBO();

			popMatrix();
		}
	}
}

void Renderer::drawMeshesShadow() {
	for (Mesh* m : scene->getMeshes()) {
		if (!m->castsShadows()) {
			continue;
		}

		__material = shadowMat;
		__shader = __material->bind();

		TransmitShadowState(__shader, shadowFBO, shadowFBOCascade,
			useShadowCascade, m->getMaterial());

		pushMatrix(m->getMatrix());

		calcMVP();

		transmitMVP();

		m->drawWithVBO();

		popMatrix();

		for (Instance* i : m->getInstances()) {
			if (cull(i, 2000)) {
				continue;
			}

			pushMatrix(i->getMatrix());

			calcMVP();

			transmitMVP();

			m->drawWithVBO();

			popMatrix();

		}
	}
}

void Renderer::resize(unsigned int w, unsigned int h) {
	assert(w > 0 && h > 0);
	finalWidth = w;
	finalHeight = h;
	float a = w / static_cast<float>(h);

	if (ratio != 1.0) {
		w = (int)std::floor(w * ratio + .5);
		h = (int)std::floor(w / a + .5);
	}

	if (postComposer != 0) {
		postComposer->resize(w, h);
		postComposer->setFinalRenderSize(finalWidth, finalHeight);
	}

	if (mainFBO != 0)
		mainFBO->resize(w, h);
	if (shadowFBO != 0) {
		int ws = w * 2;
		if (ws > MAX_SHADOW_WIDTH)
			ws = MAX_SHADOW_WIDTH;
		shadowFBO->resize(ws, ws);
	}
	if (shadowFBOCascade != 0) {
		int ws = w * 2;
		if (ws > MAX_SHADOW_WIDTH)
			ws = MAX_SHADOW_WIDTH;
		shadowFBOCascade->resize(ws, ws);
	}
	if (__sun != 0)
		__sun->getLight()->resize(1);

	if (__camera != 0) {
		__camera->setPerspective(Camera::kDefaultFov, a, Camera::kDefaultNear, Camera::kDefaultFar);
		matrixProj = __camera->getProjection();
	}
}

PostComposer* Renderer::getPostComposer() { return postComposer; }

void Renderer::pushMatrix(const glm::mat4& mat) {
	xform.push(matrixModel);
	// model = base * local. The scene base is
	// identity, so stacked results match the old products.
	matrixModel = matrixModel * mat;
}

void Renderer::popMatrix() {
	if (xform.empty())
		throw std::runtime_error("Renderer::popMatrix: matrix stack underflow");
	matrixModel = xform.top();
	xform.pop();
}

void Renderer::calcMVP() {
	__mv = matrixView * matrixModel;
	__mvp = matrixProj * __mv;
}

void Renderer::transmitShadowMatrix(bool withCascade) {
	// Ortho-only: the bias*ortho*view*model chain preserves w == 1, so
	// the shader-side textureProj divide is a no-op. A perspective
	// light would need an explicit divide-by-w here instead.
	__shader->transmitUniform(
		"shadowMatrix", __sun->getLight()->getTextureProjMatrix(matrixModel));
	if (withCascade && useShadowCascade)
		__shader->transmitUniform(
			"shadowMatrixCascade",
			__sun->getLight()->getTextureProjMatrixCascade(matrixModel));
}

void Renderer::transmitMVP() { __shader->transmitUniform("mvp", __mvp); }

namespace {
// Inverse-transpose of the upper 3x3, falling back to identity on
// near-singular input (|det| <= 1e-6) instead of producing 1/det garbage.
glm::mat3 InverseTranspose3x3(const glm::mat4& m) {
	glm::mat3 m3(m);
	if (std::fabs(glm::determinant(m3)) <= 1e-6f)
		return glm::mat3(1.0f);
	return glm::transpose(glm::inverse(m3));
}
} // namespace

void Renderer::transmitNormalMatrix() {
	__shader->transmitUniform("normalMatrix", InverseTranspose3x3(__mv));
}

void Renderer::transmitViewMatrix() {
	__shader->transmitUniform("viewMatrix", InverseTranspose3x3(matrixView));
}

void Renderer::toggleShadowCascade(bool enabled) {
	// Cascade on/off only switches the shader branch. The old size hack
	// (growing the inner frustum to 120 m when disabled) is gone: with
	// outside-frustum rendering lit, disabling the cascade simply drops
	// far shadows instead of darkening the whole scene.
	useShadowCascade = enabled;
}

void Renderer::toggleShadows(bool enabled) { useShadows = enabled; }

bool Renderer::isShadows() const { return useShadows; }

void Renderer::toggleWireframe(bool enabled) { useWireframe = enabled; }

bool Renderer::isWireframe() const { return useWireframe; }

void Renderer::setRatio(float r) {
	ratio = r > 0.0f && r < 1.0f ? r : 1.0f;
	resize(finalWidth, finalHeight);
}

void Renderer::loadShaders() {
	ShaderLibrary::addShader("rt_basic", "data/shaders/rt_basic",
		std::vector<std::string>() << "position"
		<< "normal"
		<< "texcoord",
		std::vector<std::string>() << "mvp"
		<< "mv"
		<< "normalMatrix"
		<< "viewMatrix"
		<< "lightDir"
		<< "lightIntensity"
		<< "texDiffuse"
		<< "texAlpha"
		<< "alphaCutoff"
		<< "useDiffuse"
		<< "useAlpha"
		<< "diffuseColor"
		<< "shadowQuality"
		<< "texShadow"
		<< "shadowMatrix"
		<< "texShadowCascade"
		<< "shadowMatrixCascade"
		<< "cascadeShadow"
		<< "shadowsOn");

	ShaderLibrary::addShader("rt_sun", "data/shaders/rt_sun",
		std::vector<std::string>() << "position"
		<< "texcoord",
		std::vector<std::string>() << "mvp"
		<< "texDiffuse"
		<< "texAlpha");

	ShaderLibrary::addShader("rt_ground_n", "data/shaders/rt_ground_n",
		std::vector<std::string>() << "position"
		<< "normal",
		std::vector<std::string>() << "mvp"
		<< "mv"
		<< "normalMatrix"
		<< "viewMatrix"
		<< "shadowMatrix"
		<< "shadowMatrixCascade"
		<< "cascadeShadow"
		<< "shadowsOn"
		<< "lightDir"
		<< "lightIntensity"
		<< "heightRange"
		<< "tileSize"
		<< "texMoss"
		<< "texEarth"
		<< "texShatter"
		<< "texNormalMoss"
		<< "texNormalEarth"
		<< "texNormalShatter"
		<< "shadowQuality"
		<< "texShadow"
		<< "texShadowCascade");

	ShaderLibrary::addShader("rt_shadow", "data/shaders/rt_shadow",
		std::vector<std::string>() << "position"
		<< "texcoord",
		std::vector<std::string>() << "mvp"
		<< "texAlpha"
		<< "alphaCutoff"
		<< "useAlpha");

	ShaderLibrary::addShader("rt_skybox", "data/shaders/rt_skybox",
		std::vector<std::string>() << "position",
		std::vector<std::string>() << "mvp"
		<< "texCube");

	ShaderLibrary::addShader("ss_compose", "data/shaders/ss_compose",
		std::vector<std::string>() << "position",
		std::vector<std::string>() << "texScene");

	ShaderLibrary::addShader("ss_display", "data/shaders/ss_display",
		std::vector<std::string>() << "position",
		std::vector<std::string>() << "texScene"
		<< "aspect"
		<< "gammaRGB"
		<< "exposure");

	ShaderLibrary::addShader("ss_fxaa", "data/shaders/ss_fxaa",
		std::vector<std::string>() << "position",
		std::vector<std::string>() << "texScene"
		<< "resolution");

	ShaderLibrary::addShader("ss_lenseflare_p1", "data/shaders/ss_lenseflare_p1",
		std::vector<std::string>() << "position",
		std::vector<std::string>() << "texScene"
		<< "flareThreshold");

	ShaderLibrary::addShader("ss_lenseflare_p2", "data/shaders/ss_lenseflare_p2",
		std::vector<std::string>() << "position",
		std::vector<std::string>()
		<< "texScene"
		<< "flareSamples"
		<< "flareDispersal"
		<< "flareHaloWidth");

	ShaderLibrary::addShader("ss_lenseflare_p3", "data/shaders/ss_lenseflare_p3",
		std::vector<std::string>() << "position",
		std::vector<std::string>() << "texScene"
		<< "texFlare"
		<< "texDirt"
		<< "flareIntensity");

	ShaderLibrary::addShader("ss_blur_linearh", "data/shaders/ss_blur_linearh",
		std::vector<std::string>() << "position",
		std::vector<std::string>() << "texScene"
		<< "resolution");

	ShaderLibrary::addShader("ss_blur_linearv", "data/shaders/ss_blur_linearv",
		std::vector<std::string>() << "position",
		std::vector<std::string>() << "texScene"
		<< "resolution");

	ShaderLibrary::addShader("ss_distortion", "data/shaders/ss_distortion",
		std::vector<std::string>() << "position",
		std::vector<std::string>() << "texScene");

	ShaderLibrary::addShader("ss_godrays_p1", "data/shaders/ss_godrays_p1",
		std::vector<std::string>() << "position",
		std::vector<std::string>() << "texScene"
		<< "lightPos");

	ShaderLibrary::addShader("ss_godrays_p2", "data/shaders/ss_godrays_p2",
		std::vector<std::string>() << "position",
		std::vector<std::string>() << "texScene"
		<< "texGodrays");

	ShaderLibrary::addShader("ss_bloom_p1", "data/shaders/ss_bloom_p1",
		std::vector<std::string>() << "position",
		std::vector<std::string>() << "texScene");

	ShaderLibrary::addShader("ss_bloom_p2", "data/shaders/ss_bloom_p2",
		std::vector<std::string>() << "position",
		std::vector<std::string>() << "texScene"
		<< "texBloom");

	ShaderLibrary::addShader("ss_debugtext", "data/shaders/ss_debugtext",
		std::vector<std::string>() << "position"
		<< "texcoord",
		std::vector<std::string>() << "texAtlas");

	ShaderLibrary::addShader("ss_wire", "data/shaders/ss_wire",
		std::vector<std::string>() << "position",
		std::vector<std::string>() << "mvp"
		<< "wireColor");

	// Fail here, not at first draw: registration is compiled in, sources
	// load from data/ at runtime, and any skew between them is fatal.
	ShaderLibrary::validateRegistrations();
}
