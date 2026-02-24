#ifndef BLITZNEXT_BB_SOUND_H
#define BLITZNEXT_BB_SOUND_H

#include <SDL3/SDL.h>
#include <iostream>
#include <string>
#include <cctype>
#include "bb_sdl.h"    // bb_sdl_ensure_(), bb_sdl_initialized_, bb_audio_update_hook_
#include "bb_string.h" // bbString

// ---- dr_mp3 (single-header MP3 decoder, public domain, David Reid) ----
#define DR_MP3_IMPLEMENTATION
#include "../thirdparty/dr_libs/dr_mp3.h"

// ---- stb_vorbis (single-file OGG Vorbis decoder, public domain, Sean Barrett) ----
#include "../thirdparty/stb/stb_vorbis.c"
// stb_vorbis leaks single-char macros L/C/R (channel routing flags) — undefine
// them immediately so they don't corrupt stb_image.h which uses L as a variable.
#undef L
#undef C
#undef R

// ---- Audio device ----

inline SDL_AudioDeviceID bb_snd_dev_  = 0;
inline SDL_AudioSpec     bb_snd_spec_ = {};

// ---- Sound and channel banks ----

inline constexpr int BB_MAX_SOUNDS   = 64;
inline constexpr int BB_MAX_CHANNELS = 32;

struct bb_Sound_ {
  Uint8*        data  = nullptr;
  Uint32        len   = 0;
  SDL_AudioSpec spec  = {};
  float         vol   = 1.0f;   // default volume for new channels (0.0–1.0)
  float         pan   = 0.0f;   // default pan  (-1=left, 0=center, 1=right)
  float         pitch = 0.0f;   // default pitch in Hz (0 = use native freq)
};

struct bb_Channel_ {
  SDL_AudioStream* stream  = nullptr;
  int              snd_id  = 0;
  bool             looping = false;
  bool             paused  = false;
  float            gain    = 1.0f;   // current volume (applied via SetAudioStreamGain)
  float            pan     = 0.0f;   // stored pan (-1..1; stereo mix is future work)
};

inline bb_Sound_   bb_snd_sounds_[BB_MAX_SOUNDS]     = {};
inline bb_Channel_ bb_snd_channels_[BB_MAX_CHANNELS] = {};

// ---- Lifecycle ----

// Lazily initialise SDL audio subsystem and open the default playback device.
// Separate from bb_sdl_ensure_() so that programs without sound pay no cost.
inline void bb_snd_ensure_() {
  if (bb_snd_dev_) return;
  bb_sdl_ensure_();
  if (!bb_sdl_initialized_) return;
  if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
    std::cerr << "[runtime] SDL audio init failed: " << SDL_GetError() << "\n";
    return;
  }
  bb_snd_dev_ = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
  if (!bb_snd_dev_) {
    std::cerr << "[runtime] SDL_OpenAudioDevice failed: " << SDL_GetError() << "\n";
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    return;
  }
  SDL_GetAudioDeviceFormat(bb_snd_dev_, &bb_snd_spec_, nullptr);
}

// Tear down all channels, free all sound buffers, close the audio device.
inline void bb_snd_quit_() {
  for (int i = 1; i < BB_MAX_CHANNELS; ++i) {
    if (bb_snd_channels_[i].stream) {
      SDL_UnbindAudioStream(bb_snd_channels_[i].stream);
      SDL_DestroyAudioStream(bb_snd_channels_[i].stream);
      bb_snd_channels_[i] = bb_Channel_{};
    }
  }
  for (int i = 1; i < BB_MAX_SOUNDS; ++i) {
    if (bb_snd_sounds_[i].data) {
      SDL_free(bb_snd_sounds_[i].data);
      bb_snd_sounds_[i] = bb_Sound_{};
    }
  }
  if (bb_snd_dev_) {
    SDL_CloseAudioDevice(bb_snd_dev_);
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    bb_snd_dev_ = 0;
  }
}

// ---- Looping maintenance + one-shot cleanup (called via hook) ----
//
// Called from bb_PollEvents() every frame via bb_audio_update_hook_.
// - Looping channels: refill when less than one sound-length of data remains.
// - One-shot channels: auto-destroy when the stream is fully drained.

inline void bb_snd_update_() {
  if (!bb_snd_dev_) return;
  for (int i = 1; i < BB_MAX_CHANNELS; ++i) {
    if (!bb_snd_channels_[i].stream || bb_snd_channels_[i].paused) continue;
    if (bb_snd_channels_[i].looping) {
      int snd = bb_snd_channels_[i].snd_id;
      if (snd < 1 || snd >= BB_MAX_SOUNDS || !bb_snd_sounds_[snd].data) continue;
      if (SDL_GetAudioStreamQueued(bb_snd_channels_[i].stream)
            < (int)bb_snd_sounds_[snd].len) {
        SDL_PutAudioStreamData(bb_snd_channels_[i].stream,
                               bb_snd_sounds_[snd].data,
                               (int)bb_snd_sounds_[snd].len);
      }
    } else {
      // Auto-cleanup finished one-shot channels
      if (SDL_GetAudioStreamQueued(bb_snd_channels_[i].stream) == 0) {
        SDL_UnbindAudioStream(bb_snd_channels_[i].stream);
        SDL_DestroyAudioStream(bb_snd_channels_[i].stream);
        bb_snd_channels_[i] = bb_Channel_{};
      }
    }
  }
}

