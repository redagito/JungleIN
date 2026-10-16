#include "render/Renderer.h"

#include <cassert>
#include <stdexcept>

#include <glad/gl.h>

#include <glm/gtc/type_ptr.hpp>

#include "helpers/Logger.h"
#include "helpers/Profiler.h"
#include "helpers/Frustum.h"

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
		shader->transmitUniform("shadowQuality", Utils::QUALITY.load());
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

	m_camera = 0;
	m_material = 0;
	m_shader = 0;
	m_locPosition = 0;
	m_locColor = 0;
	m_locNormal = 0;
	m_locTexcoord = 0;
	m_locMVP = 0;

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
	// scene and m_ observers belong to Window/Scene; do not delete.
	scene = nullptr;
	m_camera = nullptr;
	m_material = nullptr;
	m_shader = nullptr;
	m_skybox = nullptr;
	m_sun = nullptr;
	m_ground = nullptr;
}

void Renderer::init(Scene* _scene, unsigned int w, unsigned int h) {
	if (w == 0 || h == 0)
		throw std::runtime_error("Renderer::init with zero size");
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
	m_camera = scene->getCurrentCamera();
	m_skybox = scene->getSky();
	m_sun = scene->getSun();
	m_ground = scene->getGround();

	matrixProj = m_camera->getProjection();
	matrixView = m_camera->getMatrix();
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

	{
		Profiler::Scope scope("scene.update");
		scene->update(delta);
	}

	m_lightDir = m_sun->getLight()->getDirection();
	m_lightIntensity = m_sun->getLight()->getIntensity();

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(1.0f);

	{
		Profiler::Scope scope("render.shadow");
		renderShadow();
	}

	mainFBO->bind();
	mainFBO->resizeViewport();
	mainFBO->clear();

	matrixProj = m_camera->getProjection();
	matrixView = m_camera->getMatrix();

	{
		Profiler::Scope scope("render.sky");
		drawSky();
	}

	// Sun before opaque: the billboard sits 6 m out, so painter order is
	// what makes it behave as infinitely distant (everything opaque covers
	// it, the sky never does). It writes no depth (see MaterialSun).
	{
		Profiler::Scope scope("render.sun");
		drawSun();
	}

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

	{
		Profiler::Scope scope("render.ground");
		drawGround();
	}
	{
		Profiler::Scope scope("render.meshes");
		drawMeshes();
	}

	if (useWireframe) {
		materialOverride = nullptr;
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	popMatrix();

	mainFBO->unbind();

	// Renderer::delta is seconds / TARGET_FPS (co-tuned with movement
	// speeds), so scale back to real frame seconds for the exposure
	// adaptation time constants. PostComposer clamps spikes itself.
	{
		Profiler::Scope scope("post.total");
		postComposer->render(delta * static_cast<float>(Utils::TARGET_FPS.load()),
			!useWireframe);
	}

	Texture::needsUpdate.store(false);
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

	matrixProj = m_sun->getLight()->getProjMatrix();
	matrixView = m_sun->getLight()->getViewMatrix();

	pushMatrix(scene->getMatrix());

	drawMeshesShadow();

	popMatrix();

	shadowFBO->unbind();

	if (useShadowCascade) {
		shadowFBOCascade->bindAndViewport();
		AssertViewport(shadowFBOCascade->getWidth(),
			shadowFBOCascade->getHeight());
		shadowFBOCascade->clear();

		matrixProj = m_sun->getLight()->getProjMatrixCascade();
		matrixView = m_sun->getLight()->getViewMatrix();

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
	m_skybox->setPosition(m_camera->getPosition());
	pushMatrix(m_skybox->getMatrix());

	m_material = m_skybox->getMaterial();
	m_shader = m_material->bind();

	calcMVP();
	transmitMVP();

	m_skybox->drawWithVBO();

	popMatrix();
}

void Renderer::drawSun() {
	pushMatrix(m_sun->getMatrix());

	m_material = m_sun->getMaterial();
	m_shader = m_material->bind();

	calcMVP();
	transmitMVP();

	// Clip coords of the sun origin are the MVP translation column.
	// Perspective-divide by clip w, then map NDC [-1,1] to UV [0,1].
	// Guard w <= 0 (sun behind camera): park the anchor off-screen so
	// the god-rays pass discards instead of firing from screen centre.
	// Column-major: the translation column is m_mvp[3].
	float clipW = m_mvp[3][3];
	glm::vec3 lightPosScreen(-1.0f);
	if (clipW > 0.0f) {
		lightPosScreen = glm::vec3(
			m_mvp[3][0] / clipW * 0.5f + 0.5f,
			m_mvp[3][1] / clipW * 0.5f + 0.5f,
			m_mvp[3][2] / clipW * 0.5f + 0.5f);
	}
	postComposer->setLightPosScreen(lightPosScreen);

	m_sun->drawWithVBO();

	popMatrix();
}

void Renderer::drawGround() {
	pushMatrix(m_ground->getMatrix());

	// Wireframe overlay pass (key 0) binds the flat unlit material and
	// transmits MVP only; everything below is the lit-material path.
	m_material = (materialOverride != nullptr) ? materialOverride
		: m_ground->getMaterial();
	m_shader = m_material->bind();

	calcMVP();
	transmitMVP();
	if (materialOverride == nullptr) {
		TransmitShadowState(m_shader, shadowFBO, shadowFBOCascade,
			useShadowCascade, nullptr);

		if (m_shader->hasUniform("normalMatrix"))
			transmitNormalMatrix();
		if (m_shader->hasUniform("viewMatrix"))
			transmitViewMatrix();
		if (m_shader->hasUniform("mv"))
			m_shader->transmitUniform("mv", m_mv);
		transmitShadowMatrix(m_shader->hasUniform("shadowMatrixCascade"));
		m_shader->transmitUniform("lightDir", m_lightDir);
		if (m_shader->hasUniform("lightIntensity"))
			m_shader->transmitUniform("lightIntensity", m_lightIntensity);
		m_shader->transmitUniform("shadowsOn", useShadows ? 1 : 0);
		m_shader->transmitUniform("shadowQuality", Utils::QUALITY.load());
	}

	m_ground->drawWithVBO();

	popMatrix();
}

void Renderer::drawMeshes() {
	// One frustum for the whole pass (camera clip space here): plane
	// extraction per frame, box test per instance below.
	const Frustum frustum = Frustum::extract(matrixProj * matrixView);
	for (Mesh* m : scene->getMeshes()) {
		m_material = (materialOverride != nullptr) ? materialOverride
			: m->getMaterial();
		m_shader = m_material->bind();
		const ModelBox meshBox = m->boundingBox();

		if (materialOverride != nullptr) {
			// Diagnostic wireframe (key 0): legacy per-draw path,
			// verbatim. ss_wire has no instance support and this mode
			// is draw-diagnostic only, so it keeps one draw per
			// instance with per-instance MVP like always.
			calcMVP();
			transmitMVP();

			pushMatrix(m->getMatrix());
			calcMVP();
			transmitMVP();

			m->drawWithVBO();

			popMatrix();

			for (Instance* i : m->getInstances()) {
				const glm::mat4 world = matrixModel * i->getMatrix();
				if (CullWorldBox(frustum, world, meshBox)) {
					continue;
				}

				pushMatrix(i->getMatrix());
				calcMVP();
				transmitMVP();

				m->drawWithVBO();

				popMatrix();
			}
			continue;
		}

		// Shared fragment state (per mesh, unchanged legacy transmits).
		TransmitShadowState(m_shader, shadowFBO, shadowFBOCascade,
			useShadowCascade, nullptr);

		m_shader->transmitUniform("lightDir", m_lightDir);
		if (m_shader->hasUniform("lightIntensity"))
			m_shader->transmitUniform("lightIntensity", m_lightIntensity);
		m_shader->transmitUniform("shadowsOn", useShadows ? 1 : 0);
		// Frame-constant vertex state for the instanced shader. The
		// light clip chains (bias folded in) replace the old per-draw
		// shadowMatrix uniforms; identity model keeps just the shared
		// bias*proj*view half here, the instance half multiplies in
		// the vertex shader.
		transmitViewMatrix();
		m_shader->transmitUniform("proj", matrixProj);
		m_shader->transmitUniform("view", matrixView);
		m_shader->transmitUniform("shadowViewProj",
			m_sun->getLight()->getTextureProjMatrix(glm::mat4(1.0f)));
		if (useShadowCascade)
			m_shader->transmitUniform("shadowViewProjCascade",
				m_sun->getLight()->getTextureProjMatrixCascade(
					glm::mat4(1.0f)));

		// matrixModel is the scene base here (pushed once around the
		// loop by the caller): composing it explicitly keeps instance
		// matrices identical to the old pushMatrix-per-instance
		// products, including cull order.
		instanceStaging_.clear();
		for (Instance* i : m->getInstances()) {
			const glm::mat4 world = matrixModel * i->getMatrix();
			if (CullWorldBox(frustum, world, meshBox)) {
				continue;
			}
			instanceStaging_.push_back(world);
		}
		// The mesh-level template draw is skipped: zero-scale model
		// means degenerate triangles, provably zero fragments.
		if (!instanceStaging_.empty()) {
			m->uploadInstances(instanceStaging_.data(),
				instanceStaging_.size(), m_shader);
			m->drawInstanced(instanceStaging_.size());
		}
	}
}

void Renderer::drawMeshesShadow() {
	// Light clip space for this cascade pass (set by renderShadow
	// before each call): instances outside the shadow frustum skip
	// both shadow maps, not just the far half like the old test.
	const Frustum frustum = Frustum::extract(matrixProj * matrixView);
	for (Mesh* m : scene->getMeshes()) {
		if (!m->castsShadows()) {
			continue;
		}

		m_material = shadowMat;
		m_shader = m_material->bind();

		TransmitShadowState(m_shader, shadowFBO, shadowFBOCascade,
			useShadowCascade, m->getMaterial());

		// Frame-constant vertex state for the instanced shadow shader
		// (light clip space for this cascade pass).
		m_shader->transmitUniform("proj", matrixProj);
		m_shader->transmitUniform("view", matrixView);

		// matrixModel is the scene base here, same product the old
		// pushMatrix-per-instance loop computed.
		const ModelBox meshBox = m->boundingBox();
		instanceStaging_.clear();
		for (Instance* i : m->getInstances()) {
			const glm::mat4 world = matrixModel * i->getMatrix();
			if (CullWorldBox(frustum, world, meshBox)) {
				continue;
			}
			instanceStaging_.push_back(world);
		}
		// The mesh-level template draw is skipped: zero-scale model
		// means degenerate triangles, provably zero fragments.
		if (!instanceStaging_.empty()) {
			m->uploadInstances(instanceStaging_.data(),
				instanceStaging_.size(), m_shader);
			m->drawInstanced(instanceStaging_.size());
		}
	}
}

void Renderer::resize(unsigned int w, unsigned int h) {
	if (w == 0 || h == 0)
		throw std::runtime_error("Renderer::resize with zero size");
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
	if (m_sun != 0)
		m_sun->getLight()->resize(1);

	if (m_camera != 0) {
		m_camera->setPerspective(Camera::kDefaultFov, a, Camera::kDefaultNear, Camera::kDefaultFar);
		matrixProj = m_camera->getProjection();
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
	m_mv = matrixView * matrixModel;
	m_mvp = matrixProj * m_mv;
}

void Renderer::transmitShadowMatrix(bool withCascade) {
	// Ortho-only: the bias*ortho*view*model chain preserves w == 1, so
	// the shader-side textureProj divide is a no-op. A perspective
	// light would need an explicit divide-by-w here instead.
	m_shader->transmitUniform(
		"shadowMatrix", m_sun->getLight()->getTextureProjMatrix(matrixModel));
	if (withCascade && useShadowCascade)
		m_shader->transmitUniform(
			"shadowMatrixCascade",
			m_sun->getLight()->getTextureProjMatrixCascade(matrixModel));
}

void Renderer::transmitMVP() { m_shader->transmitUniform("mvp", m_mvp); }

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
	m_shader->transmitUniform("normalMatrix", InverseTranspose3x3(m_mv));
}

void Renderer::transmitViewMatrix() {
	m_shader->transmitUniform("viewMatrix", InverseTranspose3x3(matrixView));
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
		<< "texcoord"
		<< "instanceModel",
		std::vector<std::string>() << "proj"
		<< "view"
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
		<< "shadowViewProj"
		<< "texShadowCascade"
		<< "shadowViewProjCascade"
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
		<< "texcoord"
		<< "instanceModel",
		std::vector<std::string>() << "proj"
		<< "view"
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
