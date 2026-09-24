#ifndef BLITZNEXT_SEMANT_H
#define BLITZNEXT_SEMANT_H

#include "ast.h"
#include "commands.h"
#include "lexer.h" // toLower
#include "sourcemap.h"
#include "suggest.h"
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// ---------------------------------------------------------------------------
// Semantic pass (WEAK-03 Stufe 3 / WEAK-14)
//
// Sits between parser and emitter and answers the questions the emitter cannot:
// what type does this expression have, and does this program make sense. Every
// rule here is taken from the Blitz3D compiler itself
// (github.com/blitz-research/blitz3d, compiler/exprnode.cpp, varnode.cpp) so
// that valid Blitz3D programs stay valid:
//
//   * "Variable type mismatch"    — a type tag that contradicts the variable
//                                   (IdentVarNode::semant looks the name up
//                                   without the tag, then compares the type)
//   * "Too many/Not enough parameters" — argument count against the declaration
//                                   (ExprSeqNode::castTo)
//   * strings and arithmetic      — "+" makes the expression a string, every
//                                   other arithmetic operator is an error
//                                   (ArithExprNode::semant)
//   * objects and arithmetic      — never allowed (same function)
//
// The pass is deliberately silent whenever a type cannot be determined: an
// unknown type is UNKNOWN and suppresses every check that depends on it. It is
// better to miss a real error than to reject a valid program.
// ---------------------------------------------------------------------------

class Analyzer {
public:
  // Returns the number of errors reported (0 = clean).
  int analyze(Program *prog, const SourceMap &map) {
    map_        = &map;
    errors_     = 0;
    blockDepth_ = 0;
    constNames_.clear();
    types_.clear();
    typeNames_.clear();
    fieldNames_.clear();
    funcs_.clear();
    arrays_.clear();
    globals_.clear();

    collect(prog->nodes);

    // Die Labels des Hauptprogramms - ohne die der Funktionen, die ihre
    // eigenen haben.
    labels_.clear();
    sammleLabels(prog->nodes);

    // Main body: everything that is not a function declaration.
    Scope main;
    scope_      = &main;
    returnType_ = Ty(); // no enclosing function
    inFunction_ = false;
    for (auto &n : prog->nodes)
      if (!dynamic_cast<FunctionDecl *>(n.get()))
        stmt(n.get());

    // Each function gets a fresh scope seeded with its parameters.
    checkFunctions(prog->nodes);
    return errors_;
  }

private:
  // ------------------------------------------------------------------ types
  struct Ty {
    // NUL ist der Typ von "Null". In der Referenz ein StructType("Null")
    // (compiler/type.cpp): er passt auf jedes Objekt und jedes Objekt auf
    // ihn, aber nichts sonst - keine Zahl, kein String (BUG-45).
    enum K { UNKNOWN, INT, FLOAT, STR, OBJ, NUL } k = UNKNOWN;
    std::string obj; // type name when k == OBJ
    // Festes Array mit eckigen Klammern ("Local a[3]"). k traegt dann
    // den ELEMENTtyp; das Array selbst ist im Original ein eigener Typ
    // (VectorType) und laesst sich weder zuweisen noch als Ganzes
    // rechnen (BUG-59).
    bool vec = false;

    bool numeric() const { return k == INT || k == FLOAT; }
    bool known() const { return k != UNKNOWN; }
    // structType() der Referenz: ein Objekt oder Null.
    bool object() const { return !vec && (k == OBJ || k == NUL); }
    std::string name() const {
      if (vec) return "array of " + elemName();
      return elemName();
    }
    std::string elemName() const {
      switch (k) {
        case INT:   return "int";
        case FLOAT: return "float";
        case STR:   return "string";
        case OBJ:   return "." + obj;
        case NUL:   return "Null";
        default:    return "unknown";
      }
    }
    bool sameAs(const Ty &o) const {
      if (k != o.k || vec != o.vec) return false;
      return k != OBJ || toLower(obj) == toLower(o.obj);
    }
  };

  static Ty mk(Ty::K k, const std::string &obj = "") {
    Ty t; t.k = k; t.obj = obj; return t;
  }

  // "%"/"" → int, "#" → float, "$" → string, ".Name" → object
  static Ty fromHint(const std::string &h) {
    if (h == "$") return mk(Ty::STR);
    if (h == "#") return mk(Ty::FLOAT);
    if (!h.empty() && h[0] == '.') return mk(Ty::OBJ, h.substr(1));
    return mk(Ty::INT); // "%" or none — Blitz3D's default
  }

  // Dasselbe fuer eine Deklaration, die ein festes Array sein kann.
  static Ty fromHint(const std::string &h, bool isVec) {
    Ty t = fromHint(h);
    t.vec = isVec;
    return t;
  }

  using Scope = std::unordered_map<std::string, Ty>;

  struct FuncInfo {
    Ty ret;
    std::vector<Ty> params;
    size_t required = 0; // Mindestzahl der Argumente (BUG-49)
  };
  struct ArrayInfo {
    Ty elem;
    size_t dims = 0;
  };

  // ------------------------------------------------------------- diagnostics
  void error(int line, int col, const std::string &msg) {
    std::cerr << map_->format(line, col > 0 ? col : 1)
              << ": error: " << msg << "\n";
    ++errors_;
  }

  // --------------------------------------------------------------- collect
  // Registers types, functions, Dim'd arrays, globals and constants up front,
  // so that forward references work the same way they do in the emitter.
  // Die Labels des Bereichs, der gerade geprueft wird.
  //
  // **Labels sind funktionslokal**, am Original gemessen (2026-09-11): ein
  // `Goto` in einer Funktion erreicht ein Label im Hauptprogramm nicht, und
  // umgekehrt meldet das Original in beiden Richtungen `Undefined label`.
  // Deshalb wird diese Menge je Bereich neu gefuellt und nicht einmal fuer
  // das ganze Programm.
  // Name -> Zeile und Spalte der Definition.
  std::unordered_map<std::string, std::pair<int, int>> labels_;

