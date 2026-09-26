#ifndef BLITZNEXT_CONSTFOLD_H
#define BLITZNEXT_CONSTFOLD_H

#include "ast.h"
#include "constval.h"
#include "lexer.h" // toLower
#include <cmath>
#include <cstdint>
#include <functional>
#include <string>

// ---------------------------------------------------------------------------
// Konstantenfaltung wie im Original (BUG-99)
//
// Nachgebaut ist, was die semant()-Methoden in compiler/exprnode.cpp tun, wenn
// beide Seiten ConstNodes sind: der Ergebnistyp folgt denselben Regeln wie zur
// Laufzeit, und gerechnet wird mit intValue/floatValue/stringValue. Konstant
// ist nur, was dort gefaltet wird - Literale, Pi/True/False, Consts, alle
// Operatoren und die Operatorwoerter Abs/Sgn/Int/Float/Str. Jeder
// Funktionsaufruf (Len, Sin, MilliSecs ...) und jede Variable ist es nicht
// ("Expression must be constant", gemessen in build/const20260926).
// ---------------------------------------------------------------------------

class ConstFolder {
public:
  enum Result { OK, NOT_CONST, DIV_ZERO, BAD_TYPE };

  // Liefert den Wert eines bereits bekannten Const oder nullptr.
  using Lookup = std::function<const ConstVal *(const std::string &lower)>;

  explicit ConstFolder(Lookup lookup) : lookup_(std::move(lookup)) {}

  // Faltet e. Bei OK steht der Wert in out; sonst sagt das Ergebnis, warum
  // nicht, und errLine/errCol zeigen auf den Teilausdruck.
  Result fold(const ExprNode *e, ConstVal &out) {
    result_ = OK;
    out = eval(e);
    if (result_ == OK && !out.ok()) result_ = NOT_CONST;
    return result_;
  }

  int errLine = 0, errCol = 0;

  // Dezimales Ganzzahl-Literal wie atoi in MSVC 6: jede Ziffer wird
  // aufmultipliziert, ein Ueberlauf laeuft um ("2147483648" -> -2147483648,
  // gemessen). Hex und Binaer hat der Lexer schon in eine int-Zahl gewandelt.
  static int parseIntLiteral(const std::string &text) {
    size_t p = 0;
    bool neg = false;
    if (p < text.size() && (text[p] == '-' || text[p] == '+'))
      neg = text[p++] == '-';
    uint32_t v = 0;
    for (; p < text.size() && text[p] >= '0' && text[p] <= '9'; ++p)
      v = v * 10u + static_cast<uint32_t>(text[p] - '0');
    if (neg) v = 0u - v;
    return static_cast<int>(v);
  }

private:
  Lookup lookup_;
  Result result_ = OK;

  ConstVal fail(Result r, const ExprNode *at) {
    if (result_ == OK) {
      result_ = r;
      errLine = at ? at->line : 0;
      errCol  = at ? at->col : 0;
    }
    return ConstVal();
  }

  static int wrap(int64_t v) {
    return static_cast<int>(static_cast<uint32_t>(static_cast<uint64_t>(v)));
  }

