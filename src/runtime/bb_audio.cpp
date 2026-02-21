
#include "api.h"
#include <iostream>
#include <vector>


// #define BB_ENABLE_AUDIO // Uncomment to enable SDL_mixer dependency

#ifdef BB_ENABLE_AUDIO
#include <SDL3/SDL_mixer.h>
#endif

// =============================================================================
// Audio System State
// =============================================================================

struct bb_sound {
#ifdef BB_ENABLE_AUDIO
  Mix_Chunk *chunk = nullptr;
#endif
  std::string filename;
};

static std::vector<bb_sound *> g_sounds;

// =============================================================================
// Commands
// =============================================================================

bb_int loadsound(bb_string file) {
#ifdef BB_ENABLE_AUDIO
  Mix_Chunk *chunk = Mix_LoadWAV(file.c_str());
  if (!chunk) {
    std::cerr << "Failed to load sound: " << file << " (" << Mix_GetError()
              << ")" << std::endl;
    return 0;
  }
  bb_sound *snd = new bb_sound();
  snd->chunk = chunk;
  snd->filename = file;
  g_sounds.push_back(snd);
  return (bb_int)g_sounds.size();
#else
  std::cout << "[Audio Stub] LoadSound(\"" << file << "\")" << std::endl;
  // Return a fake handle so logic can continue
  bb_sound *snd = new bb_sound();
  snd->filename = file;
  g_sounds.push_back(snd);
  return (bb_int)g_sounds.size();
#endif
}

bb_int playsound(bb_int sound_handle) {
  if (sound_handle < 1 || sound_handle > (bb_int)g_sounds.size())
    return 0;
#ifdef BB_ENABLE_AUDIO
  bb_sound *snd = g_sounds[sound_handle - 1];
  if (snd && snd->chunk) {
    return Mix_PlayChannel(-1, snd->chunk, 0);
  }
  return 0;
#else
  std::cout << "[Audio Stub] PlaySound(" << sound_handle << ")" << std::endl;
  return 0;
#endif
}

void freesound(bb_int sound_handle) {
  if (sound_handle < 1 || sound_handle > (bb_int)g_sounds.size())
    return;
  bb_sound *snd = g_sounds[sound_handle - 1];
  if (snd) {
#ifdef BB_ENABLE_AUDIO
    if (snd->chunk)
      Mix_FreeChunk(snd->chunk);
#endif
    delete snd;
    g_sounds[sound_handle - 1] = nullptr;
  }
}

void soundvolume(bb_int sound_handle, bb_float volume) {
  if (sound_handle < 1 || sound_handle > (bb_int)g_sounds.size())
    return;
#ifdef BB_ENABLE_AUDIO
  bb_sound *snd = g_sounds[sound_handle - 1];
  if (snd && snd->chunk) {
    Mix_VolumeChunk(snd->chunk, (int)(volume * 128));
  }
#else
  // std::cout << "[Audio Stub] SoundVolume(" << sound_handle << ", " << volume
  // << ")" << std::endl;
#endif
}

void soundpitch(bb_int sound_handle, bb_int pitch) {
  // SDL_mixer doesn't support pitch easily on chunks without effects
}

void loopsound(bb_int sound_handle) {
  // Logic needs to be stored in bb_sound and applied at PlaySound time
}