  // Labels eines Bereichs einsammeln - rekursiv durch alle Bloecke, aber
  // **nicht** in eine Funktionsdeklaration hinein, denn deren Labels gehoeren
  // ihr allein. Ein Vorlauf ist noetig, weil ein Label hinter seiner
  // Verwendung stehen darf (auch das gemessen; `Goto spaet` vor `.spaet` nimmt
  // das Original an).
  //
  // Eine zweite Definition desselben Namens meldet die Referenz an der
  // zweiten (`duplicate label`, LabelNode::semant in compiler/stmtnode.cpp).
  // Ohne diese Pruefung scheiterte erst g++ an `lbl_x` (BUG-42).
  void sammleLabels(const std::vector<std::unique_ptr<ASTNode>> &nodes) {
    for (auto &n : nodes) {
      if (auto *lb = dynamic_cast<LabelStmt *>(n.get())) {
        auto ins = labels_.emplace(toLower(lb->name),
                                   std::make_pair(lb->line, lb->col));
        if (!ins.second)
          error(lb->line, lb->col,
                "Duplicate label '" + lb->name + "' (first defined at " +
                    map_->format(ins.first->second.first,
                                 ins.first->second.second) + ")");
      } else if (dynamic_cast<FunctionDecl *>(n.get())) {
        continue; // eigener Bereich
      } else if (auto *pr = dynamic_cast<Program *>(n.get())) {
        sammleLabels(pr->nodes);
      } else if (auto *is = dynamic_cast<IfStmt *>(n.get())) {
        sammleLabels(is->thenBlock); sammleLabels(is->elseBlock);
      } else if (auto *ws = dynamic_cast<WhileStmt *>(n.get())) {
        sammleLabels(ws->block);
      } else if (auto *rs = dynamic_cast<RepeatStmt *>(n.get())) {
        sammleLabels(rs->block);
      } else if (auto *fs = dynamic_cast<ForStmt *>(n.get())) {
        sammleLabels(fs->block);
      } else if (auto *ss = dynamic_cast<SelectStmt *>(n.get())) {
        for (auto &c : ss->cases) sammleLabels(c.block);
        sammleLabels(ss->defaultBlock);
      } else if (auto *fes = dynamic_cast<ForEachStmt *>(n.get())) {
        sammleLabels(fes->block);
      }
    }
  }

  // Ein Sprungziel, das es nicht gibt (BUG-87). Ohne diese Pruefung erzeugt
  // der Emitter `goto lbl_x;` bzw. `bb_DataRestore(__data_at_x__)` und der
  // Nutzer bekommt eine g++-Meldung ueber Code, den er nie geschrieben hat.
  void pruefeLabel(const std::string &name, int line, int col) {
    if (name.empty()) return;
    if (labels_.count(toLower(name))) return;
    error(line, col, "Undefined label '" + name + "'");
  }

  void collect(const std::vector<std::unique_ptr<ASTNode>> &nodes) {
    for (auto &n : nodes) {
      if (auto *td = dynamic_cast<TypeDecl *>(n.get())) {
        auto &fields = types_[toLower(td->name)];
        for (auto &f : td->fields)
          fields[toLower(f.name)] = fromHint(f.typeHint, f.vecSize != nullptr);
        for (auto &f : td->fields)
          checkVecSize(f.vecSize.get(), f.name, td->line, td->col);
        // The names as they were written, in declaration order: a
        // suggestion should read the way the source spells it, and the
        // order settles a tie reproducibly (WEAK-14, Stufe 2).
        typeNames_.push_back(td->name);
        auto &spelled = fieldNames_[toLower(td->name)];
        for (auto &f : td->fields) spelled.push_back(f.name);
      } else if (auto *fn = dynamic_cast<FunctionDecl *>(n.get())) {
        FuncInfo fi;
        fi.ret = fromHint(fn->returnHint);
        for (auto &p : fn->params)
          fi.params.push_back(fromHint(p.hint, p.vecSize != nullptr));
        // Pflicht ist alles bis zum LETZTEN Parameter ohne Vorgabe - am
        // Original gemessen: "F(a=1,b)" verlangt beide Argumente, weil b keine
        // Vorgabe hat. Eine Vorgabe vor einem Parameter ohne Vorgabe ist damit
        // zwar erlaubt, aber nie weglassbar.
        fi.required = 0;
        for (size_t i = 0; i < fn->params.size(); ++i)
          if (!fn->params[i].defaultValue) fi.required = i + 1;
        funcs_[toLower(fn->name)] = fi;
        collect(fn->body); // Global and Dim may appear inside a function
      } else if (auto *ds = dynamic_cast<DimStmt *>(n.get())) {
        arrays_[toLower(ds->name)] = ArrayInfo{fromHint(ds->typeHint), ds->dims.size()};
      } else if (auto *vd = dynamic_cast<VarDecl *>(n.get())) {
        if (vd->scope == VarDecl::GLOBAL)
          globals_[toLower(vd->name)] =
              fromHint(vd->typeHint, vd->vecSize != nullptr);
      } else if (auto *cd = dynamic_cast<ConstDecl *>(n.get())) {
        globals_[toLower(cd->name)] = fromHint(cd->typeHint);
        constNames_.insert(toLower(cd->name));
      } else if (auto *pr = dynamic_cast<Program *>(n.get())) {
        collect(pr->nodes);
      } else if (auto *is = dynamic_cast<IfStmt *>(n.get())) {
        collect(is->thenBlock); collect(is->elseBlock);
      } else if (auto *ws = dynamic_cast<WhileStmt *>(n.get())) {
        collect(ws->block);
      } else if (auto *rs = dynamic_cast<RepeatStmt *>(n.get())) {
        collect(rs->block);
      } else if (auto *fs = dynamic_cast<ForStmt *>(n.get())) {
        collect(fs->block);
      } else if (auto *ss = dynamic_cast<SelectStmt *>(n.get())) {
        for (auto &c : ss->cases) collect(c.block);
        collect(ss->defaultBlock);
      } else if (auto *fes = dynamic_cast<ForEachStmt *>(n.get())) {
        collect(fes->block);
      }
    }
  }

  // Ein Vorgabewert muss konstant sein - "Expression must be constant" meldet
  // das Original fuer eine Variable, waehrend Literal, Vorzeichen, "1+1",
  // "1 Shl 2", ein Const und die reservierten True/False/Pi durchgehen (alles
  // gemessen). Ausgewertet wird hier nichts: fuer die Diagnose genuegt die
  // Form, und der Emitter reicht den Ausdruck als C++-Vorgabeargument weiter.
  bool isConstExpr(ExprNode *e) {
    if (!e) return false;
    // Null ist kein ConstNode der Referenz, sondern ein NullNode:
    // "Function F(p.T=Null)" ergibt dort "Expression must be constant"
    // (gemessen, test_bug56_object_param.bb; BUG-45).
    if (auto *le = dynamic_cast<LiteralExpr *>(e)) return !le->isNull;
    if (auto *ue = dynamic_cast<UnaryExpr *>(e)) return isConstExpr(ue->expr.get());
    if (auto *be = dynamic_cast<BinaryExpr *>(e))
      return isConstExpr(be->left.get()) && isConstExpr(be->right.get());
    if (auto *ve = dynamic_cast<VarExpr *>(e))
      return constNames_.count(toLower(ve->name)) > 0;
    return false;
  }

