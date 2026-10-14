#pragma once

#include <cmath>
#include <stack>

#include "math/Matrix4.h"
#include "helpers/Utils.h"
#include "core/TextureCube.h"

#include "materials/MaterialBasic.h"
#include "materials/MaterialShadow.h"

#include "render/ShaderLibrary.h"
#include "render/FrameBuffer.h"
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
	void setRatio(float r);

private:
	void pushMatrix(const Matrix4& mat);
	void popMatrix();

	bool cull(Instance* m, float offset = 0.0);

	void calcMVP();
	void transmitShadowMatrix();
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
	PostComposer* postComposer = nullptr;

	bool useShadowCascade = false;

	std::stack<Matrix4> xform;
	Matrix4 matrixProj, matrixView, matrixModel;

	// Buffers
	Camera* __camera = nullptr;
	Material* __material = nullptr;
	Shader* __shader = nullptr;
	SkyBox* __skybox = nullptr;
	Sun* __sun = nullptr;
	Ground* __ground = nullptr;
	Matrix4 __mvp, __mv;
	Vector3 __lightDir;
	unsigned int __locPosition, __locColor, __locNormal, __locTexcoord, __locMVP;
};
