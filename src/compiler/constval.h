#ifndef BLITZNEXT_CONSTVAL_H
#define BLITZNEXT_CONSTVAL_H

#include "bb_numconv.h"
#include <cstdlib>
#include <string>

// Der Wert eines konstanten Ausdrucks, wie ihn das Original beim Uebersetzen
// faltet: IntConstNode, FloatConstNode oder StringConstNode
// (compiler/exprnode.cpp). Die drei Lesarten intValue/floatValue/stringValue
// sind dort die einzige Art, einen solchen Wert in einen anderen Typ zu
// bringen - und damit auch die Regel, nach der ein "Const c = 1.5" zur 2 wird
// (BUG-99).
struct ConstVal {
  enum Kind { NONE, INT, FLOAT, STR } k = NONE;
  int i = 0;
  float f = 0.0f;
  std::string s;

  static ConstVal ofInt(int v)   { ConstVal c; c.k = INT;   c.i = v; return c; }
  static ConstVal ofFloat(float v) { ConstVal c; c.k = FLOAT; c.f = v; return c; }
  static ConstVal ofStr(std::string v) {
    ConstVal c; c.k = STR; c.s = std::move(v); return c;
  }

  bool ok() const { return k != NONE; }

  // FloatConstNode::intValue: fistp im Vorgabemodus, also zur naechsten
  // Ganzzahl, bei .5 zur geraden. StringConstNode: atoi.
  int intValue() const {
    if (k == FLOAT) return bb_FloatToInt_(f);
    if (k == STR)   return std::atoi(s.c_str());
    return i;
  }
  float floatValue() const {
    if (k == INT) return static_cast<float>(i);
    if (k == STR) return static_cast<float>(std::atof(s.c_str()));
    return f;
  }
  // IntConstNode: itoa, FloatConstNode: ftoa (wie Str zur Laufzeit).
  std::string stringValue() const {
    if (k == INT)   return std::to_string(i);
    if (k == FLOAT) return bb_FloatToStr_(f);
    return s;
  }

  // CastNode::semant auf einem ConstNode.
  ConstVal castTo(Kind to) const {
    if (to == INT)   return ofInt(intValue());
    if (to == FLOAT) return ofFloat(floatValue());
    if (to == STR)   return ofStr(stringValue());
    return ConstVal();
  }
};

#endif // BLITZNEXT_CONSTVAL_H