  ConstVal eval(const ExprNode *e) {
    if (!e || result_ != OK) return ConstVal();

    if (auto *le = dynamic_cast<const LiteralExpr *>(e)) {
      if (le->isNull) return fail(NOT_CONST, e);
      switch (le->token.type) {
        case TokenType::INT_LIT:
          return ConstVal::ofInt(parseIntLiteral(le->token.value));
        case TokenType::FLOAT_LIT:
          return ConstVal::ofFloat(
              static_cast<float>(std::atof(le->token.value.c_str())));
        case TokenType::STRING_LIT:
          return ConstVal::ofStr(le->token.value);
        default:
          return fail(NOT_CONST, e);
      }
    }

    if (auto *ve = dynamic_cast<const VarExpr *>(e)) {
      const ConstVal *c = lookup_(toLower(ve->name));
      if (!c || !c->ok()) return fail(NOT_CONST, e);
      return *c;
    }

    if (auto *ue = dynamic_cast<const UnaryExpr *>(e)) {
      ConstVal v = eval(ue->expr.get());
      if (!v.ok()) return v;
      if (ue->op == "NOT") {
        // parseExpr der Referenz: "Not x" ist "x = 0", verglichen im Typ
        // von x - ein String also mit "0".
        if (v.k == ConstVal::STR) return ConstVal::ofInt(v.s == "0");
        if (v.k == ConstVal::FLOAT) return ConstVal::ofInt(v.f == 0.0f);
        return ConstVal::ofInt(v.i == 0);
      }
      if (ue->op == "~") // BinExprNode( XOR,x,-1 )
        return ConstVal::ofInt(v.intValue() ^ -1);
      if (v.k == ConstVal::STR) return fail(BAD_TYPE, e);
      if (ue->op == "-") {
        if (v.k == ConstVal::FLOAT) return ConstVal::ofFloat(-v.f);
        return ConstVal::ofInt(wrap(-static_cast<int64_t>(v.i)));
      }
      return v; // "+"
    }

    if (auto *ce = dynamic_cast<const CallExpr *>(e)) {
      const std::string lo = toLower(ce->name);
      bool op = lo == "abs" || lo == "sgn" || lo == "int" || lo == "float" ||
                lo == "str";
      if (!op || ce->args.size() != 1) return fail(NOT_CONST, e);
      ConstVal v = eval(ce->args[0].get());
      if (!v.ok()) return v;
      if (lo == "int")   return v.castTo(ConstVal::INT);
      if (lo == "float") return v.castTo(ConstVal::FLOAT);
      if (lo == "str")   return v.castTo(ConstVal::STR);
      // UniExprNode ABS/SGN
      if (v.k == ConstVal::STR) return fail(BAD_TYPE, e);
      if (v.k == ConstVal::FLOAT) {
        if (lo == "abs") return ConstVal::ofFloat(v.f >= 0 ? v.f : -v.f);
        return ConstVal::ofFloat(v.f > 0 ? 1.0f : (v.f < 0 ? -1.0f : 0.0f));
      }
      if (lo == "abs")
        return ConstVal::ofInt(v.i >= 0 ? v.i : wrap(-static_cast<int64_t>(v.i)));
      return ConstVal::ofInt(v.i > 0 ? 1 : (v.i < 0 ? -1 : 0));
    }

    if (auto *be = dynamic_cast<const BinaryExpr *>(e)) {
      ConstVal l = eval(be->left.get());
      if (!l.ok()) return l;
      ConstVal r = eval(be->right.get());
      if (!r.ok()) return r;
      const std::string &op = be->op;

      // BinExprNode: beide Seiten nach int.
      if (op == "AND" || op == "OR" || op == "XOR" || op == "SHL" ||
          op == "SHR" || op == "SAR") {
        int a = l.intValue(), b = r.intValue();
        // Die Schiebeweite nimmt x86 nur modulo 32.
        int n = b & 31;
        if (op == "AND") return ConstVal::ofInt(a & b);
        if (op == "OR")  return ConstVal::ofInt(a | b);
        if (op == "XOR") return ConstVal::ofInt(a ^ b);
        if (op == "SHL")
          return ConstVal::ofInt(static_cast<int>(static_cast<uint32_t>(a) << n));
        if (op == "SHR")
          return ConstVal::ofInt(static_cast<int>(static_cast<uint32_t>(a) >> n));
        return ConstVal::ofInt(a >> n); // SAR
      }

      // RelExprNode: verglichen wird im gemeinsamen Typ, Ergebnis int.
      if (op == "=" || op == "<>" || op == "<" || op == ">" || op == "<=" ||
          op == ">=") {
        int c;
        if (l.k == ConstVal::STR || r.k == ConstVal::STR) {
          int cmp = l.stringValue().compare(r.stringValue());
          c = cmp < 0 ? -1 : (cmp > 0 ? 1 : 0);
        } else if (l.k == ConstVal::FLOAT || r.k == ConstVal::FLOAT) {
          float a = l.floatValue(), b = r.floatValue();
          if (op == "=")  return ConstVal::ofInt(a == b);
          if (op == "<>") return ConstVal::ofInt(a != b);
          if (op == "<")  return ConstVal::ofInt(a < b);
          if (op == ">")  return ConstVal::ofInt(a > b);
          if (op == "<=") return ConstVal::ofInt(a <= b);
          return ConstVal::ofInt(a >= b);
        } else {
          c = l.i < r.i ? -1 : (l.i > r.i ? 1 : 0);
        }
        if (op == "=")  return ConstVal::ofInt(c == 0);
        if (op == "<>") return ConstVal::ofInt(c != 0);
        if (op == "<")  return ConstVal::ofInt(c < 0);
        if (op == ">")  return ConstVal::ofInt(c > 0);
        if (op == "<=") return ConstVal::ofInt(c <= 0);
        return ConstVal::ofInt(c >= 0);
      }

      // ArithExprNode: String, wenn eine Seite String ist (nur "+"), Float
      // bei "^" oder einer Float-Seite, sonst int.
      if (l.k == ConstVal::STR || r.k == ConstVal::STR) {
        if (op != "+") return fail(BAD_TYPE, e);
        return ConstVal::ofStr(l.stringValue() + r.stringValue());
      }
      if (op == "^" || l.k == ConstVal::FLOAT || r.k == ConstVal::FLOAT) {
        float a = l.floatValue(), b = r.floatValue();
        if ((op == "/" || op == "MOD") && b == 0.0f) return fail(DIV_ZERO, e);
        double d = a, bd = b;
        if (op == "+")   return ConstVal::ofFloat(static_cast<float>(d + bd));
        if (op == "-")   return ConstVal::ofFloat(static_cast<float>(d - bd));
        if (op == "*")   return ConstVal::ofFloat(static_cast<float>(d * bd));
        if (op == "/")   return ConstVal::ofFloat(static_cast<float>(d / bd));
        if (op == "MOD") return ConstVal::ofFloat(static_cast<float>(std::fmod(d, bd)));
        if (op == "^")   return ConstVal::ofFloat(static_cast<float>(std::pow(d, bd)));
        return fail(NOT_CONST, e);
      }
      int64_t a = l.i, b = r.i;
      if ((op == "/" || op == "MOD") && b == 0) return fail(DIV_ZERO, e);
      if (op == "+") return ConstVal::ofInt(wrap(a + b));
      if (op == "-") return ConstVal::ofInt(wrap(a - b));
      if (op == "*") return ConstVal::ofInt(wrap(a * b));
      // Ganzzahldivision schneidet ab, Mod traegt das Vorzeichen links - wie
      // "/" und "%" in C. INT_MIN / -1 laeuft in 64 Bit nicht ueber.
      if (op == "/")   return ConstVal::ofInt(wrap(a / b));
      if (op == "MOD") return ConstVal::ofInt(wrap(a % b));
      return fail(NOT_CONST, e);
    }

    // Alles andere - New, First, Feldzugriff, Array, Handle ... - wird nie
    // gefaltet.
    return fail(NOT_CONST, e);
  }
};

#endif // BLITZNEXT_CONSTFOLD_H