  // Die Groesse eines festen Arrays muss konstant sein: die Referenz
  // meldet "Blitz array sizes must be constant" in VectorDeclNode::proto,
  // sobald der Ausdruck kein ConstNode ist. Ein Const ist zulaessig, eine
  // Variable nicht - beides am Original gemessen (BUG-59). Geprueft wird
  // wie beim Vorgabewert eines Parameters nur die FORM; den Wert braucht
  // es hier nicht, weil der Emitter den Ausdruck als C++-Feldgroesse
  // weiterreicht und ein Const dort als constexpr steht.
  void checkVecSize(ExprNode *size, const std::string &name, int line,
                    int col) {
    if (!size || isConstExpr(size)) return;
    error(line, col,
          "the size of the array '" + name + "' must be constant");
  }

  void checkFunctions(const std::vector<std::unique_ptr<ASTNode>> &nodes) {
    for (auto &n : nodes) {
      if (auto *pr = dynamic_cast<Program *>(n.get())) {
        checkFunctions(pr->nodes);
      } else if (auto *fn = dynamic_cast<FunctionDecl *>(n.get())) {
        // Erst hier, nicht schon in collect(): ein Const darf hinter der
        // Funktion stehen, die es als Vorgabe benutzt - das Original nimmt das
        // an. Waehrend collect() laeuft, ist constNames_ noch unvollstaendig.
        for (auto &p : fn->params) {
          if (p.defaultValue && !isConstExpr(p.defaultValue.get()))
            error(p.defaultValue->line, p.defaultValue->col,
                  "the default value of '" + p.name +
                      "' must be a constant expression");
          checkVecSize(p.vecSize.get(), p.name, fn->line, fn->col);
        }
        Scope local;
        for (auto &p : fn->params)
          local[toLower(p.name)] = fromHint(p.hint, p.vecSize != nullptr);
        scope_      = &local;
        returnType_ = fromHint(fn->returnHint);
        inFunction_ = true;
        labels_.clear();
        sammleLabels(fn->body);
        for (auto &s : fn->body) stmt(s.get());
        inFunction_ = false;
      }
    }
  }

  // Statements of a nested block. Blitz3D allows Global and Const only at the
  // top level of the main program - parseStmtSeq guards both with
  // "if( scope!=STMTS_PROG ) ex( \"'Global' can only appear in main program\" )"
  // - so the pass has to know when it is inside one (BUG-17). A Program node
  // is a statement container, not a block, and deliberately does not count.
  void block(const std::vector<std::unique_ptr<ASTNode>> &b) {
    ++blockDepth_;
    for (auto &s : b) stmt(s.get());
    --blockDepth_;
  }

  // True when the expression certainly reads something only known at run time.
  // ForNode::semant rejects a computed Step ("Step value must be constant"),
  // and constNode() there is true for literals and for Const identifiers. This
  // deliberately errs the other way: only a variable, a call or an element /
  // field access counts as run-time. Literal arithmetic is left alone, because
  // whether the reference folds it is not established - and missing an error
  // is better than rejecting a valid program.
  bool mentionsRuntimeValue(const ExprNode *e) const {
    if (!e) return false;
    if (auto *ve = dynamic_cast<const VarExpr *>(e))
      return constNames_.count(toLower(ve->name)) == 0;
    if (dynamic_cast<const CallExpr *>(e))     return true;
    if (dynamic_cast<const ArrayAccess *>(e))  return true;
    if (dynamic_cast<const FieldAccess *>(e))  return true;
    if (dynamic_cast<const VectorAccess *>(e)) return true;
    if (auto *be = dynamic_cast<const BinaryExpr *>(e))
      return mentionsRuntimeValue(be->left.get()) ||
             mentionsRuntimeValue(be->right.get());
    if (auto *ue = dynamic_cast<const UnaryExpr *>(e))
      return mentionsRuntimeValue(ue->expr.get());
    return false; // literals and everything else
  }

  // ---------------------------------------------------------------- lookup
  const Ty *lookup(const std::string &name) const {
    std::string lo = toLower(name);
    auto it = scope_->find(lo);
    if (it != scope_->end()) return &it->second;
    auto g = globals_.find(lo);
    if (g != globals_.end()) return &g->second;
    return nullptr;
  }

  // Blitz3D auto-declares on first use ("ugly auto decl!" in varnode.cpp), so
  // an unknown name is not an error — it simply comes into being here.
  void declare(const std::string &name, const Ty &t) {
    std::string lo = toLower(name);
    if (globals_.count(lo)) return; // globals win, as in the emitter
    (*scope_)[lo] = t;
  }

  // A tag that contradicts the variable's type — varnode.cpp rejects this.
  // Returns true when it reported, so the caller can skip a follow-up message
  // about the same line.
  bool checkTag(const std::string &name, const std::string &hint,
                int line, int col) {
    if (hint.empty()) return false;
    const Ty *known = lookup(name);
    if (!known || !known->known()) return false;
    Ty tagged = fromHint(hint);
    if (tagged.sameAs(*known)) return false;
    error(line, col, "Variable type mismatch: '" + name + "' is " +
                         known->name() + ", but is used as " + tagged.name());
    return true;
  }

  // A tag on an array access is checked against the element type. Blitz3D does
  // this for arrays but NOT for type fields (BUG-31/BUG-32): ArrayVarNode::
  // semant resolves the tag with findType(), which maps "%"/"#"/"$" to int /
  // float / string, and then "if( t && t!=a->elementType )" is an error. An
  // empty tag resolves to nothing and checks nothing.
  void checkArrayTag(const std::string &name, const std::string &hint,
                     const Ty &elem, int line, int col) {
    if (hint.empty() || !elem.known()) return;
    Ty tagged = fromHint(hint);
    if (tagged.sameAs(elem)) return;
    error(line, col, "array type mismatch: '" + name + "' holds " +
                         elem.name() + ", but is used as " + tagged.name());
  }

  // string ↔ number is the conversion Blitz3D refuses; everything else is
  // either allowed (int ↔ float) or deliberately not checked here.
  // Was hier durchgeht, entscheidet die Sprache - nicht, was C++ bequem findet.
  //
  // Am laufenden Original gemessen (BUG-53): Integer, Float und String wandeln
  // in ALLEN sechs Richtungen ineinander um, und zwar an jeder Stelle mit
  // bekanntem Zieltyp - Zuweisung, Local/Global mit Initialisierung, Parameter
  // und Return. Frueher hat diese Funktion genau das abgelehnt; sie war damit
  // strenger als die Sprache und der haeufigste Einzelbefund im Beispielbestand
  // (31 von 61 Dateien scheiterten an "cannot assign int to a string").
  //
  // Objekte wandeln dagegen nie: das Original meldet "Illegal type conversion"
  // fuer Objekt an String, Objekt an Integer, String an Objekt und ebenso fuer
  // zwei verschiedene Types. Genau das wird jetzt hier geprueft - vorher wurde
  // es an keiner Stelle geprueft (BUG-55).
  // Eine Zuweisung an ein ganzes Array - oder eines als Wert - lehnt das
  // Original mit "Blitz arrays can not be assigned to" ab, "b = a"
  // genauso wie "a = 5" (gemessen). Ohne diese Sperre wuerde aus "b = a"
  // eine stille Kopie: zwei std::array gleicher Groesse sind in C++
  // zuweisbar (BUG-59).
  void checkNotVec(const Ty &t, int line, int col) {
    if (t.vec) error(line, col, "Blitz arrays can not be assigned to");
  }

