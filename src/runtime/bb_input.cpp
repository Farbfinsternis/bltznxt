// =============================================================================
// bb_input.cpp — Input handling (KeyDown, KeyHit, scancode mapping).
// =============================================================================

#include "bb_input.h"
#include "bb_globals.h"

// Blitz3D DirectInput scancode -> SDL scancode
static SDL_Scancode bb_to_sdl_scancode(bb_int key) {
  switch (key) {
  case 1:
    return SDL_SCANCODE_ESCAPE;
  case 2:
    return SDL_SCANCODE_1;
  case 3:
    return SDL_SCANCODE_2;
  case 4:
    return SDL_SCANCODE_3;
  case 5:
    return SDL_SCANCODE_4;
  case 6:
    return SDL_SCANCODE_5;
  case 7:
    return SDL_SCANCODE_6;
  case 8:
    return SDL_SCANCODE_7;
  case 9:
    return SDL_SCANCODE_8;
  case 10:
    return SDL_SCANCODE_9;
  case 11:
    return SDL_SCANCODE_0;
  case 14:
    return SDL_SCANCODE_BACKSPACE;
  case 15:
    return SDL_SCANCODE_TAB;
  case 28:
    return SDL_SCANCODE_RETURN;
  case 29:
    return SDL_SCANCODE_LCTRL;
  case 42:
    return SDL_SCANCODE_LSHIFT;
  case 54:
    return SDL_SCANCODE_RSHIFT;
  case 56:
    return SDL_SCANCODE_LALT;
  case 57:
    return SDL_SCANCODE_SPACE;
  case 200:
    return SDL_SCANCODE_UP;
  case 208:
    return SDL_SCANCODE_DOWN;
  case 203:
    return SDL_SCANCODE_LEFT;
  case 205:
    return SDL_SCANCODE_RIGHT;
  // Letters
  case 16:
    return SDL_SCANCODE_Q;
  case 17:
    return SDL_SCANCODE_W;
  case 18:
    return SDL_SCANCODE_E;
  case 19:
    return SDL_SCANCODE_R;
  case 20:
    return SDL_SCANCODE_T;
  case 21:
    return SDL_SCANCODE_Y;
  case 22:
    return SDL_SCANCODE_U;
  case 23:
    return SDL_SCANCODE_I;
  case 24:
    return SDL_SCANCODE_O;
  case 25:
    return SDL_SCANCODE_P;
  case 30:
    return SDL_SCANCODE_A;
  case 31:
    return SDL_SCANCODE_S;
  case 32:
    return SDL_SCANCODE_D;
  case 33:
    return SDL_SCANCODE_F;
  case 34:
    return SDL_SCANCODE_G;
  case 35:
    return SDL_SCANCODE_H;
  case 36:
    return SDL_SCANCODE_J;
  case 37:
    return SDL_SCANCODE_K;
  case 38:
    return SDL_SCANCODE_L;
  case 44:
    return SDL_SCANCODE_Z;
  case 45:
    return SDL_SCANCODE_X;
  case 46:
    return SDL_SCANCODE_C;
  case 47:
    return SDL_SCANCODE_V;
  case 48:
    return SDL_SCANCODE_B;
  case 49:
    return SDL_SCANCODE_N;
  case 50:
    return SDL_SCANCODE_M;
  default:
    return SDL_SCANCODE_UNKNOWN;
  }
}

bb_int keyhit(bb_int key) {
  SDL_Scancode sc = bb_to_sdl_scancode(key);
  if (sc == SDL_SCANCODE_UNKNOWN)
    return 0;

  if (g_keyHits[sc] > 0) {
    g_keyHits[sc] = 0;
    return 1;
  }
  return 0;
}

bb_int keydown(bb_int key) {
  SDL_Scancode sc = bb_to_sdl_scancode(key);
  if (sc == SDL_SCANCODE_UNKNOWN)
    return 0;
  return g_keyState[sc] ? 1 : 0;
}

bb_int mousex() { return g_mouseX; }
bb_int mousey() { return g_mouseY; }

bb_int mousedown(bb_int btn) {
  if (btn < 0 || btn > 3)
    return 0;
  return g_mouseState[btn];
}

bb_int mousehit(bb_int btn) {
  if (btn < 0 || btn > 3)
    return 0;
  if (g_mouseHits[btn]) {
    g_mouseHits[btn] = 0;
    return 1;
  }
  return 0;
}

void flushmouse() {
  for (int i = 0; i <= 3; i++) {
    g_mouseHits[i] = 0;
  }
}
void flushkeys() {
  for (int i = 0; i < SDL_SCANCODE_COUNT; i++) {
    g_keyHits[i] = 0;
  }
}

void hidepointer() { SDL_HideCursor(); }

void showpointer() { SDL_ShowCursor(); }
