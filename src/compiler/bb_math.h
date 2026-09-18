#ifndef BLITZNEXT_BB_MATH_H
#define BLITZNEXT_BB_MATH_H

#include <cmath>
#include <cstdlib>
#include <type_traits> // is_integral_v — _bb_mod picks % or fmod

// ============================================================
//  BlitzNext Math Runtime  —  bb_math.h
//
//  All angle-based functions use degrees (matching Blitz3D).
//  Internal helpers use the _bb_ prefix to avoid name clashes.
// ============================================================

// ---- Constant ----

constexpr float bb_Pi = 3.14159265358979323846f;

// ---- Internal helpers ----

inline float _bb_d2r(float deg) { return deg * (bb_Pi / 180.0f); }
inline float _bb_r2d(float rad) { return rad * (180.0f / bb_Pi); }

// ---- Trigonometry: degrees in, degrees/unitless out ----

inline float bb_Sin(float deg)            { return std::sin(_bb_d2r(deg)); }
inline float bb_Cos(float deg)            { return std::cos(_bb_d2r(deg)); }
inline float bb_Tan(float deg)            { return std::tan(_bb_d2r(deg)); }
inline float bb_ASin(float x)            { return _bb_r2d(std::asin(x)); }
inline float bb_ACos(float x)            { return _bb_r2d(std::acos(x)); }
inline float bb_ATan(float x)            { return _bb_r2d(std::atan(x)); }
inline float bb_ATan2(float y, float x)  { return _bb_r2d(std::atan2(y, x)); }

// ---- General math ----

inline float bb_Sqr(float x)   { return std::sqrt(x); }
// Abs und Sgn sind Operatoren, keine Befehle: UniExprNode (compiler/
// exprnode.cpp) gibt ihnen den Typ des Operanden und ruft __bbAbs/__bbSgn fuer
// int, __bbFAbs/__bbFSgn fuer float. Am Original gemessen (BUG-96): Abs(3)/2
// ist 1, Sgn(2.0)/2 ist 0.5, Abs(-2147483648) bleibt -2147483648 (int).
// Ein double (Literal-Arithmetik in C++) ist in Blitz ein float.
inline int   bb_Abs(int x)     { return x >= 0 ? x : static_cast<int>(0u - static_cast<unsigned>(x)); }
inline float bb_Abs(float x)   { return std::fabs(x); }
inline float bb_Abs(double x)  { return std::fabs(static_cast<float>(x)); }
inline float bb_Log(float x)   { return std::log(x); }
// "^" ist im Original immer float: ArithExprNode wandelt beide Seiten auf
// float und ruft __bbFPow = (float)pow(x,y) (bbruntime/basic.cpp). Vorher
// stand hier std::pow, das double liefert - "2^24 + 1 - 2^24" ergab 1 statt 0
// (BUG-97). constexpr, weil "^" auch in Const steht; GCC faltet den Builtin.
constexpr float bb_Pow(float x, float y) {
  return static_cast<float>(__builtin_pow(static_cast<double>(x), static_cast<double>(y)));
}
inline float bb_Log10(float x) { return std::log10(x); }
inline float bb_Exp(float x)   { return std::exp(x); }

// Floor/Ceil liefern float. Der Kommentar hier behauptete das Gegenteil
// ("matches Blitz3D's integer-output semantics") - gemessen am Original meldet
// `blitzcc +k` aber `Floor# ( float# )` und `Ceil# ( float# )`. Mit int als
// Rueckgabetyp wurde aus `a# = Ceil(x#) / 2` eine Ganzzahldivision (BUG-44).
inline float bb_Floor(float x) { return std::floor(x); }
inline float bb_Ceil(float x)  { return std::ceil(x); }

// Int(double/int) — rounds like every other float -> int conversion
// (bb_FloatToInt_ in bb_string.h, BUG-95); Int(string) there uses atoi.
// double overload: handles both float (implicit float→double) and double literals.
// int overload: exact match for integer arguments, avoids int→double promotion.
// Together these eliminate any ambiguity with bb_Int(const bbString&) from bb_string.h.
inline int   bb_Int(double x)  { return bb_FloatToInt_(x); }
inline int   bb_Int(int x)     { return x; }

// Sgn — -1, 0 oder 1 im Typ des Operanden (siehe Abs).
inline int   bb_Sgn(int x)     { return (x > 0) - (x < 0); }
inline float bb_Sgn(float x)   { return x > 0.0f ? 1.0f : (x < 0.0f ? -1.0f : 0.0f); }
inline float bb_Sgn(double x)  { return bb_Sgn(static_cast<float>(x)); }