  void checkAssign(const Ty &target, const Ty &value, const char *what,
                   int line, int col) {
    // Ein festes Array passt nur auf ein festes Array. Als Argument ist
    // das der Normalfall - die Uebergabe ist eine Referenz -, gemischt
    // mit einem Skalar ist es "Illegal type conversion" im Original
    // (gemessen, BUG-59). Die Groesse gehoert dort zwar zum Typ, wird
    // hier aber nicht mitgefuehrt; ein a[3] an einem v[2] faellt erst
    // dem C++-Uebersetzer auf.
    if (target.vec != value.vec) {
      if (target.known() && value.known())
        error(line, col, std::string(what) + ": cannot pass " +
                             value.name() + " where " + target.name() +
                             " is expected");
      return;
    }
    if (castable(value, target)) return;
    error(line, col, std::string(what) + ": cannot assign " + value.name() +
                         " to " + target.name());
  }

  // canCastTo() der Referenz fuer Skalare (compiler/type.cpp): Zahlen und
  // Strings wandeln frei ineinander (BUG-53); ein Objekt nur in denselben
  // Typ oder Null, Null in jedes Objekt. Eine Ganzzahl ist kein Objekt -
  // "p.T = 0" lehnt das Original ab. Bis BUG-45 war genau das erlaubt, weil
  // Null hier als 0 ankam.
  //
  // Unbekanntes bleibt still, auch ein unbekannter Typname: der ist bereits
  // als "Type 'X' not found" gemeldet, und "Local p.Punkt = First Punkte"
  // soll eine Meldung ergeben, nicht zwei.
  bool castable(const Ty &from, const Ty &to) const {
    if (!from.known() || !to.known()) return true;
    if (from.k == Ty::OBJ && !types_.count(toLower(from.obj))) return true;
    if (to.k   == Ty::OBJ && !types_.count(toLower(to.obj)))   return true;
    // Feste Arrays (checkAssign hat gemischte Faelle schon gemeldet): wie
    // bisher nur der Objekt-Elementtyp, die Groesse fuehrt Ty nicht mit.
    if (from.vec || to.vec)
      return (from.k != Ty::OBJ && to.k != Ty::OBJ) || from.sameAs(to);
    if (!from.object() && !to.object()) return true;
    if (!from.object() || !to.object()) return false;
    return from.k == Ty::NUL || to.k == Ty::NUL || from.sameAs(to);
  }

  // Original CastNode rejects objects/vectors at an integer boundary.
  // Unknown types already have their own diagnostics; do not cascade.
  void integerContext(ExprNode *e) {
    Ty t = expr(e);
    if (t.known() && (t.vec || t.object()))
      error(e->line, e->col, "Illegal type conversion");
  }

  // Before/After: AfterNode::semant und BeforeNode::semant
  // (compiler/exprnode.cpp) verlangen einen Objekttyp und liefern ihn
  // unveraendert zurueck. Null prueft die Referenz vorher eigens, mit
  // leicht verschiedenem Wortlaut ("on" bei After, "with" bei Before).
  Ty neighbour(const char *what, ExprNode *object, int line, int col) {
    Ty t = expr(object);
    if (t.k == Ty::NUL) {
      error(line, col, std::string("'") + what + "' cannot be used " +
                           (std::string(what) == "After" ? "on" : "with") +
                           " 'Null'");
      return Ty();
    }
    if (t.known() && (t.vec || t.k != Ty::OBJ)) {
      error(line, col, std::string("'") + what +
                           "' must be used with a custom type object");
      return Ty();
    }
    return t;
  }

