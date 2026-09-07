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

  // "%"/"" → int, "#" → float, "$" → string, ".Name" → object
  static Ty fromHint(const std::string &h) {
    if (h == "$") return mk(Ty::STR);
    if (h == "#") return mk(Ty::FLOAT);
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
    std::cerr << map_->format(line, col > 0 ? col : 1)
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
        // The names as they were written, in declaration order: a
        // suggestion should read the way the source spells it, and the
        // order settles a tie reproducibly (WEAK-14, Stufe 2).
        typeNames_.push_back(td->name);
        auto &spelled = fieldNames_[toLower(td->name)];
        for (auto &f : td->fields) spelled.push_back(f.name);
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
      if (vd->scope == VarDecl::GLOBAL && (blockDepth_ > 0 || inFunction_))
        error(vd->line, vd->col,
              "'Global' is only allowed at the top level of the main program, "
              "not inside a block and not inside a function - declare '" +
                  vd->name + "' there and assign to it here");
      if (vd->scope == VarDecl::LOCAL) declare(vd->name, t);
      if (vd->initValue)
        checkAssign(t, expr(vd->initValue.get()), "Local", vd->line, vd->col);
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
        checkArrayTag(aas->name, aas->typeHint, it->second.elem, aas->line,
                      aas->col);
        checkAssign(it->second.elem, val, "array assignment", aas->line, aas->col);
      }
    } else if (auto *fas = dynamic_cast<FieldAssignStmt *>(n)) {
      Ty obj = expr(fas->object.get());
      Ty val = expr(fas->value.get());
      Ty f   = fieldType(obj, fas->fieldName, fas->line, fas->col);
      checkAssign(f, val, "field assignment", fas->line, fas->col);
    } else if (auto *is = dynamic_cast<IfStmt *>(n)) {
      expr(is->condition.get());
      block(is->thenBlock);
      block(is->elseBlock);
    } else if (auto *ws = dynamic_cast<WhileStmt *>(n)) {
      expr(ws->condition.get());
      block(ws->block);
    } else if (auto *rs = dynamic_cast<RepeatStmt *>(n)) {
      block(rs->block);
      if (rs->condition) expr(rs->condition.get());
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
      if (fs->target)
        expr(fs->target.get());
      else if (lookup(fs->varName))
        checkTag(fs->varName, fs->typeHint, fs->line, fs->col);
      else
        declare(fs->varName, fromHint(fs->typeHint));
      block(fs->block);
    } else if (auto *fes = dynamic_cast<ForEachStmt *>(n)) {
      knownType(fes->typeName, fes->line, fes->col);
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
      if (sel.k == Ty::OBJ) {
        int ln = ss->expr->line ? ss->expr->line : ss->line;
        int co = ss->expr->col  ? ss->expr->col  : ss->col;
        error(ln, co,
              "'Select' cannot be used with objects; this expression holds a '"
              + sel.name() + "'");
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
      checkArrayTag(aa->name, aa->typeHint, it->second.elem, aa->line, aa->col);
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
      checkArity(ce, args.size(), uf->second.params.size(),
                 uf->second.params.size());
      for (size_t i = 0; i < args.size() && i < uf->second.params.size(); ++i)
        checkAssign(uf->second.params[i], args[i], "parameter", ce->line, ce->col);
      return uf->second.ret;
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
