#ifndef BLITZNEXT_BB_SOUND_H
#define BLITZNEXT_BB_SOUND_H

#include <SDL3/SDL.h>
#include <iostream>
#include <string>
#include <cctype>
#include "bb_sdl.h"    // bb_sdl_ensure_(), bb_sdl_initialized_, bb_audio_update_hook_
#include "bb_string.h" // bbString
#include <algorithm>
#include <vector>

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

// ---- Audio device and mixer ----
//
// Blitz3D spielt ueber FMOD, das jeden Kanal selbst mischt: Lautstaerke,
// Panorama, Tonhoehe und fuer 3D-Klaenge Entfernung und Richtung. SDL3 kennt
// kein Panorama; deshalb mischt BLTZNXT selbst. Jeder Kanal ist ein
// SDL_AudioStream, der den Klang in das Mischformat (float, Stereo) wandelt
// und dabei Tonhoehe (Frequenzverhaeltnis) und Lautstaerke (Gain) anwendet.
// Diese Stroeme haengen an keinem Geraet; der Rueckruf des Ausgabestroms
// holt sich aus jedem so viel, wie das Geraet braucht, legt das Panorama an
// und addiert alles.
//
// Der Rueckruf laeuft im Audio-Thread. Alles, was einen Kanal anlegt,
// freigibt oder an Panorama und Pause dreht, haelt bb_snd_lock_.

inline SDL_AudioDeviceID bb_snd_dev_  = 0;        // != 0: Klang ist bereit
inline SDL_AudioSpec     bb_snd_spec_ = {};       // Mischformat
inline SDL_AudioStream*  bb_snd_out_  = nullptr;  // Ausgabestrom am Geraet
inline SDL_Mutex*        bb_snd_lock_ = nullptr;

struct bb_SndLock_ {
  bb_SndLock_()  { if (bb_snd_lock_) SDL_LockMutex(bb_snd_lock_); }
  ~bb_SndLock_() { if (bb_snd_lock_) SDL_UnlockMutex(bb_snd_lock_); }
};

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
  bool          is3d  = false;  // mit Load3DSound geladen: EmitSound verortet ihn
  bool          loop  = false;  // LoopSound: jeder neue Kanal wiederholt
};

struct bb_Channel_ {
  SDL_AudioStream* stream  = nullptr;
  int              snd_id  = 0;
  bool             looping = false;
  bool             paused  = false;
  float            gain    = 1.0f;   // Lautstaerke des Programms (ChannelVolume)
  float            pan     = 0.0f;   // Panorama des Programms (ChannelPan), -1..1
  float            ratio   = 1.0f;   // Tonhoehe des Programms als Frequenzverhaeltnis
  // 3D (EmitSound): vom Listener aus berechnet, bb_sound3d.h
  bool             is3d    = false;
  float            gain3d  = 1.0f;
  float            pan3d   = 0.0f;
  float            doppler = 1.0f;
  unsigned         serial  = 0;      // unterscheidet Kanaele im selben Platz
};

inline bb_Sound_   bb_snd_sounds_[BB_MAX_SOUNDS]     = {};
inline unsigned    bb_snd_serial_ = 0;
inline bb_Channel_ bb_snd_channels_[BB_MAX_CHANNELS] = {};

// Lautstaerke und Tonhoehe eines Kanals an seinen Strom geben: die des
// Programms mal die aus 3D.
inline void bb_snd_apply_(int ch) {
  bb_Channel_& c = bb_snd_channels_[ch];
  if (!c.stream) return;
  SDL_SetAudioStreamGain(c.stream, c.gain * (c.is3d ? c.gain3d : 1.0f));
  SDL_SetAudioStreamFrequencyRatio(c.stream, c.ratio * (c.is3d ? c.doppler : 1.0f));
}

// Das Frequenzverhaeltnis fuer `hz` (0 = die eigene Frequenz des Klangs).
inline float bb_snd_ratio_(int snd, float hz) {
  const float orig = (float)bb_snd_sounds_[snd].spec.freq;
  return (hz > 0.0f && orig > 0.0f) ? hz / orig : 1.0f;
}

