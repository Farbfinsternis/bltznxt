#ifndef BB_LISTENER_H
#define BB_LISTENER_H

// ============================================================
// 3D-Klang: CreateListener, EmitSound (Leuchtturm Schritt 6)
// ============================================================
//
// Wie Blitz3D (blitz3d/listener.cpp, object.cpp, world.cpp, gxaudio.cpp):
//
//   CreateListener(parent, rolloff=1, doppler=1, distance=1)
//       Eine Entity, meist an der Kamera; es gibt hoechstens einen
//       ("Listener already created"). FreeEntity gibt ihn wieder frei.
//   EmitSound(sound, entity)
//       Spielt den Klang am Ort der Entity und liefert den Kanal. Ohne
//       Listener "No Listener created". Nur ein mit Load3DSound geladener
//       Klang wird verortet; einen aus LoadSound spielt FMOD als 2D-Klang
//       (FSOUND_2D), also wie PlaySound.
//
// Nachgefuehrt wird bei jedem RenderWorld: ein Kanal folgt seiner Entity,
// solange er klingt, und der Listener seiner Lage und Blickrichtung. Eine
// versteckte Entity nimmt RenderWorld nicht auf - ihre Klaenge bleiben, wo
// sie zuletzt waren; bei verstecktem Listener aendert sich nichts. Wird die
// Entity freigegeben, klingt der Kanal an ihrer letzten Stelle zu Ende.
//
// Was FMOD 3 daraus macht, rechnet bb_snd3d_place_ nach:
//
//   Lautstaerke  min / (min + rolloff * (d - min)) ab d > min, min = 1
//                (die Vorgabe von FMOD); darunter voll
//   Panorama     Anteil der Richtung auf der Rechts-Achse des Listeners;
//                naeher als min laeuft es zur Mitte hin aus
//   Doppler      (c + vl*u) / (c + vs*u), u vom Listener zur Quelle,
//                c = 340 * distance (Schall in Einheiten je Sekunde),
//                Geschwindigkeiten mal doppler
//
// Die Geschwindigkeiten sind die aus Object::endUpdate: Lageaenderung beim
// UpdateWorld geteilt durch dessen elapsed - bei der Vorgabe 1 also je
// UpdateWorld, nicht je Sekunde, genau wie im Original. Ein Programm mit 60
// UpdateWorld je Sekunde bekommt den physikalischen Doppler mit doppler=60.

#include "bb_entity_core.h"
#include "bb_sound.h"
#include "bb_system.h"
#include <cmath>
#include <memory>
#include <unordered_map>
#include <vector>

struct bb_ListenerEntity_ : bb_Entity_ {
  bb_EntityKind_ kind() const override { return bb_EntityKind_::Listener; }
};

inline int   bb_listener_  = 0;
inline float bb_lis_roll_  = 1.0f;
inline float bb_lis_dopp_  = 1.0f;
inline float bb_lis_dist_  = 1.0f;

// Lage beim letzten UpdateWorld und Geschwindigkeit - nur fuer die
// Entities, die es brauchen: der Listener und alles, was klingt.
struct bb_SndVel_ { float p[3]; float v[3]; };
inline std::unordered_map<int, bb_SndVel_> bb_snd_vel_;

struct bb_Emitter_ { int ent; int ch; unsigned serial; };
inline std::vector<bb_Emitter_> bb_emitters_;

inline void bb_snd_track_(int h) {
  if (bb_snd_vel_.count(h)) return;
  const float* w = bb_entity_world_(bb_entity_get_(h));
  bb_snd_vel_[h] = { { w[12], w[13], w[14] }, { 0, 0, 0 } };
}

inline bb_Entity_* bb_listener_ent_() {
  if (!bb_listener_) return nullptr;
  bb_Entity_* e = bb_entity_get_(bb_listener_);
  if (!e) bb_listener_ = 0;           // freigegeben (FreeEntity, ClearWorld)
  return e;
}

