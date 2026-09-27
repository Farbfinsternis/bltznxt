#ifndef BB_ANIMATION_H
#define BB_ANIMATION_H

// Animation (3D-19): Schluessel, Sequenzen und ihr Abspielen.
//
// Nachgebaut nach blitz3d/animation.cpp, animator.cpp und den
// Animationsbefehlen in bbblitz3d.cpp. Die Aufteilung ist dieselbe:
//
//   bb_AnimKeys_  - die Schluessel **einer** Entity fuer **eine** Sequenz:
//                   Lage, Skalierung, Drehung, je Bildnummer (Animation)
//   bb_Animator_  - sitzt an der Wurzel eines Modells, kennt die Liste der
//                   Entities, die es bewegt, und je Entity und Sequenz ihre
//                   Schluessel; zaehlt die Zeit weiter (Animator)
//
// Lage und Skalierung werden zwischen zwei Schluesseln linear gemischt, die
// Drehung als Quaternion mit Slerp. Vor dem ersten Schluessel gilt der
// erste, nach dem letzten der letzte.
//
// Unsere Entities halten ihre Drehung in Eulerwinkeln, das Original als
// Quaternion. Zwischen beiden wird hier umgerechnet (ueber die Matrix, wie
// RotateEntity sie baut) - damit bleiben PositionEntity, TurnEntity,
// EntityPitch & Co. unveraendert, und eine animierte Entity verhaelt sich
// fuer sie wie jede andere.
//
// Am Original gemessen (2026-09-26, tests/test_3d19_animation.bb), unter
// anderem: nach AddAnimSeq meldet AnimLength 0, bis Animate laeuft (der
// Animator ist frisch zurueckgesetzt); eine Kopie steht still; eine
// versteckte Entity animiert nicht weiter, ein verstecktes Kind unter einem
// animierten Elternteil schon.

#include <cmath>
#include <map>
#include <memory>
#include <vector>
#include "bb_entity_core.h"

// ============================================================
// Quaternion - wie Quat in blitz3d/geom.h
// ============================================================

struct bb_Quat_ { float w = 1, x = 0, y = 0, z = 0; };
struct bb_Vec3_ { float x = 0, y = 0, z = 0; };

// Quat::slerpTo. Liegen beide auf verschiedenen Seiten, wird das Ziel
// umgedreht (kuerzester Weg); sehr nahe beieinander wird linear gemischt.
inline bb_Quat_ bb_quat_slerp_(const bb_Quat_& a, const bb_Quat_& q, float t) {
  bb_Quat_ b = q;
  float d = a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z;
  float s = 1 - t;
  if (d < 0) { b.w = -b.w; b.x = -b.x; b.y = -b.y; b.z = -b.z; d = -d; }
  if (d < 1 - 1e-6f) {
    const float om = acosf(d);
    const float si = sinf(om);
    t = sinf(t * om) / si;
    s = sinf(s * om) / si;
  }
  return { a.w * s + b.w * t, a.x * s + b.x * t, a.y * s + b.y * t, a.z * s + b.z * t };
}

// Matrix(const Quat&): die Spalten i, j, k - bei uns Spalte 0, 1, 2.
inline void bb_quat_to_mat_(const bb_Quat_& q, float m[16]) {
  const float xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
  const float xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
  const float wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;
  m[0] = 1 - 2 * (yy + zz); m[1] = 2 * (xy - wz);     m[2]  = 2 * (xz + wy);     m[3]  = 0;
  m[4] = 2 * (xy + wz);     m[5] = 1 - 2 * (xx + zz); m[6]  = 2 * (yz - wx);     m[7]  = 0;
  m[8] = 2 * (xz - wy);     m[9] = 2 * (yz + wx);     m[10] = 1 - 2 * (xx + yy); m[11] = 0;
  m[12] = 0; m[13] = 0; m[14] = 0; m[15] = 1;
}

