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

// ---- Das x87-Register des Originals (BUG-163) ----
//
// Die FPU des Originals laeuft im 24-Bit-Genauigkeitsmodus: jede Rechnung
// rundet ihr Ergebnis auf eine float-Mantisse (darum rechnet seit BUG-97
// alles in float). Die transzendenten Befehle (fsin, fcos, fptan, fyl2x ...)
// rechnen aber in voller Genauigkeit, und ihr Ergebnis bleibt ungerundet im
// Register - auch ueber die Rueckgabe von bbSin hinweg, der (float)-Cast in
// bbruntime/bbmath.cpp faellt dort weg. Gemessen: "Sin(30) - 0.5" ist
// 1.26184e-008, "Sin(30) = 0.5" ist falsch, "Sin(a) = Sin(a) * 1.0" auch.
//
// bb_Ext ist so ein Registerwert (long double ist bei MinGW das 80-Bit-
// x87-Format selbst, sonst runden Quotienten wie "Cos(a) / 3" doppelt): eine Rechnung damit liefert wieder float
// (der naechste Schritt rundet), ein Vergleich nimmt den vollen Wert, eine
// Zuweisung an eine float-Variable oder ein Parameter rundet ueber operator
// float. 1360 Werte aus build/trig20260918 gegen das Original: vorher 598
// abweichend, jetzt 29 (ASin, Tan, Exp in der letzten Stelle - die rechnet
// die alte CRT selbst im 24-Bit-Modus, das bauen wir nicht nach).
struct bb_Ext {
  long double v;
  constexpr operator float() const { return static_cast<float>(v); }
};
template <class T> constexpr long double bb_ext_d_(const T &x) { return static_cast<long double>(x); }
constexpr long double bb_ext_d_(const bb_Ext &x) { return x.v; }
template <class A, class B>
using bb_ext_op_ = std::enable_if_t<
    (std::is_same_v<A, bb_Ext> || std::is_same_v<B, bb_Ext>) &&
    (std::is_arithmetic_v<A> || std::is_same_v<A, bb_Ext>) &&
    (std::is_arithmetic_v<B> || std::is_same_v<B, bb_Ext>)>;
#define BB_EXT_ARITH_(OP)                                                   \
  template <class A, class B, class = bb_ext_op_<A, B>>                     \
  inline float operator OP(const A &a, const B &b) {                        \
    return static_cast<float>(bb_ext_d_(a) OP bb_ext_d_(b));                \
  }
#define BB_EXT_CMP_(OP)                                                     \
  template <class A, class B, class = bb_ext_op_<A, B>>                     \
  inline bool operator OP(const A &a, const B &b) {                         \
    return bb_ext_d_(a) OP bb_ext_d_(b);                                    \
  }
BB_EXT_ARITH_(+) BB_EXT_ARITH_(-) BB_EXT_ARITH_(*) BB_EXT_ARITH_(/)
BB_EXT_CMP_(==) BB_EXT_CMP_(!=) BB_EXT_CMP_(<) BB_EXT_CMP_(>)
BB_EXT_CMP_(<=) BB_EXT_CMP_(>=)
#undef BB_EXT_ARITH_
#undef BB_EXT_CMP_
inline bb_Ext operator-(const bb_Ext &a) { return {-a.v}; }
inline bb_Ext operator+(const bb_Ext &a) { return a; }

// Zur Ganzzahl rundet fistp den Registerwert selbst, nicht erst einen float:
// "Sin(30) And 1" ist im Original 1 (0.50000001... -> 1), ueber float waere
// es 0.5 -> 0. Gleichstand zur geraden Zahl, ausserhalb des int-Bereichs
// der x87-Wert "integer indefinite" wie in bb_FloatToInt_.
inline int bb_ext_to_int_(const bb_Ext &x) {
  const long double r = std::nearbyint(x.v);
  if (!std::isfinite(r) || r < -2147483648.0L || r > 2147483647.0L)
    return std::numeric_limits<int>::min();
  return static_cast<int>(r);
}
inline int bb_ToInt(const bb_Ext &x)          { return bb_ext_to_int_(x); }
inline int bb_IntegerContext(const bb_Ext &x) { return bb_ext_to_int_(x); }
inline int bb_Int(const bb_Ext &x)            { return bb_ext_to_int_(x); }

