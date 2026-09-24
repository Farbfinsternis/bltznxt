#ifndef BB_OBJECT_H
#define BB_OBJECT_H

// Referenzen auf Type-Objekte (BUG-104).
//
// Das Original zaehlt Referenzen (bbruntime/basic.cpp): jede Variable, jedes
// Feld und jedes Array-Element, das ein Objekt haelt, zaehlt mit
// (_bbObjStore/_bbObjRelease), die Typliste selbst haelt eine Referenz
// (ref_cnt=1 bei New). "Delete" gibt nur die Felder frei und setzt
// obj->fields=0; das Objekt bleibt als leere Huelle bestehen und in der Liste,
// bis die letzte Referenz verschwindet. Beobachtbar (gemessen am 2026-09-24,
// build/obj20260924/werte.bb):
//   - jede Referenz auf ein geloeschtes Objekt ist gleich Null, zwei
//     geloeschte Objekte sind untereinander gleich (_bbObjCompare vergleicht
//     die Felder, nicht die Objekte);
//   - For Each, First, Last, After und Before ueberspringen geloeschte
//     Objekte; After/Before auf ein geloeschtes Objekt selbst gehen;
//   - ein zweites Delete tut nichts.
// Ein Feldzugriff auf Null oder ein geloeschtes Objekt ist ohne Debug-Modus
// ein Speicherzugriffsfehler; wir pruefen immer wie der Debug-Modus
// (FieldVarNode::translate) und melden "Object does not exist".
//
// Jeder Type bekommt dafuer __rc__ (Referenzen), __alive__ (Felder gueltig)
// und die statische Funktion bb_free_ (aus der Liste nehmen, freigeben).

#include <cstddef>
#include <type_traits>
#include <array>
#include <unordered_map>

template <class T>
inline void bb_obj_release_(T *p) {
  if (p && --p->__rc__ == 0) T::bb_free_(p);
}

template <class T>
class bb_ref {
  T *p_ = nullptr;

  // Das lebende Objekt oder Null - danach vergleicht das Original.
  T *live_() const { return (p_ && p_->__alive__) ? p_ : nullptr; }

public:
  bb_ref() = default;
  bb_ref(std::nullptr_t) {}
  // Der Emitter schreibt Null als 0 ("Return Null" -> "return 0;"). Ohne
  // diesen Konstruktor passte 0 gleich gut auf nullptr_t und T*.
  bb_ref(int) {}
  bb_ref(T *p) : p_(p) { if (p_) ++p_->__rc__; }
  bb_ref(const bb_ref &o) : p_(o.p_) { if (p_) ++p_->__rc__; }
  bb_ref(bb_ref &&o) noexcept : p_(o.p_) { o.p_ = nullptr; }
  ~bb_ref() { bb_obj_release_(p_); }

  bb_ref &operator=(const bb_ref &o) {
    T *old = p_;
    p_ = o.p_;
    if (p_) ++p_->__rc__;   // zuerst, falls es dasselbe Objekt ist (_bbObjStore)
    bb_obj_release_(old);
    return *this;
  }
  bb_ref &operator=(bb_ref &&o) noexcept {
    if (this != &o) {
      T *old = p_;
      p_ = o.p_;
      o.p_ = nullptr;
      bb_obj_release_(old);
    }
    return *this;
  }
  bb_ref &operator=(std::nullptr_t) {
    T *old = p_;
    p_ = nullptr;
    bb_obj_release_(old);
    return *this;
  }

  // Auch ein geloeschtes Objekt: After/Before und Insert arbeiten damit.
  T *get() const { return p_; }

  // Feldzugriff wie im Debug-Modus des Originals.
  T *operator->() const {
    if (!live_()) bb_RuntimeError("Object does not exist");
    return p_;
  }

  explicit operator bool() const { return live_() != nullptr; }

