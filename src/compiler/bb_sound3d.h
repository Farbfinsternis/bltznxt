#ifndef BLITZNEXT_BB_SOUND3D_H
#define BLITZNEXT_BB_SOUND3D_H

#include "bb_sound.h"

// ---- 3D / Positional Sound stubs (M37) ----
//
// SDL3 has no native 3D/positional audio API.  These functions store listener
// and per-channel state that a future audio-processing pass could use to
// simulate distance attenuation and stereo panning.  In the current milestone
// all channel/listener operations are stored-only; the state is kept for
// forward compatibility and as a foundation for a future milestone that adds
// real distance-based gain/pan computation.
//
// BB commands starting with "3D" (3DSoundVolume, 3DSoundPan, …) cannot be
// lexed as identifiers by the current lexer (identifiers must start with a
// letter).  Those are exposed only as C++ helpers (bb_3DSoundVolume, …) and
// are available for use from C++ but not directly from .bb source files until
// a lexer extension is added.

// ---- Per-sound 3D defaults ----

// Falloff range: sounds are at full volume inside inner_radius, silent beyond
// outer_radius.  Stored here; unused until a future audio-processing pass.
inline float bb_snd3d_inner_[BB_MAX_SOUNDS] = {};
inline float bb_snd3d_outer_[BB_MAX_SOUNDS] = {};

// ---- Per-channel 3D state ----

struct bb_Chan3D_ {
  float x  = 0.0f, y  = 0.0f, z  = 0.0f;   // world position
  float vx = 0.0f, vy = 0.0f, vz = 0.0f;   // Doppler velocity
};

inline bb_Chan3D_ bb_snd_chan3d_[BB_MAX_CHANNELS] = {};

// ---- Listener state ----

// Position in world space.
inline float bb_snd3d_lx_ = 0.0f, bb_snd3d_ly_ = 0.0f, bb_snd3d_lz_ = 0.0f;

// Forward vector (default: -Z) and up vector (default: +Y).
inline float bb_snd3d_lfx_ =  0.0f, bb_snd3d_lfy_ = 0.0f, bb_snd3d_lfz_ = -1.0f;
inline float bb_snd3d_lux_ =  0.0f, bb_snd3d_luy_ = 1.0f, bb_snd3d_luz_ =  0.0f;

// Velocity (Doppler simulation).
inline float bb_snd3d_lvx_ = 0.0f, bb_snd3d_lvy_ = 0.0f, bb_snd3d_lvz_ = 0.0f;

// ---- Sound API ----

// Load a sound file for 3D playback.
// Identical to LoadSound — the handle is used with PlaySound / LoopSound and
// then positioned via Channel3DPosition.
inline int bb_Load3DSound(const bbString& file) {
  return bb_LoadSound(file);
}

// Set the default falloff range for a 3D sound.
// inner: radius (units) of full-volume sphere.
// outer: radius of silence.  Between inner and outer the gain fades linearly.
// Values are stored for a future audio-processing pass; not applied at runtime yet.
inline void bb_SoundRange(int snd, float inner, float outer) {
  if (snd < 1 || snd >= BB_MAX_SOUNDS || !bb_snd_sounds_[snd].data) return;
  bb_snd3d_inner_[snd] = inner;
  bb_snd3d_outer_[snd] = outer;
}

// ---- Channel 3D state ----

// Set the world-space position of a playing channel.
// Stub: stores position; no automatic volume/pan update yet.
inline void bb_Channel3DPosition(int ch, float x, float y, float z) {
  if (ch < 1 || ch >= BB_MAX_CHANNELS) return;
  bb_snd_chan3d_[ch].x = x;
  bb_snd_chan3d_[ch].y = y;
  bb_snd_chan3d_[ch].z = z;
}

// Set the Doppler velocity of a playing channel (stub; stored only).
inline void bb_Channel3DVelocity(int ch, float vx, float vy, float vz) {
  if (ch < 1 || ch >= BB_MAX_CHANNELS) return;
  bb_snd_chan3d_[ch].vx = vx;
  bb_snd_chan3d_[ch].vy = vy;
  bb_snd_chan3d_[ch].vz = vz;
}

// ---- Listener API ----

// Set listener world-space position.
inline void bb_ListenerPosition(float x, float y, float z) {
  bb_snd3d_lx_ = x;  bb_snd3d_ly_ = y;  bb_snd3d_lz_ = z;
}

// Set listener orientation: forward vector (fx, fy, fz) and up vector (ux, uy, uz).
inline void bb_ListenerOrientation(float fx, float fy, float fz,
                                   float ux, float uy, float uz) {
  bb_snd3d_lfx_ = fx; bb_snd3d_lfy_ = fy; bb_snd3d_lfz_ = fz;
  bb_snd3d_lux_ = ux; bb_snd3d_luy_ = uy; bb_snd3d_luz_ = uz;
}

// Set listener Doppler velocity (stub; stored only).
inline void bb_ListenerVelocity(float vx, float vy, float vz) {
  bb_snd3d_lvx_ = vx; bb_snd3d_lvy_ = vy; bb_snd3d_lvz_ = vz;
}

// ---- WaitSound ----
//
// Block until a channel has finished playing (one-shot drained or stopped).
// Uses a 10 ms polling loop and pumps the audio update so looping/one-shot
// cleanup runs correctly.  Returns immediately when ch=0 (no channel).

inline void bb_WaitSound(int ch) {
  while (bb_ChannelPlaying(ch)) {
    bb_snd_update_();
    SDL_Delay(10);
  }
}

// ---- Internal C++ aliases for "3D"-prefixed BB commands ----
//
// Blitz3D has 3DSoundVolume / 3DSoundPan / 3DChannelVolume / 3DChannelPan but
// identifiers starting with a digit can't be lexed by the current tokeniser.
// Provide C++ helpers here; they delegate to the 2D counterparts since the
// volume/pan semantics are identical.  BB-level exposure requires a lexer
// extension and is deferred to a future milestone.

inline void bb_3DSoundVolume  (int snd, float vol) { bb_SoundVolume(snd, vol); }
inline void bb_3DSoundPan     (int snd, float pan) { bb_SoundPan(snd, pan);   }
inline void bb_3DChannelVolume(int ch,  float vol) { bb_ChannelVolume(ch, vol); }
inline void bb_3DChannelPan   (int ch,  float pan) { bb_ChannelPan(ch, pan);   }

#endif // BLITZNEXT_BB_SOUND3D_H
