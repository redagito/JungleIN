#include "render/PostComposer.h"

#include "render/ShaderLibrary.h"
#include "helpers/Logger.h"
#include "helpers/Profiler.h"
#include "helpers/Utils.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

#include <glad/gl.h>

namespace {
// Single formula for the half-resolution post buffers, shared by the
// constructor, resize and quality changes so the first frames already
// match every later resize.
int HalfBuffersRatio() { return Utils::QUALITY.load() == 1 ? 3 : 4; }

// HDR Phase 2: temporal auto-exposure constants.
// key maps adapted log-average luminance to exposure = key / Ladapt.
// tauDarken/tauBrighten are adaptation time constants in seconds:
// darkening adapts slowly, brightening quickly.
constexpr float kExposureKey = 0.2f;
constexpr float kTauDarken = 1.5f;
constexpr float kTauBrighten = 0.5f;
// eps keeps log() defined on black pixels; the clamp ranges keep the
// first frame, NaN readbacks, and tab-switch dt spikes sane.
constexpr float kLuminanceEps = 1e-2f;
constexpr float kLuminanceMin = 1e-3f;
constexpr float kLuminanceMax = 10.0f;
constexpr float kExposureMin = 1e-3f;
constexpr float kExposureMax = 4.0f;
constexpr int kLuminanceSize = 64;
} // namespace

// Batch A: pure mapping for the blur resolution factor. Extracted so
// the ratio logic is unit-testable without a GL context (declared
// extern in Test/source/TestBatchA.cpp; PostComposer.h untouched).
float PostComposerBlurFactor(bool useHalf, int halfBuffersRatio) {
	return useHalf ? static_cast<float>(halfBuffersRatio) : 1.0f;
}

PostComposer::PostComposer(FrameBuffer* inputFBO) {
	width = inputFBO->getWidth();
	height = inputFBO->getHeight();
	widthf = (float)width;
	heightf = (float)height;
	finalRenderWidth = width;
	finalRenderHeight = height;

	halfBuffersRatio = HalfBuffersRatio();

	inputBuffer = inputFBO;
	readBuffer = new FrameBuffer(std::vector<std::string>() << "texScene", width,
		height, false);
	readBuffer->init(0, true);
	writeBuffer = new FrameBuffer(std::vector<std::string>() << "texScene", width,
		height, false);
	writeBuffer->init(0, true);
	readHalfBuffer = new FrameBuffer(std::vector<std::string>() << "texScene",
		width / halfBuffersRatio,
		height / halfBuffersRatio, false);
	readHalfBuffer->init(1, true);
	writeHalfBuffer = new FrameBuffer(std::vector<std::string>() << "texScene",
		width / halfBuffersRatio,
		height / halfBuffersRatio, false);
	writeHalfBuffer->init(1, true);
	// Fixed-size luminance target (unit offset 2: never aliases the
	// full/half chains). Linear filtering averages on downsample.
	luminanceBuffer = new FrameBuffer(std::vector<std::string>() << "texScene",
		kLuminanceSize, kLuminanceSize, false);
	luminanceBuffer->init(2, true);

	quad = new SSQuad();
	quad->init();

	shaderCompose = ShaderLibrary::getShader("ss_compose");
	shaderDisplay = ShaderLibrary::getShader("ss_display");
	shaderFXAA = ShaderLibrary::getShader("ss_fxaa");
	shaderLenseFlare1 = ShaderLibrary::getShader("ss_lenseflare_p1");
	shaderLenseFlare2 = ShaderLibrary::getShader("ss_lenseflare_p2");
	shaderLenseFlare3 = ShaderLibrary::getShader("ss_lenseflare_p3");
	shaderBlurLinearH = ShaderLibrary::getShader("ss_blur_linearh");
	shaderBlurLinearV = ShaderLibrary::getShader("ss_blur_linearv");
	shaderDistortion = ShaderLibrary::getShader("ss_distortion");
	shaderGodrays1 = ShaderLibrary::getShader("ss_godrays_p1");
	shaderGodrays2 = ShaderLibrary::getShader("ss_godrays_p2");
	shaderBloom1 = ShaderLibrary::getShader("ss_bloom_p1");
	shaderBloom2 = ShaderLibrary::getShader("ss_bloom_p2");

	doFXAA = true;
	doDistord = true;
	doLenseFlares = true;
	doGodrays = true;
	doBloom = true;

	// Phase 4: gamma lives only in the final display pass (ss_display
	// applies pow(color, 1/gamma)). 2.2 restores the intended brightness;
	// 1.0 was an identity that left the linear HDR output dark.
	gammaRGB = glm::vec3(2.2f, 2.2f, 2.2f);
	// Baseline look: auto-exposure ON with bias 1.0, adapted == key
	// so the first frames transmit exposure 1.0 until measured
	// luminance steers it.
	exposureManual = 1.0f;
	exposureBias = 1.0f;
	autoExposure = true;
	adaptedLuminance = kExposureKey;
	exposure = 1.0f;

	flareThreshold = glm::vec3(0.5, 0.5, 0.5);
	flareSamples = 12;
	flareDispersal = 0.10f;
	flareHaloWidth = 0.22f;
	flareIntensity = 1.0;
	flareDirt = new Texture("data/resources/maps/post/lensdirt_highc.png", 2);
	flareDirt->init();
}