  // ------------------------------------------------------------ statements
  void stmt(ASTNode *n) {
    if (!n) return;
    // Program traegt keine eigene Position; alles andere schon.
    if (n->line > 0 && !dynamic_cast<Program *>(n)) {
      stmtLine_ = n->line;
      stmtCol_  = n->col;
    }

    if (auto *pr = dynamic_cast<Program *>(n)) {
      for (auto &s : pr->nodes) stmt(s.get());
    } else if (auto *vd = dynamic_cast<VarDecl *>(n)) {
      Ty t = fromHint(vd->typeHint, vd->vecSize != nullptr);
      checkVecSize(vd->vecSize.get(), vd->name, vd->line, vd->col);
      if (vd->scope == VarDecl::GLOBAL && (blockDepth_ > 0 || inFunction_))
        error(vd->line, vd->col,
              "'Global' is only allowed at the top level of the main program, "
              "not inside a block and not inside a function - declare '" +
                  vd->name + "' there and assign to it here");
      if (vd->scope == VarDecl::LOCAL) declare(vd->name, t);
      if (vd->initValue)
        checkAssign(t, expr(vd->initValue.get()), "Local", vd->line, vd->col);
      // "Local a[3] = 1" gibt es nicht: der Parser laesst Groesse und
      // Initialisierer nicht zusammen zu, und das Original auch nicht.
    } else if (auto *cd = dynamic_cast<ConstDecl *>(n)) {
      if (blockDepth_ > 0 || inFunction_)
        error(cd->line, cd->col,
              "'Const' is only allowed at the top level of the main program, "
              "not inside a block and not inside a function - move the "
              "declaration of '" + cd->name + "' there");
      if (cd->value) expr(cd->value.get());
    } else if (auto *as = dynamic_cast<AssignStmt *>(n)) {
      Ty val = expr(as->value.get());
      const Ty *known = lookup(as->name);
      if (known) {
        if (!checkTag(as->name, as->typeHint, as->line, as->col)) {
          checkNotVec(*known, as->line, as->col);
          checkAssign(*known, val, "assignment", as->line, as->col);
        }
      } else {
        // Eine neue Variable mit Tag: der Wert muss zum Tag passen. Fuer Zahlen
        // und Strings prueft das nichts mehr (die wandeln frei, BUG-53), fuer
        // Objekte schon - sonst waere "p.T = New U" die einzige der sechs
        // Zuweisungsstellen ohne diese Pruefung (BUG-55).
        if (!as->typeHint.empty()) {
          checkNotVec(val, as->line, as->col);
          checkAssign(fromHint(as->typeHint), val, "assignment", as->line,
                      as->col);
        }
        // Eine Variable vom Typ Null gibt es nicht. Ohne Tag entsteht in der
        // Referenz ein int, und Null wandelt nicht in int (BUG-45).
        if (as->typeHint.empty() && val.k == Ty::NUL) {
          checkAssign(mk(Ty::INT), val, "assignment", as->line, as->col);
          val = mk(Ty::INT);
        }
        declare(as->name, as->typeHint.empty() ? val : fromHint(as->typeHint));
      }
    } else if (auto *rd = dynamic_cast<ReadStmt *>(n)) {
      if (lookup(rd->name)) checkTag(rd->name, rd->typeHint, rd->line, rd->col);
      else declare(rd->name, fromHint(rd->typeHint));
    // Die drei Anweisungen mit einem Sprungziel (BUG-87). `Restore` ohne
    // Label setzt auf den Anfang zurueck und braucht keines.
    } else if (auto *gt = dynamic_cast<GotoStmt *>(n)) {
      pruefeLabel(gt->label, gt->line, gt->col);
    } else if (auto *gs = dynamic_cast<GosubStmt *>(n)) {
      pruefeLabel(gs->label, gs->line, gs->col);
    } else if (auto *rst = dynamic_cast<RestoreStmt *>(n)) {
      pruefeLabel(rst->label, rst->line, rst->col);
    } else if (auto *aas = dynamic_cast<ArrayAssignStmt *>(n)) {
      Ty val = expr(aas->value.get());
      for (auto &i : aas->indices) integerContext(i.get());
      auto it = arrays_.find(toLower(aas->name));
      if (it != arrays_.end()) {
        if (aas->indices.size() != it->second.dims)
          error(aas->line, aas->col, "array '" + aas->name + "' has " +
                    std::to_string(it->second.dims) + " dimension(s), but " +
                    std::to_string(aas->indices.size()) + " index/indices given");
        checkArrayTag(aas->name, aas->typeHint, it->second.elem, aas->line,
                      aas->col);
        checkAssign(it->second.elem, val, "array assignment", aas->line, aas->col);
      }
    } else if (auto *fas = dynamic_cast<FieldAssignStmt *>(n)) {
      Ty obj = expr(fas->object.get());
      Ty val = expr(fas->value.get());
      Ty f   = fieldType(obj, fas->fieldName, fas->line, fas->col);
      checkAssign(f, val, "field assignment", fas->line, fas->col);
    } else if (auto *vas = dynamic_cast<VectorAssignStmt *>(n)) {
      // Der Elementtyp eines festen Arrays wird hier noch nicht
      // mitgefuehrt (Ty kennt keine Array-Kategorie), also bleibt es
      // beim Durchlaufen der Teilausdruecke - sonst entginge diesem
      // Zweig jede Pruefung, die sonst ueberall greift.
      expr(vas->base.get());
      integerContext(vas->index.get());
      expr(vas->value.get());
    } else if (auto *is = dynamic_cast<IfStmt *>(n)) {
      integerContext(is->condition.get());
      block(is->thenBlock);
      block(is->elseBlock);
    } else if (auto *ws = dynamic_cast<WhileStmt *>(n)) {
      integerContext(ws->condition.get());
      block(ws->block);
    } else if (auto *rs = dynamic_cast<RepeatStmt *>(n)) {
      block(rs->block);
      if (rs->condition) integerContext(rs->condition.get());
    } else if (auto *fs = dynamic_cast<ForStmt *>(n)) {
      expr(fs->start.get());
      expr(fs->end.get());
      if (fs->step) {
        expr(fs->step.get());
        if (mentionsRuntimeValue(fs->step.get()))
          error(fs->line, fs->col,
                "the Step of a For loop must be constant - Blitz3D rejects a "
                "computed step (\"Step value must be constant\")");
      }
      // The loop variable is an ordinary variable: an existing one (Global
      // included) is used as it stands, and a new one takes the type its tag
      // says, not the type of the start expression - untagged means int
      // (BUG-19, ForNode::semant in the reference). An array element or a
      // field counter is checked like any other access and declares nothing
      // (BUG-30).
      Ty counter;
      if (fs->target) {
        counter = expr(fs->target.get());
      } else if (const Ty *had = lookup(fs->varName)) {
        checkTag(fs->varName, fs->typeHint, fs->line, fs->col);
        counter = *had;
      } else {
        counter = fromHint(fs->typeHint);
        declare(fs->varName, counter);
      }
      // ForNode::semant: "index variable must be integer or real". Ohne
      // diese Pruefung wurde "For s$ = 1 To 3" angenommen und scheiterte
      // erst im C++-Backend am ++ auf einer Zeichenkette.
      if (counter.known() && !counter.vec && counter.k != Ty::INT &&
          counter.k != Ty::FLOAT)
        error(fs->line, fs->col,
              "the index variable of a For loop must be an int or a float, "
              "not " + counter.name() +
                  " (Blitz3D: \"index variable must be integer or real\")");
      block(fs->block);
    } else if (auto *fes = dynamic_cast<ForEachStmt *>(n)) {
      knownType(fes->typeName, fes->line, fes->col);
      if (fes->target) {
        // Feld oder Array-Element als Zaehler (BUG-102): dieselben zwei
        // Schranken wie fuer eine Variable, wenn der Typ bekannt ist.
        Ty t = expr(fes->target.get());
        if (t.known() && t.k != Ty::OBJ)
          error(fes->line, fes->col,
                "the index variable has type " + t.name() + ", but 'Each " +
                    fes->typeName + "' walks a list of objects");
        else if (t.known() && toLower(t.obj) != toLower(fes->typeName))
          error(fes->line, fes->col,
                "the index variable holds a '." + t.obj +
                    "', but the loop walks '" + fes->typeName + "'");
        block(fes->block);
        return;
      }
      // ForEachNode::semant resolves the index variable like any other and
      // then insists on two things: it holds an object ("Index variable is
      // not a NewType") and that object is the very Type being walked
      // ("Type mismatch"). Without a tag the variable may already exist, so
      // both are worth checking here.
      const Ty *had = lookup(fes->varName);
      if (had && had->known()) {
        if (had->k != Ty::OBJ)
          error(fes->line, fes->col,
                "index variable '" + fes->varName + "' has type " + had->name() +
                    ", but 'Each " + fes->typeName + "' walks a list of objects");
        else if (toLower(had->obj) != toLower(fes->typeName))
          error(fes->line, fes->col,
                "index variable '" + fes->varName + "' holds a '." + had->obj +
                    "', but the loop walks '" + fes->typeName + "'");
      } else if (fes->typeTag.empty()) {
        // Gemessen am Original (blitzcc 1.108c): "For q = Each Punkt" mit
        // einem q, das es noch nicht gibt, meldet "Index variable is not a
        // NewType". Der Grund steht in IdentVarNode::semant - eine Variable,
        // die erst hier entsteht, bekommt den Typ ihres Tags, und ohne Tag
        // ist das int. Ein int ist kein structType, also greift die Schranke
        // in ForEachNode::semant.
        error(fes->line, fes->col,
              "index variable '" + fes->varName + "' is created here and is "
              "therefore an int, not an object; write '" + fes->varName + "." +
              fes->typeName + " = Each " + fes->typeName + "' or declare '" +
              fes->varName + "' before the loop");
      }
      declare(fes->varName, mk(Ty::OBJ, fes->typeName));
      block(fes->block);
    } else if (auto *ss = dynamic_cast<SelectStmt *>(n)) {
      Ty sel = expr(ss->expr.get());
      // SelectNode::semant refuses this before it looks at anything else:
      //   if( ty->structType() ) ex( "Select cannot be used with objects" );
      // Numbers and strings are fine; only a Type is not (BUG-39).
      if (sel.object()) {
        int ln = ss->expr->line ? ss->expr->line : ss->line;
        int co = ss->expr->col  ? ss->expr->col  : ss->col;
        error(ln, co,
              sel.k == Ty::NUL
                  ? std::string("'Select' cannot be used with objects, and "
                                "'Null' counts as one")
                  : "'Select' cannot be used with objects; this expression "
                    "holds a '" + sel.name() + "'");
      }
      for (auto &c : ss->cases) {
        for (auto &e : c.expressions) expr(e.get());
        block(c.block);
      }
      block(ss->defaultBlock);
    } else if (auto *ret = dynamic_cast<ReturnStmt *>(n)) {
      if (ret->value) {
        Ty v = expr(ret->value.get());
        if (inFunction_) checkAssign(returnType_, v, "Return", ret->line, ret->col);
      }
    } else if (auto *ds = dynamic_cast<DimStmt *>(n)) {
      for (auto &d : ds->dims) integerContext(d.get());
      // "Dim feld.Punkt(3)" (BUG-52): der Elementtyp muss existieren.
      if (!ds->typeHint.empty() && ds->typeHint[0] == '.')
        knownType(ds->typeHint.substr(1), ds->line, ds->col);
    } else if (auto *del = dynamic_cast<DeleteStmt *>(n)) {
      if (!del->eachTypeName.empty()) {
        knownType(del->eachTypeName, del->line, del->col);
      } else {
        // DeleteNode::semant: "Can't delete non-Newtype" fuer alles, was
        // kein structType ist. Null ist einer - "Delete Null" ist gueltig
        // und tut nichts (BUG-45).
        Ty t = expr(del->object.get());
        if (t.known() && !t.object())
          error(del->line, del->col,
                "'Delete' needs an object, but this is " + t.name());
      }
    } else if (auto *ins = dynamic_cast<InsertStmt *>(n)) {
      // InsertNode::semant: beide Seiten Objekte ("Illegal expression type")
      // und derselbe Typ ("Objects types are differnt"). Null ist dort ein
      // eigener Typ, also auch von jedem .T verschieden (BUG-45).
      Ty o = expr(ins->object.get());
      Ty t = expr(ins->target.get());
      // Ein unbekannter Typname ist schon gemeldet und bleibt hier still.
      auto resolved = [this](const Ty &x) {
        return x.k == Ty::NUL ||
               (x.k == Ty::OBJ && types_.count(toLower(x.obj)) > 0);
      };
      if ((o.known() && !o.object()) || (t.known() && !t.object()))
        error(ins->line, ins->col, "'Insert' needs two objects");
      else if (resolved(o) && resolved(t) && !o.sameAs(t))
        error(ins->line, ins->col,
              "'Insert' needs two objects of the same type, but these are " +
                  o.name() + " and " + t.name());
    } else if (auto *ce = dynamic_cast<CallExpr *>(n)) {
      expr(ce);
    } else if (auto *td = dynamic_cast<TypeDecl *>(n)) {
      // Dieselbe Regel wie Global und Const: "case TYPE: if( scope!=STMTS_PROG )
      // ex( \"'Type' can only appear in main program\" )" (BUG-43).
      if (blockDepth_ > 0 || inFunction_)
        error(td->line, td->col,
              "'Type' is only allowed at the top level of the main program, "
              "not inside a block and not inside a function - move the "
              "declaration of '" + td->name + "' there");
    }
    // Label/Goto/Gosub/Data/Restore/Exit/End — nothing to check here
  }

