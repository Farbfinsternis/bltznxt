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