// Register bb_snd_update_ with the SDL event pump hook at startup.
// The inline bool ensures exactly one registration per program.
inline const bool bb_snd_hook_reg_ = (bb_audio_update_hook_ = bb_snd_update_, true);

// ---- Sound API ----

// Internal: detect file extension (lowercase, no dot).
inline std::string bb_snd_ext_(const std::string& path) {
  auto dot = path.rfind('.');
  if (dot == std::string::npos) return "";
  std::string ext = path.substr(dot + 1);
  for (auto& c : ext) c = (char)std::tolower((unsigned char)c);
  return ext;
}

// Internal: load OGG Vorbis via stb_vorbis; decodes to int16 PCM.
// Returns a free sound slot index (1-based), 0 on failure.
inline int bb_load_ogg_(const char* path) {
  int channels = 0, sample_rate = 0;
  short* pcm = nullptr;
  int samples = stb_vorbis_decode_filename(path, &channels, &sample_rate, &pcm);
  if (samples < 0 || !pcm) {
    std::cerr << "[runtime] PlayMusic/LoadSound: failed to decode OGG: " << path << "\n";
    return 0;
  }
  Uint32 len = (Uint32)(samples * channels * sizeof(short));
  Uint8* buf = (Uint8*)SDL_malloc(len);
  if (!buf) { free(pcm); return 0; }
  memcpy(buf, pcm, len);
  free(pcm);

  SDL_AudioSpec spec{};
  spec.format   = SDL_AUDIO_S16LE;
  spec.channels = channels;
  spec.freq     = sample_rate;

  for (int i = 1; i < BB_MAX_SOUNDS; ++i) {
    if (!bb_snd_sounds_[i].data) {
      bb_snd_sounds_[i] = bb_Sound_{ buf, len, spec };
      return i;
    }
  }
  SDL_free(buf);
  return 0;
}

// Internal: load MP3 via dr_mp3; decodes to float32 PCM.
// Returns a free sound slot index (1-based), 0 on failure.
inline int bb_load_mp3_(const char* path) {
  drmp3_config cfg{};
  drmp3_uint64 frameCount = 0;
  float* pcm = drmp3_open_file_and_read_pcm_frames_f32(path, &cfg, &frameCount, nullptr);
  if (!pcm) {
    std::cerr << "[runtime] PlayMusic/LoadSound: failed to decode MP3: " << path << "\n";
    return 0;
  }
  Uint32 len = (Uint32)(frameCount * cfg.channels * sizeof(float));
  Uint8* buf = (Uint8*)SDL_malloc(len);
  if (!buf) { drmp3_free(pcm, nullptr); return 0; }
  memcpy(buf, pcm, len);
  drmp3_free(pcm, nullptr);

  SDL_AudioSpec spec{};
  spec.format   = SDL_AUDIO_F32LE;
  spec.channels = (int)cfg.channels;
  spec.freq     = (int)cfg.sampleRate;

  for (int i = 1; i < BB_MAX_SOUNDS; ++i) {
    if (!bb_snd_sounds_[i].data) {
      bb_snd_sounds_[i] = bb_Sound_{ buf, len, spec };
      return i;
    }
  }
  SDL_free(buf);
  return 0;
}

// Load a sound file (WAV or MP3); returns a sound handle (1-based, 0 on failure).
inline int bb_LoadSound(const bbString& file) {
  bb_snd_ensure_();
  if (!bb_snd_dev_) return 0;

  std::string ext = bb_snd_ext_(file);

  if (ext == "mp3")       return bb_load_mp3_(file.c_str());
  if (ext == "ogg")       return bb_load_ogg_(file.c_str());

  // WAV (and any format SDL_LoadWAV supports)
  for (int i = 1; i < BB_MAX_SOUNDS; ++i) {
    if (!bb_snd_sounds_[i].data) {
      SDL_AudioSpec spec{};
      Uint8*  buf = nullptr;
      Uint32  len = 0;
      if (!SDL_LoadWAV(file.c_str(), &spec, &buf, &len)) {
        std::cerr << "[runtime] LoadSound: SDL_LoadWAV failed for '" << file << "': "
                  << SDL_GetError() << "\n";
        return 0;
      }
      bb_snd_sounds_[i] = bb_Sound_{ buf, len, spec };
      return i;
    }
  }
  return 0;
}