  // ----------------------------------------------------------- expressions
  Ty expr(ExprNode *e) {
    if (!e) return Ty();

    if (auto *le = dynamic_cast<LiteralExpr *>(e)) {
      if (le->isNull) return mk(Ty::NUL);
      switch (le->token.type) {
        case TokenType::STRING_LIT: return mk(Ty::STR);
        case TokenType::FLOAT_LIT:  return mk(Ty::FLOAT);
        default:                    return mk(Ty::INT);
      }
    }
    // Ein gelesener Data-Wert passt sich dem Ziel an (bb_DataVal wandelt nach
    // int, float und bbString). Ohne Tag traegt er deshalb keinen eigenen Typ
    // mit, sonst wuerde `Read a$` an einer Data-Zahl zu Unrecht klagen.
    if (auto *dr = dynamic_cast<DataReadExpr *>(e)) {
      if (dr->typeHint.empty()) return Ty();
      return fromHint(dr->typeHint);
    }
    if (auto *ve = dynamic_cast<VarExpr *>(e)) {
      checkTag(ve->name, ve->typeHint, ve->line, ve->col);
      if (const Ty *t = lookup(ve->name)) return *t;
      return mk(Ty::INT); // auto-declared on use, as in Blitz3D
    }
    if (auto *aa = dynamic_cast<ArrayAccess *>(e)) {
      for (auto &i : aa->indices) integerContext(i.get());
      auto it = arrays_.find(toLower(aa->name));
      if (it == arrays_.end()) return Ty();
      if (aa->indices.size() != it->second.dims)
        error(aa->line, aa->col, "array '" + aa->name + "' has " +
                  std::to_string(it->second.dims) + " dimension(s), but " +
                  std::to_string(aa->indices.size()) + " index/indices given");
      checkArrayTag(aa->name, aa->typeHint, it->second.elem, aa->line, aa->col);
      return it->second.elem;
    }
    if (auto *fa = dynamic_cast<FieldAccess *>(e)) {
      Ty obj = expr(fa->object.get());
      return fieldType(obj, fa->fieldName, fa->line, fa->col);
    }
    if (auto *va = dynamic_cast<VectorAccess *>(e)) {
      Ty b = expr(va->base.get());
      integerContext(va->index.get());
      // "Variable must be a Blitz array" meldet VectorVarNode::semant,
      // sobald die Basis keinen VectorType hat. Ein unbekannter Typ
      // bleibt still: eine fehlende Pruefung ist besser als eine
      // erfundene.
      if (b.known() && !b.vec) {
        error(va->line, va->col,
              "'[...]' needs a Blitz array, but this is " + b.name());
        return Ty();
      }
      Ty elem = b;
      elem.vec = false;
      return elem;
    }
    if (auto *ne = dynamic_cast<NewExpr *>(e)) {
      knownType(ne->typeName, ne->line, ne->col);
      return mk(Ty::OBJ, ne->typeName);
    }
    if (auto *fe = dynamic_cast<FirstExpr *>(e)) {
      knownType(fe->typeName, fe->line, fe->col);
      return mk(Ty::OBJ, fe->typeName);
    }
    if (auto *le2 = dynamic_cast<LastExpr *>(e)) {
      knownType(le2->typeName, le2->line, le2->col);
      return mk(Ty::OBJ, le2->typeName);
    }
    if (auto *be = dynamic_cast<BeforeExpr *>(e))
      return neighbour("Before", be->object.get(), be->line, be->col);
    if (auto *ae = dynamic_cast<AfterExpr *>(e))
      return neighbour("After", ae->object.get(), ae->line, ae->col);
    // Handle braucht ein Objekt und ergibt int (ObjectHandleNode::semant),
    // Object.T wandelt nach int und ergibt ein T (ObjectCastNode::semant).
    if (auto *he = dynamic_cast<HandleExpr *>(e)) {
      Ty t = expr(he->object.get());
      if (t.known() && !t.object())   // Null geht und ergibt 0
        error(he->line, he->col, "'Handle' must be used with an object, not " + t.name());
      return mk(Ty::INT);
    }
    if (auto *oc = dynamic_cast<ObjectCastExpr *>(e)) {
      Ty t = expr(oc->handle.get());
      if (t.known() && (t.vec || t.object()))
        error(oc->line, oc->col, "'Object' needs a handle number, not " + t.name());
      knownType(oc->typeName, oc->line, oc->col);
      return mk(Ty::OBJ, oc->typeName);
    }
    if (auto *ce = dynamic_cast<CallExpr *>(e))   return call(ce);
    if (auto *ue = dynamic_cast<UnaryExpr *>(e)) {
      Ty t = expr(ue->expr.get());
      // "Not x" baut die Referenz als RelExprNode('=', x, 0): bei einem
      // Objekt oder Null muss die 0 in den Objekttyp wandeln, und das ist
      // "Illegal type conversion" (BUG-45).
      if (ue->op == "NOT") {
        if (t.object())
          error(ue->line, ue->col,
                "'Not' cannot be applied to " + t.name() +
                    " - compare with Null instead: (x = Null)");
        return mk(Ty::INT);
      }
      // UniExprNode::semant nimmt nur int und float ("Illegal operator for
      // type").
      if (t.object()) {
        error(ue->line, ue->col,
              "Operator cannot be applied to custom type objects");
        return Ty();
      }
      if (t.k == Ty::STR)
        error(ue->line, ue->col, "Operator cannot be applied to strings");
      if (ue->op == "~") return mk(Ty::INT);
      return t.numeric() ? t : mk(Ty::INT);
    }
    if (auto *bin = dynamic_cast<BinaryExpr *>(e)) return binary(bin);
    return Ty();
  }

