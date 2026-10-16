#pragma once

#include <chrono>
#include <mutex>

#include <AL/al.h>
#include <AL/alc.h>

#include "audio/AudioClip.h"

// Minimal OpenAL loop player: one looping source with a linear fade-in.
// All setup failures throw std::runtime_error; the destructor never throws.
class AudioEngine {
public:
  static constexpr float kFadeSeconds = 3.0f;

  // Loop gain for elapsed seconds since (re)start: 0 before start,
  // smoothstep ramp for kFadeSeconds, then 1 forever.
  static float fadeGain(double elapsedSeconds);

  AudioEngine();
  ~AudioEngine();

  AudioEngine(const AudioEngine&) = delete;
  AudioEngine& operator=(const AudioEngine&) = delete;

  void init();
  void playLoop(const AudioClip& clip);
  void update();
  void stop();

private:
  static void checkAL(const char* what);
  // stop() without locking; callers must hold mutex_.
  void stopLocked();

  mutable std::mutex mutex_;
  ALCdevice* device = nullptr;
  ALCcontext* context = nullptr;
  ALuint buffer = 0;
  ALuint source = 0;
  bool hasSource = false;
  bool playing = false;
  std::chrono::steady_clock::time_point started{};
};
