#pragma once

#include "framebuffer.h"
#include "objects/ssquad.h"

class PostComposer {
public:
	PostComposer(FrameBuffer* inputFBO);
	void render();
	void resize(unsigned int _width, unsigned int _height);
	void resizeHalfBuffers();

	void toggleFXAA(bool);
	void toggleDistord(bool);
	void toggleLenseFlares(bool);
	void toggleGodrays(bool);
	void toggleBloom(bool);

	void setFlareDispersal(float);
	void setFlareIntensity(float);
	void setGamma(float);

	void setLightPosScreen(const Vector3& pos);
	void setFinalRenderSize(int w, int h);

	void debugQuad(Texture* tex);

private:
	void swapBuffers();
	void swapHalfBuffers();

	void compose();
	void display();

	void passFXAA();
	void passLenseFlare();
	void passDistortion();
	void passBlurLinear(bool useHalf = false);
	void passGodrays();
	void passBloom();

	FrameBuffer* inputBuffer = nullptr;

	FrameBuffer* readBuffer = nullptr;
	FrameBuffer* writeBuffer = nullptr;

	FrameBuffer* readHalfBuffer = nullptr;
	FrameBuffer* writeHalfBuffer = nullptr;

	SSQuad* quad = nullptr;

	Shader* shaderCompose = nullptr;
	Shader* shaderDisplay = nullptr;
	Shader* shaderFXAA = nullptr;
	Shader* shaderLenseFlare1 = nullptr;
	Shader* shaderLenseFlare2 = nullptr;
	Shader* shaderLenseFlare3 = nullptr;
	Shader* shaderBlurLinearH = nullptr;
	Shader* shaderBlurLinearV = nullptr;
	Shader* shaderDistortion = nullptr;
	Shader* shaderGodrays1 = nullptr;
	Shader* shaderGodrays2 = nullptr;
	Shader* shaderBloom1 = nullptr;
	Shader* shaderBloom2 = nullptr;

	bool doFXAA = false;
	bool doDistord = false;
	bool doLenseFlares = false;
	bool doGodrays = false;
	bool doBloom = false;

	unsigned int width = 0;
	unsigned int height = 0;
	float widthf = 0.f;
	float heightf = 0.f;
	unsigned int finalRenderWidth = 0;
	unsigned int finalRenderHeight = 0;

	int halfBuffersRatio = 0;

	Vector3 gammaRGB;

	Vector3 flareThreshold;
	int flareSamples = 0;
	float flareDispersal = 0.f;
	float flareHaloWidth = 0.f;
	Vector3 flareChromaDispertion;
	float flareIntensity = 0.f;
	Texture* flareDirt = nullptr;

	Vector3 lightPosScreen;
};