inline void bb_snd_free_channel_(int ch) {
  bb_SndLock_ lock;
  if (bb_snd_channels_[ch].stream) SDL_DestroyAudioStream(bb_snd_channels_[ch].stream);
  bb_snd_channels_[ch] = bb_Channel_{};
}

// Ein Kanal ist fertig, wenn er nicht wiederholt und nichts mehr zu hoeren hat.
inline bool bb_snd_done_(const bb_Channel_& c) {
  return c.stream && !c.looping && SDL_GetAudioStreamAvailable(c.stream) <= 0;
}

// Der Rueckruf des Ausgabestroms: alle Kanaele mischen.
//
// Panorama wie ein Balance-Regler: in der Mitte beide Seiten voll, zur einen
// Seite hin wird die andere leiser, ganz aussen ist sie still. So klingt ein
// Klang ohne Panorama so laut wie vorher, als jeder Kanal direkt ins Geraet
// lief.
inline void SDLCALL bb_snd_mix_(void*, SDL_AudioStream* out, int additional, int) {
  if (additional <= 0) return;
  const int n = additional / (int)sizeof(float);          // Werte, je Bild zwei
  static thread_local std::vector<float> mix, tmp;
  mix.assign((size_t)n, 0.0f);
  tmp.resize((size_t)n);
  {
    bb_SndLock_ lock;
    for (int i = 1; i < BB_MAX_CHANNELS; ++i) {
      bb_Channel_& c = bb_snd_channels_[i];
      if (!c.stream || c.paused) continue;
      if (c.looping) {
        const bb_Sound_& s = bb_snd_sounds_[c.snd_id];
        if (s.data && SDL_GetAudioStreamQueued(c.stream) < (int)s.len)
          SDL_PutAudioStreamData(c.stream, s.data, (int)s.len);
      }
      const int got = SDL_GetAudioStreamData(c.stream, tmp.data(), n * (int)sizeof(float));
      if (got <= 0) continue;
      const float p = c.is3d ? c.pan3d : c.pan;
      const float l = p > 0.0f ? 1.0f - std::min(p, 1.0f) : 1.0f;
      const float r = p < 0.0f ? 1.0f + std::max(p, -1.0f) : 1.0f;
      const int m = got / (int)sizeof(float);
      for (int k = 0; k + 1 < m; k += 2) {
        mix[(size_t)k]     += tmp[(size_t)k] * l;
        mix[(size_t)k + 1] += tmp[(size_t)k + 1] * r;
      }
    }
  }
  SDL_PutAudioStreamData(out, mix.data(), n * (int)sizeof(float));
}

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
  SDL_AudioSpec want{};
  want.format   = SDL_AUDIO_F32;
  want.channels = 2;
  want.freq     = 48000;
  SDL_AudioDeviceID dev = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
  if (dev) {
    // Mit der Frequenz des Geraets mischen, dann wandelt SDL nicht noch einmal.
    SDL_AudioSpec have{};
    if (SDL_GetAudioDeviceFormat(dev, &have, nullptr) && have.freq > 0) want.freq = have.freq;
    SDL_CloseAudioDevice(dev);
  }
  bb_snd_lock_ = SDL_CreateMutex();
  bb_snd_out_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &want,
                                          bb_snd_mix_, nullptr);
  if (!bb_snd_out_) {
    std::cerr << "[runtime] SDL_OpenAudioDeviceStream failed: " << SDL_GetError() << "\n";
    SDL_DestroyMutex(bb_snd_lock_);
    bb_snd_lock_ = nullptr;
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    return;
  }
  bb_snd_spec_ = want;
  bb_snd_dev_  = SDL_GetAudioStreamDevice(bb_snd_out_);
  SDL_ResumeAudioStreamDevice(bb_snd_out_);
}