// Free a sound and stop any channels currently playing it.
inline void bb_FreeSound(int snd) {
  if (snd < 1 || snd >= BB_MAX_SOUNDS || !bb_snd_sounds_[snd].data) return;
  for (int i = 1; i < BB_MAX_CHANNELS; ++i) {
    if (bb_snd_channels_[i].snd_id == snd && bb_snd_channels_[i].stream) {
      SDL_UnbindAudioStream(bb_snd_channels_[i].stream);
      SDL_DestroyAudioStream(bb_snd_channels_[i].stream);
      bb_snd_channels_[i] = bb_Channel_{};
    }
  }
  SDL_free(bb_snd_sounds_[snd].data);
  bb_snd_sounds_[snd] = bb_Sound_{};
}

// Internal: play sound once (loop=false) or loop forever (loop=true).
// Reuses the first finished one-shot slot; returns channel handle, 0 on failure.
inline int bb_play_sound_(int snd, bool loop) {
  bb_snd_ensure_();
  if (!bb_snd_dev_ || snd < 1 || snd >= BB_MAX_SOUNDS ||
      !bb_snd_sounds_[snd].data) return 0;
  // Find a free or finished slot
  int slot = 0;
  for (int i = 1; i < BB_MAX_CHANNELS; ++i) {
    if (!bb_snd_channels_[i].stream) { slot = i; break; }
    if (!bb_snd_channels_[i].looping &&
        SDL_GetAudioStreamQueued(bb_snd_channels_[i].stream) == 0) {
      SDL_UnbindAudioStream(bb_snd_channels_[i].stream);
      SDL_DestroyAudioStream(bb_snd_channels_[i].stream);
      bb_snd_channels_[i] = bb_Channel_{};
      slot = i;
      break;
    }
  }
  if (!slot) return 0;
  SDL_AudioStream* s = SDL_CreateAudioStream(&bb_snd_sounds_[snd].spec,
                                             &bb_snd_spec_);
  if (!s) return 0;
  if (!SDL_BindAudioStream(bb_snd_dev_, s)) {
    SDL_DestroyAudioStream(s);
    return 0;
  }
  SDL_PutAudioStreamData(s, bb_snd_sounds_[snd].data,
                         (int)bb_snd_sounds_[snd].len);
  if (!loop) SDL_FlushAudioStream(s);   // signal: no more data for one-shot
  // Apply sound-level defaults to the new stream
  SDL_SetAudioStreamGain(s, bb_snd_sounds_[snd].vol);
  if (bb_snd_sounds_[snd].pitch > 0.0f) {
    float orig = (float)bb_snd_sounds_[snd].spec.freq;
    if (orig > 0.0f)
      SDL_SetAudioStreamFrequencyRatio(s, bb_snd_sounds_[snd].pitch / orig);
  }
  bb_snd_channels_[slot] = bb_Channel_{ s, snd, loop };
  bb_snd_channels_[slot].gain = bb_snd_sounds_[snd].vol;
  bb_snd_channels_[slot].pan  = bb_snd_sounds_[snd].pan;
  return slot;
}

inline int  bb_PlaySound(int snd) { return bb_play_sound_(snd, false); }
inline int  bb_LoopSound(int snd) { return bb_play_sound_(snd, true);  }

// Stop playback and release the channel slot immediately.
inline void bb_StopChannel(int ch) {
  if (ch < 1 || ch >= BB_MAX_CHANNELS || !bb_snd_channels_[ch].stream) return;
  SDL_UnbindAudioStream(bb_snd_channels_[ch].stream);
  SDL_DestroyAudioStream(bb_snd_channels_[ch].stream);
  bb_snd_channels_[ch] = bb_Channel_{};
}

// ---- Channel control (M35) ----

// Pause: unbind the stream from the device so no data is consumed.
inline void bb_PauseChannel(int ch) {
  if (ch < 1 || ch >= BB_MAX_CHANNELS || !bb_snd_channels_[ch].stream ||
      bb_snd_channels_[ch].paused) return;
  SDL_UnbindAudioStream(bb_snd_channels_[ch].stream);
  bb_snd_channels_[ch].paused = true;
}

// Resume: rebind the stream so playback continues from the current position.
inline void bb_ResumeChannel(int ch) {
  if (ch < 1 || ch >= BB_MAX_CHANNELS || !bb_snd_channels_[ch].stream ||
      !bb_snd_channels_[ch].paused || !bb_snd_dev_) return;
  SDL_BindAudioStream(bb_snd_dev_, bb_snd_channels_[ch].stream);
  bb_snd_channels_[ch].paused = false;
}

