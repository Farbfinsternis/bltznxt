#ifndef BLITZNEXT_SEMANT_H
#define BLITZNEXT_SEMANT_H

#include "ast.h"
#include "lexer.h" // toLower
#include <iostream>
#include <string>
#include <unordered_map>
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
  int analyze(Program *prog, const std::string &fname) {
    filename_ = fname;
    errors_   = 0;
    types_.clear();
    funcs_.clear();
    arrays_.clear();
    globals_.clear();

    collect(prog->nodes);

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
    enum K { UNKNOWN, INT, FLOAT, STR, OBJ } k = UNKNOWN;
    std::string obj; // type name when k == OBJ

    bool numeric() const { return k == INT || k == FLOAT; }
    bool known() const { return k != UNKNOWN; }
    std::string name() const {
      switch (k) {
        case INT:   return "int";
        case FLOAT: return "float";
        case STR:   return "string";
        case OBJ:   return "." + obj;
        default:    return "unknown";
      }
    }
    bool sameAs(const Ty &o) const {
      if (k != o.k) return false;
      return k != OBJ || toLower(obj) == toLower(o.obj);
    }
  };

  static Ty mk(Ty::K k, const std::string &obj = "") {
    Ty t; t.k = k; t.obj = obj; return t;
  }

  // "%"/"" → int, "#"/"!" → float, "$" → string, ".Name" → object
  static Ty fromHint(const std::string &h) {
    if (h == "$") return mk(Ty::STR);
    if (h == "#" || h == "!") return mk(Ty::FLOAT);
    if (!h.empty() && h[0] == '.') return mk(Ty::OBJ, h.substr(1));
    return mk(Ty::INT); // "%" or none — Blitz3D's default
  }

  using Scope = std::unordered_map<std::string, Ty>;

  struct FuncInfo {
    Ty ret;
    std::vector<Ty> params;
  };
  struct ArrayInfo {
    Ty elem;
    size_t dims = 0;
  };

  // ------------------------------------------------------------- diagnostics
  void error(int line, int col, const std::string &msg) {
    std::cerr << filename_ << ":" << line << ":" << (col > 0 ? col : 1)
              << ": error: " << msg << "\n";
    ++errors_;
  }

  // --------------------------------------------------------------- collect
  // Registers types, functions, Dim'd arrays, globals and constants up front,
  // so that forward references work the same way they do in the emitter.
  void collect(const std::vector<std::unique_ptr<ASTNode>> &nodes) {
    for (auto &n : nodes) {
      if (auto *td = dynamic_cast<TypeDecl *>(n.get())) {
        auto &fields = types_[toLower(td->name)];
        for (auto &f : td->fields) fields[toLower(f.name)] = fromHint(f.typeHint);
      } else if (auto *fn = dynamic_cast<FunctionDecl *>(n.get())) {
        FuncInfo fi;
        fi.ret = fromHint(fn->returnHint);
        for (auto &[pname, phint] : fn->params) fi.params.push_back(fromHint(phint));
        funcs_[toLower(fn->name)] = fi;
        collect(fn->body); // Global and Dim may appear inside a function
      } else if (auto *ds = dynamic_cast<DimStmt *>(n.get())) {
        arrays_[toLower(ds->name)] = ArrayInfo{fromHint(ds->typeHint), ds->dims.size()};
      } else if (auto *vd = dynamic_cast<VarDecl *>(n.get())) {
        if (vd->scope == VarDecl::GLOBAL)
          globals_[toLower(vd->name)] = fromHint(vd->typeHint);
      } else if (auto *cd = dynamic_cast<ConstDecl *>(n.get())) {
        globals_[toLower(cd->name)] = fromHint(cd->typeHint);
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

  void checkFunctions(const std::vector<std::unique_ptr<ASTNode>> &nodes) {
    for (auto &n : nodes) {
      if (auto *pr = dynamic_cast<Program *>(n.get())) {
        checkFunctions(pr->nodes);
      } else if (auto *fn = dynamic_cast<FunctionDecl *>(n.get())) {
        Scope local;
        for (auto &[pname, phint] : fn->params)
          local[toLower(pname)] = fromHint(phint);
        scope_      = &local;
        returnType_ = fromHint(fn->returnHint);
        inFunction_ = true;
        for (auto &s : fn->body) stmt(s.get());
        inFunction_ = false;
      }
    }
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

  // string ↔ number is the conversion Blitz3D refuses; everything else is
  // either allowed (int ↔ float) or deliberately not checked here.
  void checkAssign(const Ty &target, const Ty &value, const char *what,
                   int line, int col) {
    if (!target.known() || !value.known()) return;
    if (target.k == Ty::STR && value.numeric())
      error(line, col, std::string(what) + ": cannot assign " + value.name() +
                           " to a string");
    else if (target.numeric() && value.k == Ty::STR)
      error(line, col, std::string(what) + ": cannot assign a string to " +
                           target.name());
  }

  // ------------------------------------------------------------ statements
  void stmt(ASTNode *n) {
    if (!n) return;

    if (auto *pr = dynamic_cast<Program *>(n)) {
      for (auto &s : pr->nodes) stmt(s.get());
    } else if (auto *vd = dynamic_cast<VarDecl *>(n)) {
      Ty t = fromHint(vd->typeHint);
      if (vd->scope == VarDecl::LOCAL) declare(vd->name, t);
      if (vd->initValue)
        checkAssign(t, expr(vd->initValue.get()), "Local", vd->line, vd->col);
    } else if (auto *cd = dynamic_cast<ConstDecl *>(n)) {
      if (cd->value) expr(cd->value.get());
    } else if (auto *as = dynamic_cast<AssignStmt *>(n)) {
      Ty val = expr(as->value.get());
      const Ty *known = lookup(as->name);
      if (known) {
        if (!checkTag(as->name, as->typeHint, as->line, as->col))
          checkAssign(*known, val, "assignment", as->line, as->col);
      } else {
        declare(as->name, as->typeHint.empty() ? val : fromHint(as->typeHint));
      }
    } else if (auto *rd = dynamic_cast<ReadStmt *>(n)) {
      if (lookup(rd->name)) checkTag(rd->name, rd->typeHint, rd->line, rd->col);
      else declare(rd->name, fromHint(rd->typeHint));
    } else if (auto *aas = dynamic_cast<ArrayAssignStmt *>(n)) {
      Ty val = expr(aas->value.get());
      for (auto &i : aas->indices) expr(i.get());
      auto it = arrays_.find(toLower(aas->name));
      if (it != arrays_.end()) {
        if (aas->indices.size() != it->second.dims)
          error(aas->line, aas->col, "array '" + aas->name + "' has " +
                    std::to_string(it->second.dims) + " dimension(s), but " +
                    std::to_string(aas->indices.size()) + " index/indices given");
        checkAssign(it->second.elem, val, "array assignment", aas->line, aas->col);
      }
    } else if (auto *fas = dynamic_cast<FieldAssignStmt *>(n)) {
      Ty obj = expr(fas->object.get());
      Ty val = expr(fas->value.get());
      Ty f   = fieldType(obj, fas->fieldName, fas->line, fas->col);
      checkAssign(f, val, "field assignment", fas->line, fas->col);
    } else if (auto *is = dynamic_cast<IfStmt *>(n)) {
      expr(is->condition.get());
      for (auto &s : is->thenBlock) stmt(s.get());
      for (auto &s : is->elseBlock) stmt(s.get());
    } else if (auto *ws = dynamic_cast<WhileStmt *>(n)) {
      expr(ws->condition.get());
      for (auto &s : ws->block) stmt(s.get());
    } else if (auto *rs = dynamic_cast<RepeatStmt *>(n)) {
      for (auto &s : rs->block) stmt(s.get());
      if (rs->condition) expr(rs->condition.get());
    } else if (auto *fs = dynamic_cast<ForStmt *>(n)) {
      Ty start = expr(fs->start.get());
      expr(fs->end.get());
      if (fs->step) expr(fs->step.get());
      if (!lookup(fs->varName))
        declare(fs->varName, start.numeric() ? start : mk(Ty::INT));
      for (auto &s : fs->block) stmt(s.get());
    } else if (auto *fes = dynamic_cast<ForEachStmt *>(n)) {
      knownType(fes->typeName, fes->line, fes->col);
      declare(fes->varName, mk(Ty::OBJ, fes->typeName));
      for (auto &s : fes->block) stmt(s.get());
    } else if (auto *ss = dynamic_cast<SelectStmt *>(n)) {
      expr(ss->expr.get());
      for (auto &c : ss->cases) {
        for (auto &e : c.expressions) expr(e.get());
        for (auto &s : c.block) stmt(s.get());
      }
      for (auto &s : ss->defaultBlock) stmt(s.get());
    } else if (auto *ret = dynamic_cast<ReturnStmt *>(n)) {
      if (ret->value) {
        Ty v = expr(ret->value.get());
        if (inFunction_) checkAssign(returnType_, v, "Return", ret->line, ret->col);
      }
    } else if (auto *ds = dynamic_cast<DimStmt *>(n)) {
      for (auto &d : ds->dims) expr(d.get());
    } else if (auto *del = dynamic_cast<DeleteStmt *>(n)) {
      expr(del->object.get());
    } else if (auto *ins = dynamic_cast<InsertStmt *>(n)) {
      expr(ins->object.get());
      expr(ins->target.get());
    } else if (auto *ce = dynamic_cast<CallExpr *>(n)) {
      expr(ce);
    }
    // Label/Goto/Gosub/Data/Restore/Exit/End/TypeDecl — nothing to check here
  }

  // ----------------------------------------------------------- expressions
  Ty expr(ExprNode *e) {
    if (!e) return Ty();

    if (auto *le = dynamic_cast<LiteralExpr *>(e)) {
      switch (le->token.type) {
        case TokenType::STRING_LIT: return mk(Ty::STR);
        case TokenType::FLOAT_LIT:  return mk(Ty::FLOAT);
        default:                    return mk(Ty::INT);
      }
    }
    if (auto *ve = dynamic_cast<VarExpr *>(e)) {
      checkTag(ve->name, ve->typeHint, ve->line, ve->col);
      if (const Ty *t = lookup(ve->name)) return *t;
      if (toLower(ve->name) == "pi") return mk(Ty::FLOAT);
      return mk(Ty::INT); // auto-declared on use, as in Blitz3D
    }
    if (auto *aa = dynamic_cast<ArrayAccess *>(e)) {
      for (auto &i : aa->indices) expr(i.get());
      auto it = arrays_.find(toLower(aa->name));
      if (it == arrays_.end()) return Ty();
      if (aa->indices.size() != it->second.dims)
        error(aa->line, aa->col, "array '" + aa->name + "' has " +
                  std::to_string(it->second.dims) + " dimension(s), but " +
                  std::to_string(aa->indices.size()) + " index/indices given");
      return it->second.elem;
    }
    if (auto *fa = dynamic_cast<FieldAccess *>(e)) {
      Ty obj = expr(fa->object.get());
      return fieldType(obj, fa->fieldName, fa->line, fa->col);
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
    if (auto *be = dynamic_cast<BeforeExpr *>(e)) return expr(be->object.get());
    if (auto *ae = dynamic_cast<AfterExpr *>(e))  return expr(ae->object.get());
    if (auto *ce = dynamic_cast<CallExpr *>(e))   return call(ce);
    if (auto *ue = dynamic_cast<UnaryExpr *>(e)) {
      Ty t = expr(ue->expr.get());
      if (ue->op == "NOT") return mk(Ty::INT);
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
    if (comparison) return mk(Ty::INT);

    if (l.k == Ty::OBJ || r.k == Ty::OBJ) {
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
    bool intOnly = (op == "MOD" || op == "SHL" || op == "SHR" || op == "SAR" ||
                    op == "AND" || op == "OR" || op == "XOR");
    if (intOnly) return mk(Ty::INT);
    if (op == "^" || l.k == Ty::FLOAT || r.k == Ty::FLOAT) return mk(Ty::FLOAT);
    if (l.known() && r.known()) return mk(Ty::INT);
    return Ty();
  }

  Ty fieldType(const Ty &obj, const std::string &field, int line, int col) {
    if (obj.k != Ty::OBJ) return Ty();
    auto t = types_.find(toLower(obj.obj));
    if (t == types_.end()) return Ty();
    auto f = t->second.find(toLower(field));
    if (f == t->second.end()) {
      error(line, col, "type '" + obj.obj + "' has no field '" + field + "'");
      return Ty();
    }
    return f->second;
  }

  void knownType(const std::string &name, int line, int col) {
    if (!types_.count(toLower(name)))
      error(line, col, "Type '" + name + "' not found");
  }

  // ------------------------------------------------------------------ calls
  Ty call(CallExpr *ce) {
    std::vector<Ty> args;
    args.reserve(ce->args.size());
    for (auto &a : ce->args) args.push_back(expr(a.get()));

    auto uf = funcs_.find(toLower(ce->name));
    if (uf != funcs_.end()) {
      checkArity(ce, args.size(), uf->second.params.size(),
                 uf->second.params.size());
      for (size_t i = 0; i < args.size() && i < uf->second.params.size(); ++i)
        checkAssign(uf->second.params[i], args[i], "parameter", ce->line, ce->col);
      return uf->second.ret;
    }

    // Built-in commands are deliberately not checked here. kCommands[] in
    // commands.h is hand-maintained prose, not a type contract: "Print" is
    // declared as "value" with no type at all, "Rand" as one parameter where
    // Blitz3D takes one or two, "Text" as five required where two of them are
    // optional. Checking against it rejected 49 of 63 valid test programs and
    // both examples. Arity and parameter types for built-ins become possible
    // once that table is derived from the reference (WEAK-17) — until then a
    // built-in call has an unknown type, which keeps every dependent check
    // silent.
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

  // ------------------------------------------------------------------ state
  std::string filename_;
  int         errors_ = 0;

  std::unordered_map<std::string, std::unordered_map<std::string, Ty>> types_;
  std::unordered_map<std::string, FuncInfo>  funcs_;
  std::unordered_map<std::string, ArrayInfo> arrays_;
  Scope                                      globals_;
  Scope                                     *scope_ = nullptr;
  Ty                                         returnType_;
  bool                                       inFunction_ = false;
};

#endif // BLITZNEXT_SEMANT_H