// ---- Trigonometry: degrees in, degrees/unitless out ----
//
// Wie bbruntime/bbmath.cpp: dtor/rtod als float-Konstanten, das Produkt mit
// dtor ist ein gewoehnlicher (gerundeter) Rechenschritt. Bei den
// Umkehrfunktionen rechnet die Multiplikation mit rtod noch in Blitz-Code und
// rundet deshalb; Sqr ebenso, fsqrt haelt sich an den 24-Bit-Modus.
constexpr float bb_dtor_ = 0.0174532925199432957692369076848861f;
constexpr float bb_rtod_ = 57.2957795130823208767981548141052f;
inline bb_Ext bb_Sin(float deg) { return {std::sin(static_cast<long double>(deg * bb_dtor_))}; }
inline bb_Ext bb_Cos(float deg) { return {std::cos(static_cast<long double>(deg * bb_dtor_))}; }
inline bb_Ext bb_Tan(float deg) { return {std::tan(static_cast<long double>(deg * bb_dtor_))}; }
inline float bb_ASin(float x) { return static_cast<float>(std::asin(static_cast<double>(x)) * bb_rtod_); }
inline float bb_ACos(float x) { return static_cast<float>(std::acos(static_cast<double>(x)) * bb_rtod_); }
inline float bb_ATan(float x) { return static_cast<float>(std::atan(static_cast<double>(x)) * bb_rtod_); }
inline float bb_ATan2(float y, float x) {
  return static_cast<float>(std::atan2(static_cast<double>(y), static_cast<double>(x)) * bb_rtod_);
}

// ---- General math ----

inline float bb_Sqr(float x)   { return static_cast<float>(std::sqrt(static_cast<double>(x))); }
// Abs und Sgn sind Operatoren, keine Befehle: UniExprNode (compiler/
// exprnode.cpp) gibt ihnen den Typ des Operanden und ruft __bbAbs/__bbSgn fuer
// int, __bbFAbs/__bbFSgn fuer float. Am Original gemessen (BUG-96): Abs(3)/2
// ist 1, Sgn(2.0)/2 ist 0.5, Abs(-2147483648) bleibt -2147483648 (int).
// Ein double (Literal-Arithmetik in C++) ist in Blitz ein float.
inline int   bb_Abs(int x)     { return x >= 0 ? x : static_cast<int>(0u - static_cast<unsigned>(x)); }
inline float bb_Abs(float x)   { return std::fabs(x); }
inline float bb_Abs(double x)  { return std::fabs(static_cast<float>(x)); }
inline bb_Ext bb_Log(float x)  { return {std::log(static_cast<long double>(x))}; }
// Ganzzahl durch eine konstante Zweierpotenz: das Original erzeugt dafuer
// "sar" statt "idiv" (munchArith in codegen_x86.cpp; 1<<k fuer k = 0..31, also
// auch $80000000). Fuer negative Zahlen rundet das ab: -33/16 = -3, -1/2 = -1.
// Der Emitter ruft das nur, wenn der Teiler konstant ist und der Zaehler
// nicht (sonst faltet das Original und schneidet ab). Andere Typen: normale
// Division (BUG-162).
template <class L, class R>
inline auto bb_IDivC_(L l, R r) {
  // Jede Ganzzahl-Art zaehlt als Blitz-int: "Const M = $80000000" steht im
  // C++ als -2147483648, und das ist dort ein long long.
  if constexpr (std::is_integral_v<L> && std::is_integral_v<R> &&
                !std::is_same_v<L, bool> && !std::is_same_v<R, bool>) {
    const int li = static_cast<int>(l), ri = static_cast<int>(r);
    const unsigned u = static_cast<unsigned>(ri);
    if (u != 0 && (u & (u - 1)) == 0) {
      int s = 0;
      for (unsigned v = u; v > 1; v >>= 1) ++s;
      return li >> s; // arithmetisch, wie sar
    }
    return li / ri;
  } else {
    return l / r;
  }
}

// "^" ist im Original immer float: ArithExprNode wandelt beide Seiten auf
// float und ruft __bbFPow = (float)pow(x,y) (bbruntime/basic.cpp). Vorher
// stand hier std::pow, das double liefert - "2^24 + 1 - 2^24" ergab 1 statt 0
// (BUG-97). constexpr, weil "^" auch in Const steht; GCC faltet den Builtin.
constexpr float bb_Pow(float x, float y) {
  return static_cast<float>(__builtin_pow(static_cast<double>(x), static_cast<double>(y)));
}
inline bb_Ext bb_Log10(float x) { return {std::log10(static_cast<long double>(x))}; }
inline bb_Ext bb_Exp(float x)   { return {std::exp(static_cast<long double>(x))}; }

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