  // Mirrors ArithExprNode::semant: objects never, strings only with "+",
  // "^" and any float operand make the result a float.
  Ty binary(BinaryExpr *b) {
    Ty l = expr(b->left.get());
    Ty r = expr(b->right.get());
    const std::string &op = b->op;

    bool comparison = (op == "=" || op == "<>" || op == "<" || op == ">" ||
                       op == "<=" || op == ">=");
    if (comparison) {
      // RelExprNode::semant: ist eine Seite ein Objekt (Null eingeschlossen),
      // gibt es nur = und <>, und beide Seiten wandeln in den Typ der Seite,
      // die nicht Null ist. "p = 0" und "p.A = q.B" scheitern daran
      // (BUG-45).
      if (l.object() || r.object()) {
        if (op != "=" && op != "<>") {
          error(b->line, b->col,
                "Illegal operator for custom type objects: only = and <> "
                "compare objects");
        } else {
          const Ty &opType = l.k != Ty::NUL ? l : r;
          if (!castable(l, opType) || !castable(r, opType))
            error(b->line, b->col,
                  "cannot compare " + l.name() + " with " + r.name());
        }
      }
      return mk(Ty::INT);
    }

    // And, Or, Xor, Shl, Shr, Sar sind in der Referenz BinExprNode, nicht
    // ArithExprNode: beide Seiten gehen durch castTo(int_type). Ein String
    // wandelt dabei mit atoi ("no" ist 0), ein Float rundet; nur ein Objekt
    // oder ein festes Array ist "Illegal type conversion" (BUG-54). Bis
    // dahin lief die String-Sperre der Arithmetik auch hier.
    if (op == "AND" || op == "OR" || op == "XOR" || op == "SHL" ||
        op == "SHR" || op == "SAR") {
      if ((l.known() && (l.object() || l.vec)) ||
          (r.known() && (r.object() || r.vec)))
        error(b->line, b->col, "Illegal type conversion: '" + op.substr(0, 1) +
                                   toLower(op.substr(1)) +
                                   "' works on numbers and strings, not on "
                                   "objects or Blitz arrays");
      return mk(Ty::INT);
    }

    if (l.object() || r.object()) {
      error(b->line, b->col,
            "Arithmetic operator cannot be applied to custom type objects");
      return Ty();
    }
    if (l.k == Ty::STR || r.k == Ty::STR) {
      if (op != "+") {
        error(b->line, b->col, "Operator cannot be applied to strings");
        return Ty();
      }
      return mk(Ty::STR);
    }
    if (op == "MOD") return mk(Ty::INT);
    if (op == "^" || l.k == Ty::FLOAT || r.k == Ty::FLOAT) return mk(Ty::FLOAT);
    if (l.known() && r.known()) return mk(Ty::INT);
    return Ty();
  }