PostComposer::~PostComposer() {
	// inputBuffer is borrowed from Renderer; shaders belong to ShaderLibrary.
	delete readBuffer;
	delete writeBuffer;
	delete readHalfBuffer;
	delete writeHalfBuffer;
	delete luminanceBuffer;
	delete quad;
	delete flareDirt;
}

void PostComposer::swapBuffers() {
	FrameBuffer* tmp = readBuffer;
	readBuffer = writeBuffer;
	writeBuffer = tmp;
}

void PostComposer::swapHalfBuffers() {
	FrameBuffer* tmp = readHalfBuffer;
	readHalfBuffer = writeHalfBuffer;
	writeHalfBuffer = tmp;
}

void PostComposer::render(float dtSeconds, bool effectsEnabled) {
	{
		Profiler::Scope scope("post.compose");
		compose();
	}
	{
		Profiler::Scope scope("post.autoexposure");
		updateAutoExposure(dtSeconds);
	}

	if (effectsEnabled) {
		if (doFXAA) {
			Profiler::Scope scope("post.fxaa");
			passFXAA();
		}
		if (doBloom) {
			Profiler::Scope scope("post.bloom");
			passBloom();
		}
		if (doLenseFlares) {
			Profiler::Scope scope("post.flare");
			passLenseFlare();
		}
		if (doGodrays) {
			Profiler::Scope scope("post.godrays");
			passGodrays();
		}
		if (doDistord) {
			Profiler::Scope scope("post.distortion");
			passDistortion();
		}
	}

	{
		Profiler::Scope scope("post.display");
		display();
	}
}

void PostComposer::compose() {
	readBuffer->bind();
	readBuffer->resizeViewport();

	glClear(GL_COLOR_BUFFER_BIT);

	shaderCompose->bind();
	inputBuffer->bindAndTransmitTextures(shaderCompose);
	quad->draw(shaderCompose->attribute("position"));

	readBuffer->unbind();
}

void PostComposer::passFXAA() {
	writeBuffer->bind();
	writeBuffer->resizeViewport();

	glClear(GL_COLOR_BUFFER_BIT);

	shaderFXAA->bind();
	shaderFXAA->transmitUniform("resolution", 1 / widthf, 1 / heightf);

	readBuffer->bindAndTransmitTextures(shaderFXAA);
	quad->draw(shaderFXAA->attribute("position"));
	writeBuffer->unbind();

	swapBuffers();
}

void PostComposer::passDistortion() {
	writeBuffer->bind();
	writeBuffer->resizeViewport();

	glClear(GL_COLOR_BUFFER_BIT);

	shaderDistortion->bind();

	readBuffer->bindAndTransmitTextures(shaderDistortion);

	quad->draw(shaderDistortion->attribute("position"));

	writeBuffer->unbind();

	swapBuffers();
}

/*!
 * Based on a Lense flare tutorial by
 * http://www.john-chapman.net/content.php?id=18
 */
