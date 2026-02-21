#ifndef BB_RUNTIME_API_H
#define BB_RUNTIME_API_H

#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <string>
#include <variant>
#include <vector>

// Primitive types
typedef int64_t bb_int;
typedef double bb_float; // Float precision increase? No, keep float or double.
                         // Blitz3D used float.
typedef std::string bb_string;

// String Comparison
int _bbStrCompare(const std::string &s1, const std::string &s2);

struct bb_value {
  std::variant<bb_int, bb_float, bb_string> v;

  bb_value() : v((bb_int)0) {}
  bb_value(bb_int i) : v(i) {}
  bb_value(bb_float f) : v(f) {}
  bb_value(bb_string s) : v(s) {}

  explicit operator bb_int() const {
    if (std::holds_alternative<bb_int>(v))
      return std::get<bb_int>(v);
    if (std::holds_alternative<bb_float>(v))
      return (bb_int)std::get<bb_float>(v);
    return 0;
  }
  explicit operator bb_float() const {
    if (std::holds_alternative<bb_float>(v))
      return std::get<bb_float>(v);
    if (std::holds_alternative<bb_int>(v))
      return (bb_float)std::get<bb_int>(v);
    return 0.0;
  }
  explicit operator bb_string() const {
    if (std::holds_alternative<bb_string>(v))
      return std::get<bb_string>(v);
    if (std::holds_alternative<bb_int>(v))
      return std::to_string(std::get<bb_int>(v));
    if (std::holds_alternative<bb_float>(v))
      return std::to_string(std::get<bb_float>(v));
    return "";
  }

  bb_value &operator=(bb_int i) {
    v = i;
    return *this;
  }
  bb_value &operator=(bb_float f) {
    v = f;
    return *this;
  }
  bb_value &operator=(bb_string s) {
    v = s;
    return *this;
  }
};

// Array Support (Runtime)
void bbDim(bb_string name, std::initializer_list<bb_int> dims);
bb_value &bbArrayAccess(bb_string name, std::initializer_list<bb_int> dims);

// Data Management
enum bb_data_type { BB_DATA_INT, BB_DATA_FLOAT, BB_DATA_STRING };
struct bb_data_value {
  bb_data_type type;
  bb_int i;
  bb_float f;
  bb_string s;
  bb_data_value(bb_int v) : type(BB_DATA_INT), i(v), f(0.0), s("") {}
  bb_data_value(bb_float v) : type(BB_DATA_FLOAT), i(0), f(v), s("") {}
  bb_data_value(bb_string v) : type(BB_DATA_STRING), i(0), f(0.0), s(v) {}
};

void bbRegisterData(const bb_data_value &val, const std::string &label = "");
void bbRestore(const std::string &label = "");
bb_int bbReadInt();
bb_float bbReadFloat();
bb_string bbReadString();

// Entities are opaque pointers for now, or a base class
struct bb_entity;

// ==========================================================================
// Blitz3D Command Declarations — auto-generated from commands.def
// ==========================================================================
#define T_INT bb_int
#define T_FLOAT bb_float
#define T_STRING bb_string
#define T_VOID void
#define P(n, t) , t n
#define OP(n, t, v) , t n = v
// Strip leading comma: STRIP_FIRST(void , bb_int x , bb_float y) → bb_int x,
// bb_float y
#define STRIP_FIRST_IMPL_(first, ...) __VA_ARGS__
#define STRIP_FIRST_(...) STRIP_FIRST_IMPL_(__VA_ARGS__)
#define CMD(name, ret, params) ret name(STRIP_FIRST_(void params));
#define CMATH(name, ret, params) ret bb_##name(STRIP_FIRST_(void params));
#define CSTR(name, ret, params) ret bb_##name(STRIP_FIRST_(void params));

#include "../commands.def"

#ifndef BB_RUNTIME_COMPILING
#define sgn bb_sgn

// Math
#define sin bb_sin
#define cos bb_cos
#define tan bb_tan
#define asin bb_asin
#define acos bb_acos
#define atan bb_atan
#define atan2 bb_atan2
#define sqr bb_sqr
#define floor bb_floor
#define ceil bb_ceil
#define abs bb_abs

