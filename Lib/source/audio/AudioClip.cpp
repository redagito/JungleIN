#include "audio/AudioClip.h"

#include <stdexcept>

#define MINIMP3_IMPLEMENTATION
#include <minimp3_ex.h>

AudioClip AudioClip::decodeMP3(const std::string& file) {
  static_assert(sizeof(mp3d_sample_t) == sizeof(short),
                "minimp3 sample type must be 16-bit");

  mp3dec_ex_t decoder;
  if (mp3dec_ex_open(&decoder, file.c_str(), 0) != 0)
    throw std::runtime_error("Failed to open MP3 file: " + file);

  AudioClip clip;
  clip.channels = decoder.info.channels;
  clip.sampleRate = decoder.info.hz;
  if (clip.channels < 1 || clip.channels > 2 || clip.sampleRate <= 0) {
    mp3dec_ex_close(&decoder);
    throw std::runtime_error("Unsupported MP3 format in file: " + file);
  }

  std::vector<mp3d_sample_t> frame(static_cast<size_t>(1152) * 32 *
                                   static_cast<size_t>(clip.channels));
  size_t read = 0;
  while ((read = mp3dec_ex_read(&decoder, frame.data(), frame.size())) > 0)
    clip.samples.insert(clip.samples.end(), frame.begin(),
                        frame.begin() + read);
  mp3dec_ex_close(&decoder);

  if (clip.samples.empty() ||
      clip.samples.size() % static_cast<size_t>(clip.channels) != 0)
    throw std::runtime_error("MP3 decoded to no usable audio: " + file);

  return clip;
}