// Mod — Blitz3D's remainder operator, which C++ "%" only covers for integers.
//
// The reference keeps two runtime functions and picks between them in
// ArithExprNode::translate (compiler/exprnode.cpp): "__bbMod" for two ints,
// "__bbFMod" as soon as one side is a float. bbruntime/basic.cpp defines them
// as "return x%y" and "return (float)fmod(x,y)". ArithExprNode::semant casts
// BOTH sides to float when either one is, so a mixed Mod is a float Mod and
// yields a float — measured at the running original: 7 Mod 2.0 is 1.0, and
// 7.5 Mod 3 is 1.5 (BUG-73).
//
// Both C's "%" and fmod take the sign of the dividend, which the same
// measurement confirms for the original: -7 Mod 3 = -1, 7 Mod -3 = 1,
// -7.5 Mod 2.0 = -1.5. Constant folding in the reference uses the very same
// two operations, so compile time and run time agree there as they do here.
//
// A template rather than four overloads, because a double slips in whenever
// "^" is involved (std::pow returns one) and would be ambiguous between an
// int and a float parameter. A string never reaches this: the semantic pass
// rejects it first with "Operator cannot be applied to strings", word for
// word as the original does.
template <typename A, typename B>
inline auto _bb_mod(A x, B y) {
  if constexpr (std::is_integral_v<A> && std::is_integral_v<B>) return x % y;
  else return (float)std::fmod((double)x, (double)y);
}

// ---- Random Numbers ----
// Wie bbruntime/bbmath.cpp (BUG-116, am Original gemessen 2026-09-17):
// Park-Miller-Generator mit Faktor 48271 (Schrage-Zerlegung), Startzustand
// $1234, SeedRnd maskiert auf 31 Bit und macht aus 0 eine 1, RndSeed liefert
// den aktuellen Zustand. Jeder Zug nimmt nur die unteren 16 Bit.
//
// Rnd(from, to=0) = r*(to-from)+from, also Rnd(max) = max*(1-r), nicht r*max.
// Rand(from, to=1) tauscht die Grenzen, also Rand(max) = Rand(1, max).
// Gerechnet wird in float wie im Original - Rand(1,100000000) liegt sonst um
// einige Einheiten daneben (gemessen). Die Breite to-from+1 laeuft wie in 32-Bit-int ueber (Rand(0,2147483647)
// liefert negative Werte).

inline int bb_rnd_state_ = 0x1234;

inline void  bb_SeedRnd(int seed) {
  seed &= 0x7fffffff;
  bb_rnd_state_ = seed ? seed : 1;
}
inline int   bb_RndSeed()                    { return bb_rnd_state_; }

// Ein Rnd OHNE Argument gibt es nicht: das Original meldet `Rnd# ( from#[,to#] )`
// und lehnt `Rnd()` mit "Not enough parameters" ab (BUG-44). Der Einheitswert
// bleibt als interner Helfer erhalten, damit die beiden Formen ihn teilen; der
// fuehrende Kleinbuchstabe haelt ihn aus der erzeugten Befehlstabelle heraus.
inline float bb_rnd_unit_() {
  const int A = 48271, M = 2147483647, Q = 44488, R = 3399;
  bb_rnd_state_ = A * (bb_rnd_state_ % Q) - R * (bb_rnd_state_ / Q);
  if (bb_rnd_state_ < 0) bb_rnd_state_ += M;
  return (bb_rnd_state_ & 65535) / 65536.0f + (0.5f / 65536.0f);
}
inline float bb_Rnd(float from, float to)    { return bb_rnd_unit_() * (to - from) + from; }
inline float bb_Rnd(float from)              { return bb_Rnd(from, 0.0f); }

inline int   bb_Rand(int from, int to) {
  if (to < from) std::swap(from, to);
  int width = (int)((unsigned)to - (unsigned)from + 1u);
  float f = bb_rnd_unit_() * (float)width;
  int v = (int)f;
  return (int)((unsigned)v + (unsigned)from);
}
inline int   bb_Rand(int from)               { return bb_Rand(from, 1); }

// ---- Min / Max ----

template<typename T> inline T bb_Min(T a, T b) { return a < b ? a : b; }
template<typename T> inline T bb_Max(T a, T b) { return a > b ? a : b; }

#endif // BLITZNEXT_BB_MATH_H