// String
#define left bb_left
#define right bb_right
#define mid bb_mid
#define replace bb_replace
#define instr bb_instr
#define upper bb_upper
#define lower bb_lower
#define trim bb_trim
#define lset bb_lset
#define rset bb_rset
#define chr bb_chr
#define asc bb_asc
#define len bb_len
#define hex bb_hex
#define bin bb_bin
#define str bb_str
#endif
#undef CMD
#undef CMATH
#undef CSTR
#undef P
#undef OP
#undef STRIP_FIRST_
#undef STRIP_FIRST_IMPL_
#undef T_INT
#undef T_FLOAT
#undef T_STRING
#undef T_VOID

// ==========================================================================
// Internal Runtime Init
// ==========================================================================
void bbRuntimeInit();
void bbTimersCleanup();
void bbFontsCleanup();

// Print function (lowercase as emitted by parser)
template <typename T> void print(T val) { std::cout << val << std::endl; }

// String conversion helper
inline bb_string bbToString(bb_int val) { return std::to_string(val); }
inline bb_string bbToString(int val) { return std::to_string(val); }
inline bb_string bbToString(bb_float val) { return std::to_string(val); }
inline bb_string bbToString(bb_string val) { return val; }
inline bb_string bbToString(const char *val) { return bb_string(val); }
inline bb_string bbToString(const bb_value &val) { return (bb_string)val; }
inline bb_string bbToString(bool val) { return std::to_string((int)val); }

// Arithmetic Helpers
bb_int bb_abs(bb_int val);     // Internal helper for integer Abs
bb_float bb_abs(bb_float val); // Internal helper for float Abs
inline bb_int bbAbs(bb_int val) { return bb_abs(val); }
inline bb_float bbAbs(bb_float val) { return bb_abs(val); }

bb_int bb_sgn(bb_int val);
bb_float bb_sgn(bb_float val);
inline bb_int bbSgn(bb_int val) { return bb_sgn(val); }
inline bb_float bbSgn(bb_float val) { return bb_sgn(val); }

bb_int bbAdd(bb_int a, bb_int b);
bb_float bbAdd(bb_float a, bb_float b);
bb_float bbAdd(bb_int a, bb_float b);
bb_float bbAdd(bb_float a, bb_int b);

#include <type_traits>

// String concatenation templates with SFINAE to avoid ambiguity with bb_value
template <typename T, typename = std::enable_if_t<
                          !std::is_same_v<std::decay_t<T>, bb_value> &&
                          !std::is_same_v<std::decay_t<T>, bb_string>>>
bb_string bbAdd(bb_string a, T b) {
  if constexpr (std::is_arithmetic_v<T>) {
    return a + std::to_string(b);
  } else {
    return a + bb_string(b);
  }
}

template <typename T, typename = std::enable_if_t<
                          !std::is_same_v<std::decay_t<T>, bb_value> &&
                          !std::is_same_v<std::decay_t<T>, bb_string>>>
bb_string bbAdd(T a, bb_string b) {
  if constexpr (std::is_arithmetic_v<T>) {
    return std::to_string(a) + b;
  } else {
    return bb_string(a) + b;
  }
}

inline bb_string bbAdd(bb_string a, bb_string b) { return a + b; }
inline bb_string bbAdd(bb_string a, const char *b) { return a + b; }
inline bb_string bbAdd(const char *a, bb_string b) { return bb_string(a) + b; }

// Dedicated bb_value overloads
inline bb_string bbAdd(bb_string a, const bb_value &b) {
  return a + (bb_string)b;
}
inline bb_string bbAdd(const bb_value &a, bb_string b) {
  return (bb_string)a + b;
}
inline bb_string bbAdd(bb_string a, bb_value &b) { return a + (bb_string)b; }
inline bb_string bbAdd(bb_value &a, bb_string b) { return (bb_string)a + b; }
bb_value bbAdd(bb_value a, bb_value b);

bb_int bbSub(bb_int a, bb_int b);
bb_float bbSub(bb_float a, bb_float b);
bb_float bbSub(bb_int a, bb_float b);
bb_float bbSub(bb_float a, bb_int b);

bb_int bbMul(bb_int a, bb_int b);
bb_float bbMul(bb_float a, bb_float b);
bb_float bbMul(bb_int a, bb_float b);
bb_float bbMul(bb_float a, bb_int b);

bb_int bbDiv(bb_int a, bb_int b);
bb_float bbDiv(bb_float a, bb_float b);

bb_int bbMod(bb_int a, bb_int b);

#endif