// Tear down all channels, free all sound buffers, close the audio device.
inline void bb_snd_quit_() {
  if (bb_snd_out_) {
    SDL_DestroyAudioStream(bb_snd_out_);   // haelt auch den Rueckruf an
    bb_snd_out_ = nullptr;
  }
  for (int i = 1; i < BB_MAX_CHANNELS; ++i) {
    if (bb_snd_channels_[i].stream) SDL_DestroyAudioStream(bb_snd_channels_[i].stream);
    bb_snd_channels_[i] = bb_Channel_{};
  }
  for (int i = 1; i < BB_MAX_SOUNDS; ++i) {
    if (bb_snd_sounds_[i].data) {
      SDL_free(bb_snd_sounds_[i].data);
      bb_snd_sounds_[i] = bb_Sound_{};
    }
  }
  if (bb_snd_dev_) {
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    bb_snd_dev_ = 0;
  }
  if (bb_snd_lock_) {
    SDL_DestroyMutex(bb_snd_lock_);
    bb_snd_lock_ = nullptr;
  }
}

// ---- One-shot cleanup (called via hook) ----
//
// Called from bb_PollEvents() every frame via bb_audio_update_hook_: a
// one-shot channel that has played out is released. Looping channels are
// refilled by the mixer itself.

inline void bb_snd_update_() {
  if (!bb_snd_dev_) return;
  for (int i = 1; i < BB_MAX_CHANNELS; ++i)
    if (bb_snd_done_(bb_snd_channels_[i]) && !bb_snd_channels_[i].paused)
      bb_snd_free_channel_(i);
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
  for (int i = 1; i < BB_MAX_CHANNELS; ++i)
    if (bb_snd_channels_[i].snd_id == snd && bb_snd_channels_[i].stream)
      bb_snd_free_channel_(i);
  SDL_free(bb_snd_sounds_[snd].data);
  bb_snd_sounds_[snd] = bb_Sound_{};
}

// Internal: play sound once (loop=false) or loop forever (loop=true).
// Reuses the first finished one-shot slot; returns channel handle, 0 on failure.
// PlaySound nimmt die Wiederholung vom Klang (LoopSound), PlayMusic erzwingt sie.
inline int bb_play_sound_(int snd, bool loop) {
  bb_snd_ensure_();
  if (!bb_snd_dev_ || snd < 1 || snd >= BB_MAX_SOUNDS ||
      !bb_snd_sounds_[snd].data) return 0;
  // Find a free or finished slot
  int slot = 0;
  for (int i = 1; i < BB_MAX_CHANNELS; ++i) {
    if (!bb_snd_channels_[i].stream) { slot = i; break; }
    if (bb_snd_done_(bb_snd_channels_[i]) && !bb_snd_channels_[i].paused) {
      bb_snd_free_channel_(i);
      slot = i;
      break;
    }
  }
  if (!slot) return 0;
  const bb_Sound_& sd = bb_snd_sounds_[snd];
  SDL_AudioStream* s = SDL_CreateAudioStream(&sd.spec, &bb_snd_spec_);
  if (!s) return 0;
  SDL_PutAudioStreamData(s, sd.data, (int)sd.len);
  if (!loop) SDL_FlushAudioStream(s);   // signal: no more data for one-shot
  bb_Channel_ c{};
  c.stream  = s;
  c.snd_id  = snd;
  c.looping = loop;
  c.gain    = sd.vol;
  c.pan     = sd.pan;
  c.ratio   = bb_snd_ratio_(snd, sd.pitch);
  c.serial  = ++bb_snd_serial_;
  {
    bb_SndLock_ lock;
    bb_snd_channels_[slot] = c;
  }
  bb_snd_apply_(slot);
  return slot;
}

inline int bb_PlaySound(int snd) {
  const bool loop = snd >= 1 && snd < BB_MAX_SOUNDS && bb_snd_sounds_[snd].loop;
  return bb_play_sound_(snd, loop);
}