// matrixQuat: erst die Achsen senkrecht stellen, dann die uebliche
// Fallunterscheidung nach der groessten Diagonale.
inline bb_Quat_ bb_quat_from_mat_(const float in[16]) {
  float m[16];
  mat4_orthogonalize_(m, in);
  const float ix = m[0], iy = m[1], iz = m[2];
  const float jx = m[4], jy = m[5], jz = m[6];
  const float kx = m[8], ky = m[9], kz = m[10];
  float t = ix + jy + kz, w, x, y, z;
  if (t > 1e-6f) {
    t = sqrtf(t + 1) * 2;
    x = (ky - jz) / t; y = (iz - kx) / t; z = (jx - iy) / t; w = t / 4;
  } else if (ix > jy && ix > kz) {
    t = sqrtf(ix - jy - kz + 1) * 2;
    x = t / 4; y = (jx + iy) / t; z = (iz + kx) / t; w = (ky - jz) / t;
  } else if (jy > kz) {
    t = sqrtf(jy - kz - ix + 1) * 2;
    x = (jx + iy) / t; y = t / 4; z = (ky + jz) / t; w = (iz - kx) / t;
  } else {
    t = sqrtf(kz - jy - ix + 1) * 2;
    x = (iz + kx) / t; y = (ky + jz) / t; z = t / 4; w = (jx - iy) / t;
  }
  return { w, x, y, z };
}

// ---- Lokale Lage einer Entity als Vektor/Quaternion ----
// Entity::getLocalPosition/Scale/Rotation und die set-Gegenstuecke.

