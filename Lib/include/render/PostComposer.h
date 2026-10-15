#pragma once

#include <glm/glm.hpp>

#include "render/FrameBuffer.h"
#include "objects/SSQuad.h"

class PostComposer {
public:
	PostComposer(FrameBuffer* inputFBO);
	~PostComposer();
	// dtSeconds is real frame seconds for auto-exposure adaptation.
	// Non-positive falls back to 1/60 so headless/tests stay defined.
	void render(float dtSeconds = 0.0f);
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
	void setExposure(float);
	// HDR Phase 2: auto-exposure switch and manual bias. setExposure
	// sets the manual level (auto off) and doubles as the bias applied
	// on top of auto (auto on): transmitted = auto * bias : manual.
	void toggleAutoExposure(bool active);
	void setExposureBias(float value);

	void setLightPosScreen(const glm::vec3& pos);
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

	// HDR Phase 2: downsample to the luminance target, read back,
	// log-average, temporally adapt, and refresh the exposure knob.
	void updateAutoExposure(float dtSeconds);
	void applyExposure();

	FrameBuffer* inputBuffer = nullptr;

	FrameBuffer* readBuffer = nullptr;
	FrameBuffer* writeBuffer = nullptr;

	FrameBuffer* readHalfBuffer = nullptr;
	FrameBuffer* writeHalfBuffer = nullptr;

	// Dedicated 64x64 downsample target for luminance measurement.
	// Separate from the half-buffer chain so bloom/flare/godrays
	// ping-pong state is never disturbed.
	FrameBuffer* luminanceBuffer = nullptr;

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

	glm::vec3 gammaRGB;
	float exposure = 1.0f;

	// HDR Phase 2 auto-exposure state. Defaults keep the current
	// look as baseline: auto ON, bias 1.0, adapted == key so the
	// first frame transmits exposure 1.0.
	bool autoExposure = true;
	float exposureManual = 1.0f;
	float exposureBias = 1.0f;
	float adaptedLuminance = 0.2f;

	glm::vec3 flareThreshold;
	int flareSamples = 0;
	float flareDispersal = 0.f;
	float flareHaloWidth = 0.f;
	glm::vec3 flareChromaDispertion;
	float flareIntensity = 0.f;
	Texture* flareDirt = nullptr;

	glm::vec3 lightPosScreen;
};