#ifndef BLITZNEXT_BB_MATH_H
#define BLITZNEXT_BB_MATH_H

#include <cmath>
#include <cstdlib>   // rand, srand, RAND_MAX
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
inline float bb_Abs(float x)   { return std::fabs(x); }
inline float bb_Log(float x)   { return std::log(x); }
inline float bb_Log10(float x) { return std::log10(x); }
inline float bb_Exp(float x)   { return std::exp(x); }

// Floor/Ceil liefern float. Der Kommentar hier behauptete das Gegenteil
// ("matches Blitz3D's integer-output semantics") - gemessen am Original meldet
// `blitzcc +k` aber `Floor# ( float# )` und `Ceil# ( float# )`. Mit int als
// Rueckgabetyp wurde aus `a# = Ceil(x#) / 2` eine Ganzzahldivision (BUG-44).
inline float bb_Floor(float x) { return std::floor(x); }
inline float bb_Ceil(float x)  { return std::ceil(x); }

// Int(double/int) — truncate toward zero (distinct from Int(string) in bb_string.h).
// double overload: handles both float (implicit float→double) and double literals.
// int overload: exact match for integer arguments, avoids int→double promotion.
// Together these eliminate any ambiguity with bb_Int(const bbString&) from bb_string.h.
inline int   bb_Int(double x)  { return static_cast<int>(x); }
inline int   bb_Int(int x)     { return x; }

// Sgn — returns sign of x as -1, 0, or 1
inline int   bb_Sgn(float x)   { return (x > 0.0f) - (x < 0.0f); }

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
// Blitz3D: Rnd = float result, Rand = integer result
// SeedRnd seeds the generator; RndSeed returns the last seed used.

inline unsigned int bb_rnd_seed_ = 0;

inline void  bb_SeedRnd(int seed)            { bb_rnd_seed_ = static_cast<unsigned int>(seed); std::srand(bb_rnd_seed_); }
inline int   bb_RndSeed()                    { return static_cast<int>(bb_rnd_seed_); }

// Rnd(max) → [0, max)   Rnd(min, max) → [min, max)
//
// Ein Rnd OHNE Argument gibt es nicht: das Original meldet `Rnd# ( from#[,to#] )`
// und lehnt `Rnd()` mit "Not enough parameters" ab (BUG-44). Der Einheitswert
// bleibt als interner Helfer erhalten, damit die beiden Formen ihn teilen; der
// fuehrende Kleinbuchstabe haelt ihn aus der erzeugten Befehlstabelle heraus.
inline float bb_rnd_unit_()                  { return std::rand() / (float)(RAND_MAX + 1u); }
inline float bb_Rnd(float max)               { return bb_rnd_unit_() * max; }
inline float bb_Rnd(float min, float max)    { return min + bb_rnd_unit_() * (max - min); }

// Rand(max) → [1, max]   Rand(min, max) → [min, max]
inline int   bb_Rand(int max)                { if (max < 1) max = 1; return 1 + (std::rand() % max); }
inline int   bb_Rand(int min, int max)       { if (min > max) std::swap(min, max); return min + (std::rand() % (max - min + 1)); }

// ---- Min / Max ----

template<typename T> inline T bb_Min(T a, T b) { return a < b ? a : b; }
template<typename T> inline T bb_Max(T a, T b) { return a > b ? a : b; }

#endif // BLITZNEXT_BB_MATH_H