  friend bool operator==(const bb_ref &a, const bb_ref &b) { return a.live_() == b.live_(); }
  friend bool operator!=(const bb_ref &a, const bb_ref &b) { return a.live_() != b.live_(); }
  friend bool operator==(const bb_ref &a, std::nullptr_t) { return !a.live_(); }
  friend bool operator!=(const bb_ref &a, std::nullptr_t) { return a.live_() != nullptr; }
  friend bool operator==(std::nullptr_t, const bb_ref &a) { return !a.live_(); }
  friend bool operator!=(std::nullptr_t, const bb_ref &a) { return a.live_() != nullptr; }
};

// Nachbar in der Liste, geloeschte uebersprungen (_bbObjNext/_bbObjPrev).
template <class T>
inline T *bb_obj_next_(T *p) {
  if (!p) return nullptr;
  do p = p->__next__; while (p && !p->__alive__);
  return p;
}

template <class T>
inline T *bb_obj_prev_(T *p) {
  if (!p) return nullptr;
  do p = p->__prev__; while (p && !p->__alive__);
  return p;
}

template <class T>
inline T *bb_obj_first_(T *head) {
  while (head && !head->__alive__) head = head->__next__;
  return head;
}

template <class T>
inline T *bb_obj_last_(T *tail) {
  while (tail && !tail->__alive__) tail = tail->__prev__;
  return tail;
}

// Der rohe Zeiger hinter einer Referenz oder einem Zeiger aus New/First/...
template <class T> inline T *bb_obj_ptr_(T *p) { return p; }
template <class T> inline T *bb_obj_ptr_(const bb_ref<T> &r) { return r.get(); }

// Handle/Object wie _bbObjToHandle/_bbObjFromHandle (BUG-101): ein Objekt
// bekommt beim ersten Handle die naechste Nummer (ab 1) und behaelt sie;
// Null und geloeschte Objekte haben 0. Object.T liefert das Objekt nur,
// wenn die Nummer bekannt ist und zu einem T gehoert, sonst Null. Delete
// streicht die Nummer (_bbObjDelete).
struct bb_obj_handle_entry_ { const void *obj; const void *type; };
inline std::unordered_map<int, bb_obj_handle_entry_> bb_handle_map_;
inline std::unordered_map<const void *, int> bb_object_map_;
inline int bb_next_handle_ = 0;
template <class T> inline const char bb_obj_type_tag_ = 0;

inline void bb_obj_forget_handle_(const void *p) {
  auto it = bb_object_map_.find(p);
  if (it == bb_object_map_.end()) return;
  bb_handle_map_.erase(it->second);
  bb_object_map_.erase(it);
}

// Handle Null: der Emitter schreibt Null als 0.
inline int bb_obj_handle_(int) { return 0; }

template <class X>
inline int bb_obj_handle_(const X &x) {
  auto *p = bb_obj_ptr_(x);
  if (!p || !p->__alive__) return 0;
  using T = std::remove_pointer_t<decltype(p)>;
  auto it = bb_object_map_.find(p);
  if (it != bb_object_map_.end()) return it->second;
  ++bb_next_handle_;
  bb_object_map_[p] = bb_next_handle_;
  bb_handle_map_[bb_next_handle_] = { p, &bb_obj_type_tag_<T> };
  return bb_next_handle_;
}

template <class T>
inline T *bb_obj_from_handle_(int h) {
  auto it = bb_handle_map_.find(h);
  if (it == bb_handle_map_.end()) return nullptr;
  if (it->second.type != &bb_obj_type_tag_<T>) return nullptr;
  return static_cast<T *>(const_cast<void *>(it->second.obj));
}

// _bbObjDelete fuer jeden Typ: Null und ein schon geloeschtes Objekt tun
// nichts; sonst Felder freigeben (T::bb_clear_), als geloescht markieren und
// die Referenz der Liste abgeben. Ohne Typnamen im Emitter, damit "Delete"
// mit jedem Ausdruck geht - Feld, Array-Element, New, Aufruf (BUG-105).
// Wer eine Referenz uebergibt, haelt das Objekt waehrend des Aufrufs am Leben.
template <class X>
inline void bb_obj_delete_(const X &x) {
  auto *p = bb_obj_ptr_(x);
  if (!p || !p->__alive__) return;
  using T = std::remove_pointer_t<decltype(p)>;
  bb_ref<T> hold(p);        // Felder duerfen p nicht vorzeitig freigeben
  p->__alive__ = false;
  bb_obj_forget_handle_(p);
  T::bb_clear_(p);
  bb_obj_release_(p);       // die Referenz der Liste
}