inline bb_Vec3_ bb_ent_local_pos_(const bb_Entity_* e) { return { e->px, e->py, e->pz }; }
inline bb_Vec3_ bb_ent_local_scl_(const bb_Entity_* e) { return { e->sx, e->sy, e->sz }; }
inline bb_Quat_ bb_ent_local_rot_(const bb_Entity_* e) {
  float m[16];
  mat4_make_euler_YXZ_(m, e->rx, e->ry, e->rz);
  return bb_quat_from_mat_(m);
}
inline void bb_ent_set_local_pos_(bb_Entity_* e, const bb_Vec3_& v) { e->px = v.x; e->py = v.y; e->pz = v.z; }
inline void bb_ent_set_local_scl_(bb_Entity_* e, const bb_Vec3_& v) { e->sx = v.x; e->sy = v.y; e->sz = v.z; }
// setLocalRotation normiert die Quaternion vorher.
inline void bb_ent_set_local_rot_(bb_Entity_* e, bb_Quat_ q) {
  const float l = sqrtf(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
  if (l > 0) { q.w /= l; q.x /= l; q.y /= l; q.z /= l; }
  float m[16];
  bb_quat_to_mat_(q, m);
  mat4_extract_euler_YXZ_(m, e->rx, e->ry, e->rz);
}

// Entity::setLocalTform: Lage aus der Verschiebung, Skalierung aus den
// Laengen der drei Achsen, Drehung ueber matrixQuat. Eine Spiegelung geht
// dabei verloren - im Original genauso.
inline void bb_ent_set_local_tform_(bb_Entity_* e, const float m[16]) {
  e->px = m[12]; e->py = m[13]; e->pz = m[14];
  e->sx = sqrtf(m[0] * m[0] + m[1] * m[1] + m[2] * m[2]);
  e->sy = sqrtf(m[4] * m[4] + m[5] * m[5] + m[6] * m[6]);
  e->sz = sqrtf(m[8] * m[8] + m[9] * m[9] + m[10] * m[10]);
  bb_ent_set_local_rot_(e, bb_quat_from_mat_(m));
}

// ============================================================
// bb_AnimKeys_ - Animation im Original
// ============================================================

struct bb_AnimKeys_ {
  std::map<int, bb_Vec3_> pos, scl;
  std::map<int, bb_Quat_> rot;
  // Morph-Gewichte (glTF, 3D-24): je Bild ein Wert je Ziel, linear gemischt.
  std::map<int, std::vector<float>> wts;

  bb_AnimKeys_() = default;

  // Animation(const Animation&, first, last): nur die Schluessel im
  // Bereich, um `first` nach vorn geschoben.
  bb_AnimKeys_(const bb_AnimKeys_& t, int first, int last) {
    for (const auto& [f, v] : t.pos) if (f >= first && f <= last) pos[f - first] = v;
    for (const auto& [f, v] : t.scl) if (f >= first && f <= last) scl[f - first] = v;
    for (const auto& [f, v] : t.rot) if (f >= first && f <= last) rot[f - first] = v;
    for (const auto& [f, v] : t.wts) if (f >= first && f <= last) wts[f - first] = v;
  }

  // Zwei Gewichtsfelder mischen; ein fehlender Eintrag zaehlt als 0.
  static std::vector<float> mix_(const std::vector<float>& a, const std::vector<float>& b, float d) {
    std::vector<float> o(std::max(a.size(), b.size()), 0.0f);
    for (size_t i = 0; i < o.size(); ++i) {
      const float x = i < a.size() ? a[i] : 0, y = i < b.size() ? b[i] : 0;
      o[i] = (y - x) * d + x;
    }
    return o;
  }
  std::vector<float> weights(float time) const {
    if (wts.empty()) return {};
    auto next = wts.upper_bound(static_cast<int>(time));
    if (next == wts.begin()) return next->second;
    auto curr = std::prev(next);
    if (next == wts.end()) return curr->second;
    const float d = (time - curr->first) / static_cast<float>(next->first - curr->first);
    return mix_(curr->second, next->second, d);
  }

  // getLinearValue: der naechste Schluessel wird ueber die abgeschnittene
  // Zeit gesucht (upper_bound((int)time)), gemischt wird mit der vollen.
  static bb_Vec3_ linear_(const std::map<int, bb_Vec3_>& keys, float time) {
    auto next = keys.upper_bound(static_cast<int>(time));
    if (next == keys.begin()) return next->second;
    auto curr = std::prev(next);
    if (next == keys.end()) return curr->second;
    const float d = (time - curr->first) / static_cast<float>(next->first - curr->first);
    const bb_Vec3_& a = curr->second;
    const bb_Vec3_& b = next->second;
    return { (b.x - a.x) * d + a.x, (b.y - a.y) * d + a.y, (b.z - a.z) * d + a.z };
  }

  bb_Vec3_ position(float t) const { return pos.empty() ? bb_Vec3_{} : linear_(pos, t); }
  bb_Vec3_ scale(float t)    const { return scl.empty() ? bb_Vec3_{ 1, 1, 1 } : linear_(scl, t); }
  bb_Quat_ rotation(float t) const {
    if (rot.empty()) return {};
    auto next = rot.upper_bound(static_cast<int>(t));
    if (next == rot.begin()) return next->second;
    auto curr = std::prev(next);
    if (next == rot.end()) return curr->second;
    const float d = (t - curr->first) / static_cast<float>(next->first - curr->first);
    return bb_quat_slerp_(curr->second, next->second, d);
  }
};

using bb_AnimKeysRef_ = std::shared_ptr<const bb_AnimKeys_>;

// Schreibzugriff mit Kopie beim Schreiben, wie Animation::write(): mehrere
// Sequenzen und Kopien teilen sich dieselben Schluessel.
inline bb_AnimKeys_& bb_anim_write_(bb_AnimKeysRef_& r) {
  auto neu = r ? std::make_shared<bb_AnimKeys_>(*r) : std::make_shared<bb_AnimKeys_>();
  bb_AnimKeys_& ref = *neu;
  r = std::move(neu);
  return ref;
}

inline size_t bb_anim_npos_(const bb_AnimKeysRef_& r) { return r ? r->pos.size() : 0; }
inline size_t bb_anim_nscl_(const bb_AnimKeysRef_& r) { return r ? r->scl.size() : 0; }
inline size_t bb_anim_nrot_(const bb_AnimKeysRef_& r) { return r ? r->rot.size() : 0; }

// ============================================================
// bb_Animator_ - Animator im Original
// ============================================================

inline constexpr int BB_ANIM_LOOP     = 1;
inline constexpr int BB_ANIM_PINGPONG = 2;
inline constexpr int BB_ANIM_ONESHOT  = 3;
inline constexpr int BB_ANIM_TRANS    = 0x8000;

struct bb_Animator_ {
  struct Anim {
    std::vector<bb_AnimKeysRef_> keys;   // je Sequenz
    // fuer Uebergaenge
    bool     pos = false, scl = false, rot = false, wts = false;
    bb_Vec3_ src_pos, dest_pos, src_scl, dest_scl;
    bb_Quat_ src_rot, dest_rot;
    std::vector<float> src_wts, dest_wts;
  };

  std::vector<int>  seqs;   // Laenge je Sequenz in Bildern
  std::vector<Anim> anims;  // je Entity in `objs`
  std::vector<int>  objs;   // Handles; [0] ist die Wurzel

  int   seq = 0, mode = 0, seq_len = 0;
  float time = 0, speed = 0, trans_time = 0, trans_speed = 0;

  void reset() {
    seq = mode = seq_len = 0;
    time = speed = trans_time = trans_speed = 0;
  }

  // Animator(Object*, frames): die Wurzel und alle Nachfahren, Tiefe zuerst.
  void add_objs(int h) {
    objs.push_back(h);
    if (const bb_Entity_* e = bb_entity_get_(h))
      for (int c : e->children) add_objs(c);
  }

  // Die gesammelten Schluessel jeder Entity werden die neue Sequenz, die
  // Entity selbst faengt danach mit leeren Schluesseln neu an.
  void add_seq(int frames) {
    seqs.push_back(frames);
    for (size_t k = 0; k < objs.size(); ++k) {
      bb_Entity_* e = bb_entity_get_(objs[k]);
      anims[k].keys.push_back(e ? e->anim : nullptr);
      if (e) e->anim = nullptr;
    }
  }

  // Animator::extractSeq. Eine ungueltige Quellsequenz liest im Original
  // hinter das Feld; hier wird sie wie eine ohne Schluessel behandelt.
  void extract_seq(int first, int last, int from) {
    seqs.push_back(last - first);
    for (auto& a : anims) {
      const bool ok = from >= 0 && from < static_cast<int>(a.keys.size()) && a.keys[from];
      a.keys.push_back(ok ? std::make_shared<bb_AnimKeys_>(*a.keys[from], first, last)
                          : nullptr);
    }
  }

  // Animator::addSeqs (LoadAnimSeq): alle Sequenzen eines zweiten Animators
  // anhaengen. Die Entities werden ueber ihren **Namen** zugeordnet, die
  // erste mit gleichem Namen gilt; wer keine Entsprechung hat, bekommt eine
  // Sequenz ohne Schluessel.
  void add_seqs(const bb_Animator_& t) {
    for (size_t n = 0; n < t.seqs.size(); ++n) {
      seqs.push_back(t.seqs[n]);
      for (size_t k = 0; k < objs.size(); ++k) {
        const bb_Entity_* e = bb_entity_get_(objs[k]);
        size_t j = 0;
        for (; j < t.objs.size(); ++j) {
          const bb_Entity_* te = bb_entity_get_(t.objs[j]);
          if (e && te && e->name == te->name) break;
        }
        anims[k].keys.push_back(j < t.objs.size() && n < t.anims[j].keys.size()
                                    ? t.anims[j].keys[n] : nullptr);
      }
    }
  }

  void update_anim() {
    for (size_t k = 0; k < objs.size(); ++k) {
      bb_Entity_* e = bb_entity_get_(objs[k]);
      if (!e || seq >= static_cast<int>(anims[k].keys.size())) continue;
      const bb_AnimKeysRef_& keys = anims[k].keys[seq];
      if (!keys) continue;
      if (!keys->pos.empty()) bb_ent_set_local_pos_(e, keys->position(time));
      if (!keys->scl.empty()) bb_ent_set_local_scl_(e, keys->scale(time));
      if (!keys->rot.empty()) bb_ent_set_local_rot_(e, keys->rotation(time));
      if (!keys->wts.empty()) e->morph_w = keys->weights(time);
    }
  }

  void update_trans() {
    const float t = trans_time;
    for (size_t k = 0; k < objs.size(); ++k) {
      bb_Entity_* e = bb_entity_get_(objs[k]);
      if (!e) continue;
      const Anim& a = anims[k];
      if (a.pos) bb_ent_set_local_pos_(e, { (a.dest_pos.x - a.src_pos.x) * t + a.src_pos.x,
                                            (a.dest_pos.y - a.src_pos.y) * t + a.src_pos.y,
                                            (a.dest_pos.z - a.src_pos.z) * t + a.src_pos.z });
      if (a.scl) bb_ent_set_local_scl_(e, { (a.dest_scl.x - a.src_scl.x) * t + a.src_scl.x,
                                            (a.dest_scl.y - a.src_scl.y) * t + a.src_scl.y,
                                            (a.dest_scl.z - a.src_scl.z) * t + a.src_scl.z });
      if (a.rot) bb_ent_set_local_rot_(e, bb_quat_slerp_(a.src_rot, a.dest_rot, t));
      if (a.wts) e->morph_w = bb_AnimKeys_::mix_(a.src_wts, a.dest_wts, t);
    }
  }

  // Von der jetzigen Lage zur Lage am Anfang der neuen Sequenz.
  void begin_trans() {
    for (size_t k = 0; k < objs.size(); ++k) {
      Anim& a = anims[k];
      bb_Entity_* e = bb_entity_get_(objs[k]);
      const bb_AnimKeysRef_ keys =
          (e && seq < static_cast<int>(a.keys.size())) ? a.keys[seq] : nullptr;
      if ((a.pos = bb_anim_npos_(keys) > 0)) {
        a.src_pos = bb_ent_local_pos_(e); a.dest_pos = keys->position(time);
      }
      if ((a.scl = bb_anim_nscl_(keys) > 0)) {
        a.src_scl = bb_ent_local_scl_(e); a.dest_scl = keys->scale(time);
      }
      if ((a.rot = bb_anim_nrot_(keys) > 0)) {
        a.src_rot = bb_ent_local_rot_(e); a.dest_rot = keys->rotation(time);
      }
      if ((a.wts = keys && !keys->wts.empty())) {
        a.src_wts = e->morph_w; a.dest_wts = keys->weights(time);
      }
    }
  }

  // Animator::setAnimTime. Das Original prueft `seq > size` statt `>=` und
  // liest bei seq == size hinter das Feld; hier ist auch das ungueltig.
  void set_time(float t, int s) {
    if (s < 0 || s >= static_cast<int>(seqs.size())) return;
    mode = 0;
    speed = 0;
    seq = s;
    seq_len = seqs[seq];
    time = static_cast<float>(fmod(t, seq_len));
    if (time < 0) time += seq_len;
    update_anim();
  }

  void animate(int m, float sp, int s, float trans) {
    if (!m && !sp) { mode = 0; return; }
    if (s < 0 || s >= static_cast<int>(seqs.size())) return;
    seq = s;
    mode = m;
    seq_len = seqs[seq];
    speed = sp;
    time = speed >= 0 ? 0.0f : static_cast<float>(seq_len);
    if (trans <= 0) {
      update_anim();
      if (!speed) mode = 0;
      return;
    }
    mode |= BB_ANIM_TRANS;
    trans_time = 0;
    trans_speed = 1 / trans;
    begin_trans();
  }

  void update(float elapsed) {
    if (!mode) return;
    if (mode & BB_ANIM_TRANS) {
      trans_time += trans_speed * elapsed;
      if (trans_time < 1) { update_trans(); return; }
      mode &= 0x7fff;
      if (!mode || !speed) { update_anim(); mode = 0; return; }
    }
    time += speed * elapsed;
    switch (mode) {
    case BB_ANIM_LOOP:
      time = static_cast<float>(fmod(time, seq_len));
      if (time < 0) time += seq_len;
      break;
    case BB_ANIM_PINGPONG:
      time = static_cast<float>(fmod(time, seq_len * 2));
      if (time < 0) time += seq_len * 2;
      if (time >= seq_len) { time = seq_len - (time - seq_len); speed = -speed; }
      break;
    case BB_ANIM_ONESHOT:
      if (time < 0) { time = 0; mode = 0; }
      else if (time >= seq_len) { time = static_cast<float>(seq_len); mode = 0; }
      break;
    }
    update_anim();
  }
};

// Neuer Animator ueber `root` und alle Nachfahren, mit einer ersten Sequenz.
inline std::shared_ptr<bb_Animator_> bb_animator_new_(int root, int frames) {
  auto a = std::make_shared<bb_Animator_>();
  a->add_objs(root);
  a->anims.resize(a->objs.size());
  a->add_seq(frames);
  a->reset();
  return a;
}

// Dasselbe mit einer fest vorgegebenen Liste (Knochen eines .b3d).
inline std::shared_ptr<bb_Animator_> bb_animator_new_(const std::vector<int>& objs, int frames) {
  auto a = std::make_shared<bb_Animator_>();
  a->objs = objs;
  a->anims.resize(a->objs.size());
  a->add_seq(frames);
  a->reset();
  return a;
}

// Animator(Animator*): dieselben Sequenzen und Schluessel, die Entities
// ersetzt durch ihre eben gemachten Kopien, angehalten.
inline std::shared_ptr<bb_Animator_> bb_animator_clone_(const bb_Animator_& t) {
  auto a = std::make_shared<bb_Animator_>();
  a->seqs = t.seqs;
  a->objs.resize(t.objs.size());
  a->anims.resize(t.anims.size());
  for (size_t k = 0; k < t.objs.size(); ++k) {
    const bb_Entity_* e = bb_entity_get_(t.objs[k]);
    a->objs[k] = e ? e->lastCopy : 0;
    a->anims[k].keys = t.anims[k].keys;
  }
  a->reset();
  return a;
}

// Object::animate fuer jedes nicht versteckte Entity, aufgerufen aus
// UpdateWorld. Liefert, ob sich eine Lage geaendert haben kann.
inline bool bb_anim_update_all_(float elapsed) {
  bool any = false;
  for (auto& [h, e] : bb_entities_) {
    bb_Animator_* a = e->animator.get();
    if (!a || !a->mode) continue;
    if (!bb_entity_shown_(e.get())) continue;
    a->update(elapsed);
    any = true;
  }
  return any;
}

// ============================================================
// Befehle
// ============================================================

inline void bb_SetAnimTime(int entity, float time, int anim_seq = 0) {
  bb_Entity_* e = bb_ent_chk_(entity);
  // Der Tippfehler steht so im Original.
  if (!e->animator) bb_RuntimeError("Entity has not animation");
  e->animator->set_time(time, anim_seq);
}

inline void bb_Animate(int entity, int mode = 1, float speed = 1.0f,
                       int sequence = 0, float transition = 0.0f) {
  bb_Entity_* e = bb_ent_chk_(entity);
  if (!e->animator) bb_RuntimeError("Entity has no animation");
  e->animator->animate(mode, speed, sequence, transition);
}

// Die jetzige lokale Lage als Schluessel fuer Bild `frame`. Gesammelt wird
// an der Entity selbst; erst AddAnimSeq macht daraus eine Sequenz.
inline void bb_SetAnimKey(int entity, int frame, int pos_key = 1, int rot_key = 1,
                          int scale_key = 1) {
  bb_Entity_* e = bb_ent_chk_(entity);
  bb_AnimKeys_& k = bb_anim_write_(e->anim);
  if (pos_key)   k.pos[frame] = bb_ent_local_pos_(e);
  if (rot_key)   k.rot[frame] = bb_ent_local_rot_(e);
  if (scale_key) k.scl[frame] = bb_ent_local_scl_(e);
}

// Die erste Sequenz legt den Animator an - ueber die Entity und alle
// Nachfahren, die sie **jetzt** hat. Spaeter angehaengte Kinder gehoeren
// nicht dazu.
inline int bb_AddAnimSeq(int entity, int length) {
  bb_Entity_* e = bb_ent_chk_(entity);
  if (e->animator) e->animator->add_seq(length);
  else             e->animator = bb_animator_new_(entity, length);
  return static_cast<int>(e->animator->seqs.size()) - 1;
}

inline int bb_ExtractAnimSeq(int entity, int first_frame, int last_frame, int anim_seq = 0) {
  bb_Entity_* e = bb_ent_chk_(entity);
  if (!e->animator) return -1;
  e->animator->extract_seq(first_frame, last_frame, anim_seq);
  return static_cast<int>(e->animator->seqs.size()) - 1;
}

inline int bb_AnimSeq(int entity) {
  bb_Entity_* e = bb_ent_chk_(entity);
  return e->animator ? e->animator->seq : -1;
}

inline float bb_AnimTime(int entity) {
  bb_Entity_* e = bb_ent_chk_(entity);
  return e->animator ? e->animator->time : -1.0f;
}

inline int bb_AnimLength(int entity) {
  bb_Entity_* e = bb_ent_chk_(entity);
  return e->animator ? e->animator->seq_len : -1;
}

inline int bb_Animating(int entity) {
  bb_Entity_* e = bb_ent_chk_(entity);
  return (e->animator && e->animator->mode) ? 1 : 0;
}

#endif // BB_ANIMATION_H
