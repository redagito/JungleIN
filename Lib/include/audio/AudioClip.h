#pragma once

#include <string>
#include <vector>

// Decoded PCM audio: interleaved 16-bit samples.
class AudioClip {
public:
  std::vector<short> samples;
  int channels = 0;
  int sampleRate = 0;

  // Decodes an MP3 file fully into memory. Throws std::runtime_error
  // naming the file when it cannot be opened, decoded or understood.
  static AudioClip decodeMP3(const std::string& file);
};
