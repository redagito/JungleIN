#include "audio/AudioEngine.h"

#include <stdexcept>
#include <string>

float AudioEngine::fadeGain(double elapsedSeconds) {
  if (elapsedSeconds <= 0.0)
    return 0.0f;
  double t = elapsedSeconds / kFadeSeconds;
  if (t >= 1.0)
    return 1.0f;
  // Smoothstep: same 0/0.5/1 anchors as linear, gentler onset.
  return static_cast<float>(t * t * (3.0 - 2.0 * t));
}

AudioEngine::AudioEngine() = default;

AudioEngine::~AudioEngine() {
  if (hasSource) {
    alDeleteSources(1, &source);
    alDeleteBuffers(1, &buffer);
  }
  if (context != nullptr) {
    alcMakeContextCurrent(nullptr);
    alcDestroyContext(context);
  }
  if (device != nullptr)
    alcCloseDevice(device);
}

void AudioEngine::checkAL(const char* what) {
  const ALenum error = alGetError();
  if (error != AL_NO_ERROR)
    throw std::runtime_error(std::string("OpenAL error in ") + what + ": " +
                             std::to_string(static_cast<int>(error)));
}

void AudioEngine::init() {
  device = alcOpenDevice(nullptr);
  if (device == nullptr)
    throw std::runtime_error("Failed to open OpenAL device");

  context = alcCreateContext(device, nullptr);
  if (context == nullptr || alcMakeContextCurrent(context) == ALC_FALSE) {
    if (context != nullptr)
      alcDestroyContext(context);
    alcCloseDevice(device);
    device = nullptr;
    context = nullptr;
    throw std::runtime_error("Failed to create OpenAL context");
  }
}

void AudioEngine::playLoop(const AudioClip& clip) {
  if (device == nullptr || context == nullptr)
    throw std::runtime_error("AudioEngine::playLoop before init");
  if (clip.channels != 1 && clip.channels != 2)
    throw std::runtime_error("Unsupported channel count for OpenAL upload");
  if (clip.samples.empty())
    throw std::runtime_error("Cannot play empty audio clip");

  stop();
  if (hasSource) {
    alDeleteSources(1, &source);
    alDeleteBuffers(1, &buffer);
    hasSource = false;
  }

  const ALenum format =
      clip.channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
  alGenBuffers(1, &buffer);
  checkAL("alGenBuffers");
  alBufferData(buffer, format, clip.samples.data(),
               static_cast<ALsizei>(clip.samples.size() * sizeof(short)),
               clip.sampleRate);
  checkAL("alBufferData");

  alGenSources(1, &source);
  checkAL("alGenSources");
  alSourcei(source, AL_BUFFER, static_cast<ALint>(buffer));
  alSourcei(source, AL_LOOPING, AL_TRUE);
  alSourcef(source, AL_GAIN, 0.0f);
  checkAL("alSource setup");
  alSourcePlay(source);
  checkAL("alSourcePlay");

  hasSource = true;
  playing = true;
  started = std::chrono::steady_clock::now();
}

void AudioEngine::update() {
  if (!playing || !hasSource)
    return;
  const std::chrono::duration<double> elapsed =
      std::chrono::steady_clock::now() - started;
  alSourcef(source, AL_GAIN, fadeGain(elapsed.count()));
  checkAL("alSource gain update");
}

void AudioEngine::stop() {
  if (!playing || !hasSource)
    return;
  alSourceStop(source);
  playing = false;
}