// LoopSound spielt nichts: es stellt den Klang auf Wiederholung, und jeder
// Kanal, den PlaySound oder EmitSound danach startet, wiederholt ihn bis
// StopChannel (bbaudio.cpp: sound->setLoop(true), FSOUND_LOOP_NORMAL am
// Sample, BUG-184). Eine Anweisung ohne Wert wie im Original (BUG-44).
inline void bb_LoopSound(int snd) {
  if (snd < 1 || snd >= BB_MAX_SOUNDS || !bb_snd_sounds_[snd].data) return;
  bb_snd_sounds_[snd].loop = true;
}

// Stop playback and release the channel slot immediately.
inline void bb_StopChannel(int ch) {
  if (ch < 1 || ch >= BB_MAX_CHANNELS || !bb_snd_channels_[ch].stream) return;
  bb_snd_free_channel_(ch);
}

// ---- Channel control (M35) ----

// Pause: the mixer skips the channel, its position stays.
inline void bb_PauseChannel(int ch) {
  if (ch < 1 || ch >= BB_MAX_CHANNELS || !bb_snd_channels_[ch].stream) return;
  bb_SndLock_ lock;
  bb_snd_channels_[ch].paused = true;
}

inline void bb_ResumeChannel(int ch) {
  if (ch < 1 || ch >= BB_MAX_CHANNELS || !bb_snd_channels_[ch].stream) return;
  bb_SndLock_ lock;
  bb_snd_channels_[ch].paused = false;
}

// Returns 1 while the channel has an active stream (playing or paused), 0 when done.
inline int bb_ChannelPlaying(int ch) {
  if (ch < 1 || ch >= BB_MAX_CHANNELS) return 0;
  return bb_snd_channels_[ch].stream && !bb_snd_done_(bb_snd_channels_[ch]) ? 1 : 0;
}

// Volume: 0.0 (silent) to 1.0 (full).
inline void bb_ChannelVolume(int ch, float vol) {
  if (ch < 1 || ch >= BB_MAX_CHANNELS || !bb_snd_channels_[ch].stream) return;
  bb_snd_channels_[ch].gain = vol;
  bb_snd_apply_(ch);
}

// Pan: -1.0 (left), 0.0 (center), 1.0 (right). Bei einem 3D-Kanal legt der
// Listener das Panorama fest.
inline void bb_ChannelPan(int ch, float pan) {
  if (ch < 1 || ch >= BB_MAX_CHANNELS || !bb_snd_channels_[ch].stream) return;
  bb_SndLock_ lock;
  bb_snd_channels_[ch].pan = pan;
}

// Pitch: frequency in Hz.  Ratio = hz / original_src_freq.
// Die Frequenz ist im Original int ("ChannelPitch channel,pitch"); ein Float
// wandelt an der Aufrufgrenze wie bei jedem int-Parameter (BUG-62).
inline void bb_ChannelPitch(int ch, int hz) {
  if (ch < 1 || ch >= BB_MAX_CHANNELS || !bb_snd_channels_[ch].stream) return;
  int snd = bb_snd_channels_[ch].snd_id;
  if (snd < 1 || snd >= BB_MAX_SOUNDS || hz <= 0) return;
  bb_snd_channels_[ch].ratio = bb_snd_ratio_(snd, (float)hz);
  bb_snd_apply_(ch);
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
  int ch = bb_play_sound_(snd, true);   // LoopSound liefert seit BUG-44
                                        // keinen Kanal mehr
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
// Der Modus ist optional und wird noch nicht ausgewertet - im Original
// `PlayCDTrack ( track[,mode] )` (BUG-44).
inline int bb_PlayCDTrack(int track, int mode = 1) {
  (void)mode;
  (void)track;
  std::cerr << "[runtime] PlayCDTrack: CD audio is not supported\n";
  return 0;   // 0 = kein Track laeuft; die Referenz fuehrt den Befehl mit
              // Klammern und damit als Funktion mit int (BUG-44)
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
// int wie im Original ("SoundPitch sound,pitch", BUG-62).
inline void bb_SoundPitch(int snd, int hz) {
  if (snd < 1 || snd >= BB_MAX_SOUNDS || !bb_snd_sounds_[snd].data) return;
  bb_snd_sounds_[snd].pitch = (float)hz;
}

#endif // BLITZNEXT_BB_SOUND_H