void PostComposer::passLenseFlare() {
	// Pass 1 : Threshold + downsample + flip
	writeHalfBuffer->resizeViewport();
	writeHalfBuffer->bind();

	glClear(GL_COLOR_BUFFER_BIT);

	shaderLenseFlare1->bind();
	shaderLenseFlare1->transmitUniform("flareThreshold", flareThreshold);

	// readBuffer->bindAndTransmitTextures(shaderLenseFlare1);
	Texture* thresh = inputBuffer->getTexture("texThreshold");
	thresh->bind();
	shaderLenseFlare1->transmitUniform("texScene", thresh);

	quad->draw(shaderLenseFlare1->attribute("position"));

	writeHalfBuffer->unbind();

	swapHalfBuffers();

	// Pass 1b : Pre-blur
	if (Utils::QUALITY.load() == 1)
		passBlurLinear(true);

	// Pass 2 : Flaring
	writeHalfBuffer->resizeViewport();
	writeHalfBuffer->bind();

	glClear(GL_COLOR_BUFFER_BIT);

	shaderLenseFlare2->bind();
	shaderLenseFlare2->transmitUniform("flareSamples", flareSamples);
	shaderLenseFlare2->transmitUniform("flareDispersal", flareDispersal);
	shaderLenseFlare2->transmitUniform("flareHaloWidth", flareHaloWidth);

	readHalfBuffer->bindAndTransmitTextures(shaderLenseFlare2);

	quad->draw(shaderLenseFlare2->attribute("position"));

	writeHalfBuffer->unbind();

	swapHalfBuffers();

	// Pass 3 : Bluring (fast gaussian)
	if (Utils::QUALITY.load() == 1)
		passBlurLinear(true);

	// Pass 4 : Upsample + blur + compose
	writeBuffer->resizeViewport();
	writeBuffer->bind();

	glClear(GL_COLOR_BUFFER_BIT);

	shaderLenseFlare3->bind();
	Texture* flareTex = readHalfBuffer->getTexture("texScene");
	flareTex->bind();
	shaderLenseFlare3->transmitUniform("texFlare", flareTex);
	flareDirt->bind();
	shaderLenseFlare3->transmitUniform("texDirt", flareDirt);
	shaderLenseFlare3->transmitUniform("flareIntensity", flareIntensity);

	readBuffer->bindAndTransmitTextures(shaderLenseFlare3);

	quad->draw(shaderLenseFlare3->attribute("position"));

	writeBuffer->unbind();

	swapBuffers();
}

void PostComposer::passGodrays() {
	// Pass 1 : downsample
	writeHalfBuffer->resizeViewport();
	writeHalfBuffer->bind();

	glClear(GL_COLOR_BUFFER_BIT);

	shaderCompose->bind();
	Texture* thresh = inputBuffer->getTexture("texThreshold");
	thresh->bind();
	shaderCompose->transmitUniform("texScene", thresh);

	quad->draw(shaderGodrays1->attribute("position"));

	writeHalfBuffer->unbind();

	swapHalfBuffers();

	// Pass 1.b : blur
	passBlurLinear(true);

	// Pass 2 : rays
	writeHalfBuffer->resizeViewport();
	writeHalfBuffer->bind();

	glClear(GL_COLOR_BUFFER_BIT);

	shaderGodrays1->bind();
	shaderGodrays1->transmitUniform("lightPos", lightPosScreen);

	readHalfBuffer->bindAndTransmitTextures(shaderGodrays1);

	quad->draw(shaderGodrays1->attribute("position"));

	writeHalfBuffer->unbind();

	swapHalfBuffers();

	// Pass 1b : Pre-blur
	// if(Utils::QUALITY == 1) passBlurLinear(true);

	// Pass 3 : compose + upsample
	writeBuffer->resizeViewport();
	writeBuffer->bind();

	glClear(GL_COLOR_BUFFER_BIT);

	shaderGodrays2->bind();
	Texture* base = readBuffer->getTexture("texScene");
	base->bind();
	shaderGodrays2->transmitUniform("texScene", base);
	Texture* godrays = readHalfBuffer->getTexture("texScene");
	godrays->bind();
	shaderGodrays2->transmitUniform("texGodrays", godrays);

	quad->draw(shaderGodrays2->attribute("position"));

	writeBuffer->unbind();

	swapBuffers();
}

void PostComposer::passBloom() {
	// Pass 1 : downsample
	writeHalfBuffer->resizeViewport();
	writeHalfBuffer->bind();

	glClear(GL_COLOR_BUFFER_BIT);

	shaderBloom1->bind();
	readBuffer->bindAndTransmitTextures(shaderBloom1);

	quad->draw(shaderBloom1->attribute("position"));

	writeHalfBuffer->unbind();

	swapHalfBuffers();

	// Pass 1.b : blur
	passBlurLinear(true);

	// Pass 2 : compose + upsample
	writeBuffer->resizeViewport();
	writeBuffer->bind();

	glClear(GL_COLOR_BUFFER_BIT);

	shaderBloom2->bind();
	Texture* base = readBuffer->getTexture("texScene");
	base->bind();
	shaderBloom2->transmitUniform("texScene", base);
	Texture* bloom = readHalfBuffer->getTexture("texScene");
	bloom->bind();
	shaderBloom2->transmitUniform("texBloom", bloom);

	quad->draw(shaderBloom2->attribute("position"));

	writeBuffer->unbind();

	swapBuffers();
}