// Insert: umhaengen, auch ein geloeschtes Objekt (_bbObjInsBefore/After).
template <class A, class B>
inline void bb_obj_insert_before_(const A &a, const B &b) {
  auto *o = bb_obj_ptr_(a);
  auto *t = bb_obj_ptr_(b);
  if (!o || !t || o == t) return;
  std::remove_pointer_t<decltype(o)>::bb_insert_before_(o, t);
}

template <class A, class B>
inline void bb_obj_insert_after_(const A &a, const B &b) {
  auto *o = bb_obj_ptr_(a);
  auto *t = bb_obj_ptr_(b);
  if (!o || !t || o == t) return;
  std::remove_pointer_t<decltype(o)>::bb_insert_after_(o, t);
}

// Str(obj) wie _bbObjToStr: die Felder in eckigen Klammern, Kommazahlen wie
// bei Str, Zeichenketten in Anfuehrungszeichen, Objektfelder rekursiv, Null
// und geloeschte Objekte als [NULL], der Ausgangspunkt als [ROOT], ab Tiefe 8
// "....", Array-Felder als "???" (gemessen 2026-09-24, str_formen).
inline const void *bb_obj_str_root_ = nullptr;
inline int bb_obj_str_depth_ = 0;

template <class T> bbString bb_obj_to_str_(T *p);

inline bbString bb_obj_field_str_(int v) { return bb_Str(v); }
inline bbString bb_obj_field_str_(float v) { return bb_Str((double)v); }
inline bbString bb_obj_field_str_(const bbString &s) { return "\"" + s + "\""; }
template <class U>
inline bbString bb_obj_field_str_(const bb_ref<U> &r) { return bb_obj_to_str_(r.get()); }
template <class U, std::size_t N>
inline bbString bb_obj_field_str_(const std::array<U, N> &) { return "???"; }

template <class T>
bbString bb_obj_to_str_(T *p) {
  if (!p || !p->__alive__) return "[NULL]";
  if (p == bb_obj_str_root_) return "[ROOT]";
  if (bb_obj_str_depth_ == 8) return "....";
  ++bb_obj_str_depth_;
  const void *old = bb_obj_str_root_;
  if (!bb_obj_str_root_) bb_obj_str_root_ = p;
  bbString s = "[" + T::bb_tostr_(p) + "]";
  bb_obj_str_root_ = old;
  --bb_obj_str_depth_;
  return s;
}

template <class T>
inline bbString bb_Str(const bb_ref<T> &r) { return bb_obj_to_str_(r.get()); }
template <class T, class = decltype(&T::bb_tostr_)>
inline bbString bb_Str(T *p) { return bb_obj_to_str_(p); }

// After/Before: Null ist "Object does not exist" (Debug-Modus des Originals),
// ein geloeschtes Objekt geht (AfterNode::translate prueft nur den Zeiger).
// Fuer Referenzen und fuer rohe Zeiger aus First/Last/After/Before.
template <class T>
inline T *bb_obj_after_(T *p) {
  if (!p) bb_RuntimeError("Object does not exist");
  return bb_obj_next_(p);
}
template <class T>
inline T *bb_obj_after_(const bb_ref<T> &o) { return bb_obj_after_(o.get()); }
template <class T>
inline T *bb_obj_before_(T *p) {
  if (!p) bb_RuntimeError("Object does not exist");
  return bb_obj_prev_(p);
}
template <class T>
inline T *bb_obj_before_(const bb_ref<T> &o) { return bb_obj_before_(o.get()); }

#endif // BB_OBJECT_H
