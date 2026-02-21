// =============================================================================
// bb_timer.cpp — Timer commands (CreateTimer, WaitTimer, FreeTimer).
// =============================================================================

#include "bb_globals.h"
#include <SDL3/SDL.h>
#include <iostream>

bb_int createtimer(bb_float hertz) {
  if (hertz <= 0)
    hertz = 1.0f;
  bb_timer *timer = new bb_timer();
  timer->hertz = (double)hertz;
  timer->period = 1000.0 / timer->hertz;
  timer->last_tick = (double)SDL_GetTicks();
  timer->ticks_accum = 0;

  bb_int handle = ++g_lastTimerHandle;
  g_timers[handle] = timer;
  return handle;
}

bb_int waittimer(bb_int handle) {
  if (g_timers.find(handle) == g_timers.end())
    return 0;
  bb_timer *timer = g_timers[handle];

  double now = (double)SDL_GetTicks();
  double elapsed = now - timer->last_tick;

  if (elapsed < timer->period) {
    uint32_t wait_ms = (uint32_t)(timer->period - elapsed);
    if (wait_ms > 0)
      SDL_Delay(wait_ms);
    now = (double)SDL_GetTicks();
    elapsed = now - timer->last_tick;
  }

  int ticks = (int)(elapsed / timer->period);
  if (ticks < 1)
    ticks = 1;

  timer->last_tick += (double)ticks * timer->period;
  return (bb_int)ticks;
}

void freetimer(bb_int handle) {
  auto it = g_timers.find(handle);
  if (it != g_timers.end()) {
    delete it->second;
    g_timers.erase(it);
  }
}

void bbTimersCleanup() {
  for (auto const &[handle, timer] : g_timers) {
    delete timer;
  }
  g_timers.clear();
}