/*!
 * HDR Phase 2: temporal auto-exposure. Downsamples the composed scene
 * into the 64x64 luminance target (linear filter averages), reads it
 * back (one small sync point per frame), and takes the log-average
 * luminance Lavg = exp(mean(log(luma + eps))) with Rec.709 luma.
 * Ladapt tracks Lavg with an exponential approach whose tau is slow
 * for darkening and fast for brightening; exposure = key / Ladapt.
 */
void PostComposer::updateAutoExposure(float dtSeconds) {
	// Real frame seconds; non-positive (first frame / headless) falls
	// back to 1/60, and tab-switch spikes clamp to 0.5 s.
	float dt = dtSeconds > 0.0f ? dtSeconds : 1.0f / 60.0f;
	dt = std::min(dt, 0.5f);

	luminanceBuffer->bind();
	luminanceBuffer->resizeViewport();

	glClear(GL_COLOR_BUFFER_BIT);

	shaderCompose->bind();
	readBuffer->bindAndTransmitTextures(shaderCompose);
	quad->draw(shaderCompose->attribute("position"));

	std::vector<float> pixels(
		static_cast<size_t>(kLuminanceSize) *
		static_cast<size_t>(kLuminanceSize) * 4u);
	glReadPixels(0, 0, kLuminanceSize, kLuminanceSize, GL_RGBA, GL_FLOAT,
		pixels.data());
	// Fail fast: a failing downsample/readback used to leave the
	// zero-filled buffer behind, pinning adaptation at its floor and
	// freezing exposure. Throttled so the log stays readable.
	const GLenum readErr = glGetError();
	if (readErr != GL_NO_ERROR) {
		luminanceBuffer->unbind();
		throw std::runtime_error("PostComposer::updateAutoExposure "
			"glReadPixels failed: " + std::to_string(readErr));
	}
	static int exposureLogTick = 0;
	const bool logThis = (++exposureLogTick % 120 == 0);

	luminanceBuffer->unbind();

	double sum = 0.0;
	for (size_t i = 0; i < pixels.size(); i += 4) {
		// Rec.709 luma; negatives (should not happen) fold to black.
		float luma = 0.2126f * pixels[i] + 0.7152f * pixels[i + 1] +
			0.0722f * pixels[i + 2];
		luma = std::max(luma, 0.0f);
		sum += std::log(static_cast<double>(luma) + kLuminanceEps);
	}
	const double mean = sum / static_cast<double>(pixels.size() / 4);
	const float measured = static_cast<float>(std::exp(mean));
	if (logThis)
		logi("autoexposure dt={:.4f} err={} measured={:.5f} target={:.5f} "
			 "adapted={:.5f} exposure={:.4f}",
			dt, static_cast<unsigned int>(readErr), measured,
			std::clamp(measured, kLuminanceMin, kLuminanceMax),
			adaptedLuminance, exposure);
	// Non-finite readback is a broken measurement, not a floor to keep:
	// fail fast instead of freezing exposure at the old adaptation.
	if (!std::isfinite(measured))
		throw std::runtime_error("PostComposer::updateAutoExposure "
			"non-finite measured luminance");
	const float target =
		std::clamp(measured, kLuminanceMin, kLuminanceMax);
	const float tau = target < adaptedLuminance ? kTauDarken : kTauBrighten;
	adaptedLuminance = std::clamp(
		adaptedLuminance +
		(target - adaptedLuminance) * (1.0f - std::exp(-dt / tau)),
		kLuminanceMin, kLuminanceMax);

	applyExposure();
}

void PostComposer::applyExposure() {
	const float autoExposureValue = std::clamp(
		kExposureKey / adaptedLuminance, kExposureMin, kExposureMax);
	exposure = autoExposure ? autoExposureValue * exposureBias
		: exposureManual;
}

/*!
 * Fast gaussian approximation blur based on
 * http://rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/
 */