// Returns 1 while the channel has an active stream (playing or paused), 0 when done.
inline int bb_ChannelPlaying(int ch) {
  if (ch < 1 || ch >= BB_MAX_CHANNELS) return 0;
  return bb_snd_channels_[ch].stream ? 1 : 0;
}

// Volume: 0.0 (silent) to 1.0 (full). Applied via SDL_SetAudioStreamGain.
// Only applied when not paused; stored in gain regardless.
inline void bb_ChannelVolume(int ch, float vol) {
  if (ch < 1 || ch >= BB_MAX_CHANNELS || !bb_snd_channels_[ch].stream) return;
  bb_snd_channels_[ch].gain = vol;
  if (!bb_snd_channels_[ch].paused)
    SDL_SetAudioStreamGain(bb_snd_channels_[ch].stream, vol);
}

// Pan: -1.0 (left), 0.0 (center), 1.0 (right).
// Value is stored for future stereo-pan audio processing; no SDL3 native pan.
inline void bb_ChannelPan(int ch, float pan) {
  if (ch < 1 || ch >= BB_MAX_CHANNELS || !bb_snd_channels_[ch].stream) return;
  bb_snd_channels_[ch].pan = pan;
}

// Pitch: frequency in Hz.  Ratio = hz / original_src_freq.
// SDL3 SDL_SetAudioStreamFrequencyRatio handles resampling transparently.
inline void bb_ChannelPitch(int ch, float hz) {
  if (ch < 1 || ch >= BB_MAX_CHANNELS || !bb_snd_channels_[ch].stream) return;
  int snd = bb_snd_channels_[ch].snd_id;
  if (snd < 1 || snd >= BB_MAX_SOUNDS) return;
  float orig = (float)bb_snd_sounds_[snd].spec.freq;
  if (orig > 0.0f)
    SDL_SetAudioStreamFrequencyRatio(bb_snd_channels_[ch].stream, hz / orig);
}

// ---- Music (M36) ----
//
// A single background-music "track" — one looping channel dedicated to music.
// bb_PlayMusic loads a WAV file into a temporary sound slot and starts it
// looping.  When a new track is started the old one is stopped first.
// PlayCDTrack is stub-only (CD audio is deprecated hardware).

inline int bb_snd_music_snd_ = 0;  // sound slot occupied by current music (0 = none)
inline int bb_snd_music_ch_  = 0;  // channel slot occupied by current music (0 = none)

// Stop and free the currently-playing music track (if any).
inline void bb_StopMusic() {
  if (bb_snd_music_ch_) {
    bb_StopChannel(bb_snd_music_ch_);
    bb_snd_music_ch_ = 0;
  }
  if (bb_snd_music_snd_) {
    bb_FreeSound(bb_snd_music_snd_);
    bb_snd_music_snd_ = 0;
  }
}

// Play a WAV, MP3, or OGG file as looping background music.
// Returns the channel handle (>0) on success, 0 on failure.
// MP3: dr_mp3 (bundled). OGG: stb_vorbis (bundled). WAV: SDL_LoadWAV.
inline int bb_PlayMusic(const bbString& file) {
  bb_StopMusic();  // stop previous track first
  int snd = bb_LoadSound(file);
  if (!snd) return 0;
  int ch = bb_LoopSound(snd);
  if (!ch) { bb_FreeSound(snd); return 0; }
  bb_snd_music_snd_ = snd;
  bb_snd_music_ch_  = ch;
  return ch;
}

// Returns 1 while music is playing (the channel has an active stream), else 0.
inline int bb_MusicPlaying() {
  if (!bb_snd_music_ch_) return 0;
  int alive = bb_ChannelPlaying(bb_snd_music_ch_);
  if (!alive) { bb_snd_music_ch_ = 0; bb_snd_music_snd_ = 0; }
  return alive;
}

// CD audio is deprecated hardware — log a warning and do nothing.
inline void bb_PlayCDTrack(int track) {
  (void)track;
  std::cerr << "[runtime] PlayCDTrack: CD audio is not supported\n";
}

// ---- Sound-level defaults (applied when new channels are spawned) ----

inline void bb_SoundVolume(int snd, float vol) {
  if (snd < 1 || snd >= BB_MAX_SOUNDS || !bb_snd_sounds_[snd].data) return;
  bb_snd_sounds_[snd].vol = vol;
}

inline void bb_SoundPan(int snd, float pan) {
  if (snd < 1 || snd >= BB_MAX_SOUNDS || !bb_snd_sounds_[snd].data) return;
  bb_snd_sounds_[snd].pan = pan;
}

// Pitch in Hz; stored as default for future PlaySound/LoopSound calls.
inline void bb_SoundPitch(int snd, float hz) {
  if (snd < 1 || snd >= BB_MAX_SOUNDS || !bb_snd_sounds_[snd].data) return;
  bb_snd_sounds_[snd].pitch = hz;
}

#endif // BLITZNEXT_BB_SOUND_H
