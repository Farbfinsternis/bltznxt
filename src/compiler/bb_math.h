#ifndef BLITZNEXT_BB_MATH_H
#define BLITZNEXT_BB_MATH_H

#include <cmath>
#include <cstdlib>   // rand, srand, RAND_MAX

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

// Floor/Ceil return int — matches Blitz3D's integer-output semantics for console
inline int   bb_Floor(float x) { return static_cast<int>(std::floor(x)); }
inline int   bb_Ceil(float x)  { return static_cast<int>(std::ceil(x)); }

// Int(float) — truncate toward zero (distinct from Int(string) in bb_string.h)
inline int   bb_Int(float x)   { return static_cast<int>(x); }

// Sgn — returns sign of x as -1, 0, or 1
inline int   bb_Sgn(float x)   { return (x > 0.0f) - (x < 0.0f); }

// ---- Random Numbers ----
// Blitz3D: Rnd = float result, Rand = integer result
// SeedRnd seeds the generator; RndSeed returns the last seed used.

inline unsigned int bb_rnd_seed_ = 0;

inline void  bb_SeedRnd(int seed)            { bb_rnd_seed_ = static_cast<unsigned int>(seed); std::srand(bb_rnd_seed_); }
inline int   bb_RndSeed()                    { return static_cast<int>(bb_rnd_seed_); }

// Rnd() → [0, 1)   Rnd(max) → [0, max)   Rnd(min, max) → [min, max)
inline float bb_Rnd()                        { return std::rand() / (float)(RAND_MAX + 1u); }
inline float bb_Rnd(float max)               { return bb_Rnd() * max; }
inline float bb_Rnd(float min, float max)    { return min + bb_Rnd() * (max - min); }

// Rand(max) → [1, max]   Rand(min, max) → [min, max]
inline int   bb_Rand(int max)                { return 1 + (std::rand() % max); }
inline int   bb_Rand(int min, int max)       { return min + (std::rand() % (max - min + 1)); }

#endif // BLITZNEXT_BB_MATH_H