// Lautstaerke, Panorama und Doppler eines 3D-Kanals aus den Lagen von
// Listener und Quelle.
inline void bb_snd3d_place_(int ch, int src) {
  bb_Entity_* l = bb_listener_ent_();
  bb_Entity_* s = bb_entity_get_(src);
  if (!l || !s) return;
  const float* lw = bb_entity_world_(l);
  const float lp[3] = { lw[12], lw[13], lw[14] };
  const float ra[3] = { lw[0], lw[1], lw[2] };
  const float* sw = bb_entity_world_(s);
  const float dx = sw[12] - lp[0], dy = sw[13] - lp[1], dz = sw[14] - lp[2];
  const float d = std::sqrt(dx * dx + dy * dy + dz * dz);
  constexpr float MIN = 1.0f;

  float gain = 1.0f;
  if (d > MIN) gain = MIN / (MIN + bb_lis_roll_ * (d - MIN));

  float pan = 0.0f, ratio = 1.0f;
  if (d > 1e-6f) {
    const float rl = std::sqrt(ra[0] * ra[0] + ra[1] * ra[1] + ra[2] * ra[2]);
    if (rl > 0) pan = (dx * ra[0] + dy * ra[1] + dz * ra[2]) / (rl * d);
    if (d < MIN) pan *= d / MIN;
    const float u[3] = { dx / d, dy / d, dz / d };
    auto along = [&](int h) {
      auto it = bb_snd_vel_.find(h);
      if (it == bb_snd_vel_.end()) return 0.0f;
      const float* v = it->second.v;
      return (v[0] * u[0] + v[1] * u[1] + v[2] * u[2]) * bb_lis_dopp_;
    };
    const float c = 340.0f * bb_lis_dist_;
    const float den = c + along(src);
    if (c > 0 && den > 0) ratio = (c + along(bb_listener_)) / den;
    ratio = std::min(std::max(ratio, 0.1f), 10.0f);
  }

  {
    bb_SndLock_ lock;
    bb_Channel_& c = bb_snd_channels_[ch];
    c.gain3d  = gain;
    c.pan3d   = pan;
    c.doppler = ratio;
  }
  bb_snd_apply_(ch);
}

inline int bb_CreateListener(int parent, float rolloff = 1.0f, float doppler = 1.0f,
                             float distance = 1.0f) {
  if (bb_listener_ent_()) bb_RuntimeError("Listener already created");
  bb_snd_ensure_();
  const int h = bb_entity_register_(std::make_unique<bb_ListenerEntity_>(), parent);
  bb_listener_ = h;
  bb_lis_roll_ = rolloff;
  bb_lis_dopp_ = doppler;
  bb_lis_dist_ = distance;
  bb_snd_track_(h);
  return h;
}

inline int bb_EmitSound(int snd, int entity) {
  bb_ent_chk_(entity);
  if (!bb_listener_ent_()) bb_RuntimeError("No Listener created");
  const int ch = bb_PlaySound(snd);
  if (!ch || !bb_snd_sounds_[snd].is3d) return ch;
  {
    bb_SndLock_ lock;
    bb_snd_channels_[ch].is3d = true;
  }
  bb_snd_track_(entity);
  bb_emitters_.push_back({ entity, ch, bb_snd_channels_[ch].serial });
  bb_snd3d_place_(ch, entity);
  return ch;
}

// Am Ende von UpdateWorld: Geschwindigkeiten (Object::endUpdate).
inline void bb_snd3d_update_(float elapsed) {
  for (auto it = bb_snd_vel_.begin(); it != bb_snd_vel_.end();) {
    bb_Entity_* e = bb_entity_get_(it->first);
    if (!e) { it = bb_snd_vel_.erase(it); continue; }
    const float* w = bb_entity_world_(e);
    bb_SndVel_& sv = it->second;
    for (int k = 0; k < 3; ++k) {
      sv.v[k] = elapsed != 0 ? (w[12 + k] - sv.p[k]) / elapsed : 0.0f;
      sv.p[k] = w[12 + k];
    }
    ++it;
  }
}

// Am Ende von RenderWorld: die Kanaele folgen ihren Entities.
inline void bb_snd3d_render_() {
  if (bb_emitters_.empty()) return;
  bb_Entity_* l = bb_listener_ent_();
  const bool listener_ok = l && bb_entity_shown_(l);
  std::vector<bb_Emitter_> keep;
  keep.reserve(bb_emitters_.size());
  for (const bb_Emitter_& em : bb_emitters_) {
    const bb_Channel_& c = bb_snd_channels_[em.ch];
    if (!c.stream || c.serial != em.serial || bb_snd_done_(c)) continue;  // verklungen
    bb_Entity_* e = bb_entity_get_(em.ent);
    if (!e) continue;                            // klingt an der letzten Stelle aus
    if (listener_ok && bb_entity_shown_(e)) bb_snd3d_place_(em.ch, em.ent);
    keep.push_back(em);
  }
  bb_emitters_.swap(keep);
  // Nur noch verfolgen, was klingt oder hoert.
  for (auto it = bb_snd_vel_.begin(); it != bb_snd_vel_.end();) {
    bool used = it->first == bb_listener_;
    for (const bb_Emitter_& em : bb_emitters_) used = used || em.ent == it->first;
    it = used ? std::next(it) : bb_snd_vel_.erase(it);
  }
}

#endif // BB_LISTENER_H
