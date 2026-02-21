// =============================================================================
// bb_arith.cpp — Arithmetic & String helper overloads.
// =============================================================================

#include "bb_arith.h"
#include <string>

bb_int bbAdd(bb_int a, bb_int b) { return a + b; }
bb_float bbAdd(bb_float a, bb_float b) { return a + b; }
bb_float bbAdd(bb_int a, bb_float b) { return a + b; }
bb_float bbAdd(bb_float a, bb_int b) { return a + b; }

bb_value bbAdd(bb_value a, bb_value b) {
  if (std::holds_alternative<bb_string>(a.v) ||
      std::holds_alternative<bb_string>(b.v)) {
    return (bb_string)a + (bb_string)b;
  }
  if (std::holds_alternative<bb_float>(a.v) ||
      std::holds_alternative<bb_float>(b.v)) {
    return (bb_float)a + (bb_float)b;
  }
  return (bb_int)a + (bb_int)b;
}

bb_int bbSub(bb_int a, bb_int b) { return a - b; }
bb_float bbSub(bb_float a, bb_float b) { return a - b; }
bb_float bbSub(bb_int a, bb_float b) { return a - b; }
bb_float bbSub(bb_float a, bb_int b) { return a - b; }

bb_int bbMul(bb_int a, bb_int b) { return a * b; }
bb_float bbMul(bb_float a, bb_float b) { return a * b; }
bb_float bbMul(bb_int a, bb_float b) { return a * b; }
bb_float bbMul(bb_float a, bb_int b) { return a * b; }

bb_int bbDiv(bb_int a, bb_int b) { return b ? a / b : 0; }
bb_float bbDiv(bb_float a, bb_float b) { return a / b; }

bb_int bbMod(bb_int a, bb_int b) { return b ? a % b : 0; }
#include <cmath>
#include <ctime>
#include <random>

// Static random engine
static std::mt19937 g_rng(time(NULL));

void seedrnd(bb_int seed) { g_rng.seed((uint32_t)seed); }

bb_int rand(bb_int low, bb_int high) {
  if (high < low)
    std::swap(low, high);
  std::uniform_int_distribution<bb_int> dist(low, high);
  return dist(g_rng);
}

bb_float rnd(bb_float low, bb_float high) {
  if (high < low)
    std::swap(low, high);
  std::uniform_real_distribution<bb_float> dist(low, high);
  return dist(g_rng);
}

bb_int bb_abs(bb_int val) { return val < 0 ? -val : val; }
bb_int bb_sgn(bb_int val) { return (val > 0) - (val < 0); }
bb_float bb_sgn(bb_float val) { return (val > 0.0f) - (val < 0.0f); }

// Trig (Degrees)
static constexpr double DEG_TO_RAD = 0.017453292519943295;
static constexpr double RAD_TO_DEG = 57.29577951308232;

bb_float bb_sin(bb_float deg) { return std::sin((double)deg * DEG_TO_RAD); }
bb_float bb_cos(bb_float deg) { return std::cos((double)deg * DEG_TO_RAD); }
bb_float bb_tan(bb_float deg) { return std::tan((double)deg * DEG_TO_RAD); }
bb_float bb_asin(bb_float val) { return std::asin((double)val) * RAD_TO_DEG; }
bb_float bb_acos(bb_float val) { return std::acos((double)val) * RAD_TO_DEG; }
bb_float bb_atan(bb_float val) { return std::atan((double)val) * RAD_TO_DEG; }
bb_float bb_atan2(bb_float y, bb_float x) {
  return std::atan2((double)y, (double)x) * RAD_TO_DEG;
}
bb_float bb_sqr(bb_float val) { return std::sqrt((double)val); }
bb_float bb_floor(bb_float val) { return std::floor((double)val); }
bb_float bb_ceil(bb_float val) { return std::ceil((double)val); }
bb_float bb_abs(bb_float val) { return std::abs((double)val); }
