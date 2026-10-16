#pragma once

#include <cmath>
#include <stack>
#include <vector>

#include <glm/glm.hpp>

#include "helpers/Utils.h"
#include "helpers/Timer.h"
#include "core/TextureCube.h"

#include "materials/MaterialBasic.h"
#include "materials/MaterialShadow.h"

#include "render/ShaderLibrary.h"
#include "render/FrameBuffer.h"
#include "materials/MaterialWire.h"
#include "render/ShadowBuffer.h"
#include "render/PostComposer.h"

#include "objects/Mesh.h"
#include "objects/SSQuad.h"
#include "objects/Camera.h"
#include "objects/Ground.h"
#include "objects/Scene.h"
#include "objects/Skybox.h"
#include "objects/Sun.h"

class Renderer 
{
public:
	Renderer();
	~Renderer();

	void init(Scene* _scene, unsigned int w, unsigned int h);
	void resize(unsigned int w, unsigned int h);
	void loadShaders();

	void start();
	void stop();
	void render();
	void renderShadow();

	void drawSky();
	void drawSun();
	void drawGround();
	void drawMeshes();
	void drawMeshesShadow();

	PostComposer* getPostComposer();

	void toggleShadowCascade(bool active);
	// Diagnostic master switch for all shadow lookups (key 9): lighting
	// treats everything as lit. Shadow maps still render; only the
	// lookup is bypassed, so toggling is free of pass restructuring.
	void toggleShadows(bool enabled);
	bool isShadows() const;
	void toggleWireframe(bool enabled);
	bool isWireframe() const;
	void setRatio(float r);

private:
	void pushMatrix(const glm::mat4& mat);
	void popMatrix();

	void calcMVP();	void transmitShadowMatrix(bool withCascade = true);
	void transmitMVP();
	void transmitNormalMatrix();
	void transmitViewMatrix();

	Timer timer;
	float delta;
	bool active;

	Scene* scene = nullptr;
	unsigned int width, height;
	unsigned int finalWidth, finalHeight;
	float ratio;

	FrameBuffer* mainFBO = nullptr;
	ShadowBuffer* shadowFBO = nullptr;
	ShadowBuffer* shadowFBOCascade = nullptr;
	MaterialShadow* shadowMat = nullptr;
	// Flat unlit material for the wireframe overlay pass (key 0). When
	// materialOverride is set, drawGround/drawMeshes bind it instead of
	// the object materials and transmit MVP only.
	MaterialWire* wireMat = nullptr;
	Material* materialOverride = nullptr;
	PostComposer* postComposer = nullptr;

	bool useShadowCascade = false;
	bool useShadows = true;
	bool useWireframe = false;

	std::stack<glm::mat4> xform;
	glm::mat4 matrixProj, matrixView, matrixModel;

	// Reused visible-instance staging for the instanced draws below:
	// cleared per mesh, capacity persists, so frames allocate nothing.
	std::vector<glm::mat4> instanceStaging_;

	// Buffers
	Camera* m_camera = nullptr;
	Material* m_material = nullptr;
	Shader* m_shader = nullptr;
	SkyBox* m_skybox = nullptr;
	Sun* m_sun = nullptr;
	Ground* m_ground = nullptr;
	glm::mat4 m_mvp, m_mv;
	glm::vec3 m_lightDir;
	glm::vec3 m_lightIntensity;
	unsigned int m_locPosition, m_locColor, m_locNormal, m_locTexcoord, m_locMVP;
};