void PostComposer::passBlurLinear(bool useHalf) {
	float res = PostComposerBlurFactor(useHalf, halfBuffersRatio);

	FrameBuffer* inBuffer = useHalf ? readHalfBuffer : readBuffer;
	FrameBuffer* outBuffer = useHalf ? writeHalfBuffer : writeBuffer;

	// Vertical
	outBuffer->bind();
	outBuffer->resizeViewport();

	glClear(GL_COLOR_BUFFER_BIT);

	shaderBlurLinearV->bind();
	shaderBlurLinearV->transmitUniform("resolution", res / widthf, res / heightf);

	inBuffer->bindAndTransmitTextures(shaderBlurLinearV);

	quad->draw(shaderBlurLinearV->attribute("position"));

	outBuffer->unbind();

	// Swap buffers
	FrameBuffer* tmp = inBuffer;
	inBuffer = outBuffer;
	outBuffer = tmp;

	// Horizontal
	outBuffer->bind();
	outBuffer->resizeViewport();

	glClear(GL_COLOR_BUFFER_BIT);

	shaderBlurLinearH->bind();
	shaderBlurLinearH->transmitUniform("resolution", res / widthf, res / heightf);

	inBuffer->bindAndTransmitTextures(shaderBlurLinearH);

	quad->draw(shaderBlurLinearH->attribute("position"));

	outBuffer->unbind();
}

void PostComposer::display() {
	glClear(GL_COLOR_BUFFER_BIT);

	glViewport(0, 0, finalRenderWidth, finalRenderHeight);

	shaderDisplay->bind();

	readBuffer->bindAndTransmitTextures(shaderDisplay);

	shaderDisplay->transmitUniform("aspect", widthf / heightf);
	shaderDisplay->transmitUniform("gammaRGB", gammaRGB);
	shaderDisplay->transmitUniform("exposure", exposure);

	quad->draw(shaderDisplay->attribute("position"));
}

void PostComposer::debugQuad(Texture* tex) {
	glClear(GL_COLOR_BUFFER_BIT);

	shaderCompose->bind();

	tex->bind();
	shaderCompose->transmitUniform("texScene", tex);

	quad->draw(shaderCompose->attribute("position"));
}

void PostComposer::resize(unsigned int _width, unsigned int _height) {
	width = _width;
	height = _height;
	widthf = (float)_width;
	heightf = (float)_height;
	finalRenderWidth = width;
	finalRenderHeight = height;
	halfBuffersRatio = HalfBuffersRatio();
	unsigned int halfwidth = width / halfBuffersRatio;
	unsigned int halfheight = height / halfBuffersRatio;

	readBuffer->resize(width, height);
	writeBuffer->resize(width, height);
	readHalfBuffer->resize(halfwidth, halfheight);
	writeHalfBuffer->resize(halfwidth, halfheight);
}

void PostComposer::resizeHalfBuffers() {
	halfBuffersRatio = HalfBuffersRatio();
	unsigned int halfwidth = width / halfBuffersRatio;
	unsigned int halfheight = height / halfBuffersRatio;
	readHalfBuffer->resize(halfwidth, halfheight);
	writeHalfBuffer->resize(halfwidth, halfheight);
}

void PostComposer::toggleFXAA(bool active) { doFXAA = active; }

bool PostComposer::isFXAA() const { return doFXAA; }

void PostComposer::toggleDistord(bool active) { doDistord = active; }

void PostComposer::toggleLenseFlares(bool active) { doLenseFlares = active; }

bool PostComposer::isLenseFlares() const { return doLenseFlares; }

void PostComposer::toggleGodrays(bool active) { doGodrays = active; }

void PostComposer::toggleBloom(bool active) { doBloom = active; }

void PostComposer::setFlareDispersal(float value) { flareDispersal = value; }

void PostComposer::setFlareIntensity(float value) { flareIntensity = value; }

void PostComposer::setGamma(float value) { gammaRGB = glm::vec3(value); }

void PostComposer::setExposure(float value) {
	// Manual knob doubles as the auto bias: with auto ON the frame
	// transmits auto * bias, with auto OFF it transmits manual.
	exposureManual = value;
	exposureBias = value;
	applyExposure();
}

void PostComposer::toggleAutoExposure(bool active) {
	autoExposure = active;
	applyExposure();
}

bool PostComposer::isAutoExposure() const { return autoExposure; }

bool PostComposer::isGodrays() const { return doGodrays; }

bool PostComposer::isBloom() const { return doBloom; }

bool PostComposer::isDistord() const { return doDistord; }

void PostComposer::setExposureBias(float value) {
	exposureBias = value;
	applyExposure();
}

void PostComposer::setLightPosScreen(const glm::vec3& pos) {
	lightPosScreen = pos;
}

void PostComposer::setFinalRenderSize(int w, int h) {
	finalRenderWidth = w;
	finalRenderHeight = h;
}