  Ty fieldType(const Ty &obj, const std::string &field, int line, int col) {
    // Ein Feldzugriff braucht einen Objekttyp. FieldVarNode::semant in
    // compiler/varnode.cpp meldet sonst "Variable must be a Type" - am
    // Original gemessen (2026-09-10) fuer eine implizite Int-Variable, fuer
    // eine nie erwaehnte, fuer String und Float, fuer ein Element eines
    // Dim-Arrays und fuer ein festes Array, auch fuer eines aus Objekten,
    // solange kein Index dahintersteht. Angenommen werden dort ein
    // Objektparameter, eine For-Each-Variable und eine Kette ueber ein
    // Objektfeld (BUG-60).
    //
    // Ein UNBEKANNTER Typ bleibt still - dieselbe Zurueckhaltung wie bei
    // VectorAccess: eine fehlende Pruefung ist besser als eine erfundene.
    if (obj.k != Ty::OBJ || obj.vec) {
      if (obj.known())
        error(stmtLine_ > 0 ? stmtLine_ : line,
              stmtLine_ > 0 ? stmtCol_ : col, "Variable must be a Type");
      return Ty();
    }
    auto t = types_.find(toLower(obj.obj));
    if (t == types_.end()) return Ty();
    auto f = t->second.find(toLower(field));
    if (f == t->second.end()) {
      auto sp = fieldNames_.find(toLower(obj.obj));
      error(line, col, "type '" + obj.obj + "' has no field '" + field + "'" +
                       (sp == fieldNames_.end() ? std::string()
                                                : didYouMean(field, sp->second)));
      return Ty();
    }
    return f->second;
  }

  void knownType(const std::string &name, int line, int col) {
    if (!types_.count(toLower(name)))
      error(line, col, "Type '" + name + "' not found" +
                       didYouMean(name, typeNames_));
  }

  // ------------------------------------------------------------------ calls
  Ty call(CallExpr *ce) {
    std::vector<Ty> args;
    args.reserve(ce->args.size());
    for (auto &a : ce->args) args.push_back(expr(a.get()));

    auto uf = funcs_.find(toLower(ce->name));
    if (uf != funcs_.end()) {
      checkArity(ce, args.size(), uf->second.required,
                 uf->second.params.size());
      for (size_t i = 0; i < args.size() && i < uf->second.params.size(); ++i)
        checkAssign(uf->second.params[i], args[i], "parameter", ce->line, ce->col);
      return uf->second.ret;
    }

    // Abs und Sgn sind Operatoren (UniExprNode::semant): das Ergebnis hat den
    // Typ des Operanden, und nur int und float sind erlaubt - "Abs(\"3\")"
    // meldet das Original als "Illegal operator for type" (BUG-96).
    const std::string lo = toLower(ce->name);
    if ((lo == "abs" || lo == "sgn") && args.size() == 1) {
      const Ty &t = args[0];
      if (t.object() || t.k == Ty::STR || t.vec) {
        error(ce->line, ce->col,
              "'" + ce->name + "' cannot be applied to " + t.name() +
                  " - only to an int or a float (Blitz3D: \"Illegal operator "
                  "for type\")");
        return Ty();
      }
      return t.numeric() ? t : Ty();
    }

    // Built-in commands: kCommands[] is generated from the runtime headers
    // (tools/gen_commands.py), so what is checked here is exactly what the
    // emitted C++ will call — a rejection can never be a false positive
    // against our own runtime.
    for (const auto &c : kCommands) {
      if (toLower(c.name) != toLower(ce->name)) continue;
      Sig sig = signature(c);
      checkArity(ce, args.size(), sig.required, sig.total);
      for (size_t i = 0; i < args.size() && i < sig.params.size(); ++i)
        checkAssign(sig.params[i], args[i], "parameter", ce->line, ce->col);
      return sig.ret;
    }
    return Ty();
  }

  void checkArity(CallExpr *ce, size_t given, size_t required, size_t total) {
    if (given > total)
      error(ce->line, ce->col, "Too many parameters for '" + ce->name +
                                   "': expected at most " + std::to_string(total) +
                                   ", got " + std::to_string(given));
    else if (given < required)
      error(ce->line, ce->col, "Not enough parameters for '" + ce->name +
                                   "': expected at least " +
                                   std::to_string(required) + ", got " +
                                   std::to_string(given));
  }

  // ---- the generated command table ---------------------------------------
  // Entry format (see commands.h): ret is "%", "#", "$", "" for void or "."
  // when overloads disagree; params is a comma list of "name<type>[?]", where a
  // missing type character means "any" and '?' marks an optional parameter.
  struct Sig {
    Ty ret;
    std::vector<Ty> params;
    size_t required = 0, total = 0;
  };

  static Ty fromTypeChar(char c) {
    switch (c) {
      case '%': return mk(Ty::INT);
      case '#': return mk(Ty::FLOAT);
      case '$': return mk(Ty::STR);
      default:  return Ty(); // '.' or none — any type, checked nowhere
    }
  }

  Sig signature(const CmdInfo &c) {
    auto cached = sigCache_.find(c.name);
    if (cached != sigCache_.end()) return cached->second;

    Sig sig;
    sig.ret = fromTypeChar(c.ret[0] ? c.ret[0] : ' ');

    std::string params = c.params;
    size_t pos = 0;
    while (pos < params.size()) {
      size_t comma = params.find(',', pos);
      std::string tok = params.substr(pos, comma == std::string::npos
                                               ? std::string::npos
                                               : comma - pos);
      pos = (comma == std::string::npos) ? params.size() : comma + 1;
      if (tok.empty()) continue;

      bool optional = tok.back() == '?';
      if (optional) tok.pop_back();
      char type = tok.empty() ? ' ' : tok.back();
      if (type != '%' && type != '#' && type != '$') type = ' ';

      sig.params.push_back(fromTypeChar(type));
      ++sig.total;
      if (!optional) ++sig.required;
    }
    sigCache_[c.name] = sig;
    return sig;
  }

  // ------------------------------------------------------------------ state
  const SourceMap *map_        = nullptr;
  int              errors_     = 0;
  int              blockDepth_ = 0; // 0 = top level of the body being walked
  // Position der Anweisung, die gerade geprueft wird. Das Original
  // meldet "Variable must be a Type" nicht an der Variablen, sondern am
  // Anfang der Anweisung - gemessen an "a = 1 : Print a\x", wo es Spalte
  // 9 nennt und nicht 15 (BUG-74).
  int              stmtLine_   = 0;
  int              stmtCol_    = 0;

  std::unordered_map<std::string, std::unordered_map<std::string, Ty>> types_;
  std::vector<std::string>                            typeNames_;  // as written
  std::unordered_map<std::string, std::vector<std::string>> fieldNames_;
  std::unordered_map<std::string, FuncInfo>  funcs_;
  std::unordered_map<std::string, ArrayInfo> arrays_;
  Scope                                      globals_;
  Scope                                     *scope_ = nullptr;
  Ty                                         returnType_;
  bool                                       inFunction_ = false;
  std::unordered_set<std::string>            constNames_;
  std::unordered_map<std::string, Sig>       sigCache_;
};

#endif // BLITZNEXT_SEMANT_H
