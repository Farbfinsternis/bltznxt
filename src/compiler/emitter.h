#ifndef BLITZNEXT_EMITTER_H
#define BLITZNEXT_EMITTER_H

#include "ast.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

class Emitter : public ASTVisitor {
public:
  void emit(Program *prog, const std::string &outputPath) {
    output.str("");
    userFunctions.clear();
    typeNames.clear();
    varObjectTypes.clear();
    declaredVars.clear();
    globalVarNames.clear();
    indentLevel    = 1;
    inExprCtx      = false;
    inFunctionBody = false;
    gosubCount     = 0;

    // First pass: collect function and type names for forward reference.
    for (auto &n : prog->nodes) {
      if (auto *fn = dynamic_cast<FunctionDecl *>(n.get())) {
        std::string lo = fn->name;
        std::transform(lo.begin(), lo.end(), lo.begin(),
               [](unsigned char c){ return (char)std::tolower(c); });
        userFunctions.insert(lo);
      } else if (auto *td = dynamic_cast<TypeDecl *>(n.get())) {
        typeNames.insert(td->name);
      }
    }

    output << "#include \"bb_runtime.h\"\n\n";

    // Emit type struct definitions + linked-list helpers (before functions)
    for (auto &n : prog->nodes)
      if (auto *td = dynamic_cast<TypeDecl *>(n.get()))
        emitTypeDecl(td);

    // Emit global variable declarations at file scope (visible to all functions)
    collectGlobals(prog->nodes);
    if (!globalVarNames.empty()) output << "\n";

    // Emit user function bodies before main()
    for (auto &n : prog->nodes)
      if (dynamic_cast<FunctionDecl *>(n.get()))
        n->accept(this);

    output << "int main(int argc, char** argv) {\n";
    output << "    bbInit(argc, argv);\n";
    output << "    void* __gosub_ret__ = nullptr;\n";

    // Emit Data pool initialisation + label index constants.
    size_t dataIdx = 0;
    collectData(prog->nodes, dataIdx);

    // Emit everything that is not a FunctionDecl or TypeDecl
    for (auto &n : prog->nodes)
      if (!dynamic_cast<FunctionDecl *>(n.get()) &&
          !dynamic_cast<TypeDecl *>(n.get()))
        n->accept(this);

    output << "    bbEnd();\n";
    output << "    return 0;\n";
    output << "}\n";

    std::ofstream f(outputPath + ".cpp");
    if (!f.is_open()) {
      std::cerr << "[Emitter] Cannot open output file: " << outputPath
                << ".cpp\n";
      return;
    }
    f << output.str();
  }

  // ------------------------------------------------------------------ visitors

  void visit(LiteralExpr *node) override {
    if (node->token.type == TokenType::STRING_LIT)
      output << "\"" << node->token.value << "\"";
    else
      output << node->token.value;
  }

  void visit(BinaryExpr *node) override {
    bool prev = inExprCtx;
    inExprCtx  = true;

    if (node->op == "^") {
      // Blitz3D ^ is power, not XOR
      output << "std::pow(";
      node->left->accept(this);
      output << ", ";
      node->right->accept(this);
      output << ")";
    } else {
      output << "(";
      node->left->accept(this);
      output << " " << mapOp(node->op) << " ";
      node->right->accept(this);
      output << ")";
    }

    inExprCtx = prev;
  }

  void visit(UnaryExpr *node) override {
    bool prev = inExprCtx;
    inExprCtx  = true;

    std::string op = node->op;
    if (op == "NOT") op = "!";
    // "~" and +/- pass through as-is
    output << op;
    node->expr->accept(this);

    inExprCtx = prev;
  }

  void visit(VarExpr *node) override {
    // Pi is a Blitz3D built-in constant — map to bb_Pi, never var_Pi
    std::string lo = node->name;
    std::transform(lo.begin(), lo.end(), lo.begin(),
               [](unsigned char c){ return (char)std::tolower(c); });
    if (lo == "pi") {
      output << "bb_Pi";
    } else {
      output << "var_" << node->name;
    }
  }

  void visit(VarDecl *node) override {
    std::string lo = node->name;
    std::transform(lo.begin(), lo.end(), lo.begin(),
               [](unsigned char c){ return (char)std::tolower(c); });

    if (node->scope == VarDecl::GLOBAL) {
      // File-scope declaration already emitted by collectGlobals().
      // Register metadata and only emit initializer assignment if present.
      declaredVars.insert(lo);
      if (!node->typeHint.empty() && node->typeHint[0] == '.')
        varObjectTypes[lo] = node->typeHint.substr(1);
      if (node->initValue) {
        output << ind() << "var_" << node->name << " = ";
        bool prev = inExprCtx; inExprCtx = true;
        node->initValue->accept(this);
        inExprCtx = prev;
        output << ";\n";
      }
      return;
    }

    // LOCAL variable declaration
    auto [type, defVal] = hintToType(node->typeHint);

    output << ind() << type << " var_" << node->name;
    if (node->initValue) {
      output << " = ";
      bool prev = inExprCtx; inExprCtx = true;
      node->initValue->accept(this);
      inExprCtx = prev;
    } else {
      output << " = " << defVal;
    }
    output << ";\n";

    declaredVars.insert(lo);

    // Remember object type for Delete statement code-gen
    if (!node->typeHint.empty() && node->typeHint[0] == '.')
      varObjectTypes[lo] = node->typeHint.substr(1);
  }

  void visit(AssignStmt *node) override {
    std::string lo = node->name;
    std::transform(lo.begin(), lo.end(), lo.begin(),
                   [](unsigned char c){ return (char)std::tolower(c); });
    if (declaredVars.count(lo) == 0) {
      // Implicit global — Blitz3D allows bare assignment without Local/Global
      auto [type, defVal] = hintToType(node->typeHint);
      output << ind() << type << " var_" << node->name << " = ";
      bool prev = inExprCtx; inExprCtx = true;
      node->value->accept(this);
      inExprCtx = prev;
      output << ";\n";
      declaredVars.insert(lo);
    } else {
      output << ind() << "var_" << node->name << " = ";
      bool prev = inExprCtx; inExprCtx = true;
      node->value->accept(this);
      inExprCtx = prev;
      output << ";\n";
    }
  }

  void visit(IfStmt *node) override {
    output << ind() << "if (";
    bool prev = inExprCtx; inExprCtx = true;
    node->condition->accept(this);
    inExprCtx = prev;
    output << ") {\n";

    indentLevel++;
    for (auto &n : node->thenBlock) n->accept(this);
    indentLevel--;

    if (!node->elseBlock.empty()) {
      // Check if the else-block is a single nested IfStmt (ElseIf chain)
      if (node->elseBlock.size() == 1 &&
          dynamic_cast<IfStmt *>(node->elseBlock[0].get())) {
        output << ind() << "} else ";
        // Let the nested IfStmt emit "if (...) {" without leading indent
        emitElseIf(static_cast<IfStmt *>(node->elseBlock[0].get()));
        return;
      }
      output << ind() << "} else {\n";
      indentLevel++;
      for (auto &n : node->elseBlock) n->accept(this);
      indentLevel--;
    }
    output << ind() << "}\n";
  }

  void visit(WhileStmt *node) override {
    output << ind() << "while (";
    bool prev = inExprCtx; inExprCtx = true;
    node->condition->accept(this);
    inExprCtx = prev;
    output << ") {\n";
    indentLevel++;
    for (auto &n : node->block) n->accept(this);
    indentLevel--;
    output << ind() << "}\n";
  }

  void visit(RepeatStmt *node) override {
    if (node->condition) {
      // Repeat … Until cond  →  do { … } while (!(cond));
      output << ind() << "do {\n";
      indentLevel++;
      for (auto &n : node->block) n->accept(this);
      indentLevel--;
      output << ind() << "} while (!(";
      bool prev = inExprCtx; inExprCtx = true;
      node->condition->accept(this);
      inExprCtx = prev;
      output << "));\n";
    } else {
      // Repeat … Forever  →  while (true) { … }
      output << ind() << "while (true) {\n";
      indentLevel++;
      for (auto &n : node->block) n->accept(this);
      indentLevel--;
      output << ind() << "}\n";
    }
  }

  void visit(ForStmt *node) override {
    if (node->step) {
      // When a STEP is given, support negative steps via a ternary condition.
      // Emit as a block-scoped while loop so var and step temps don't leak.
      output << ind() << "{\n";
      indentLevel++;

      output << ind() << "auto var_" << node->varName << " = ";
      emitExpr(node->start.get());
      output << ";\n";

      output << ind() << "const auto _end_" << node->varName << " = ";
      emitExpr(node->end.get());
      output << ";\n";

      output << ind() << "const auto _step_" << node->varName << " = ";
      emitExpr(node->step.get());
      output << ";\n";

      output << ind() << "for (; (_step_" << node->varName
             << " > 0 ? var_" << node->varName
             << " <= _end_" << node->varName
             << " : var_" << node->varName
             << " >= _end_" << node->varName
             << "); var_" << node->varName
             << " += _step_" << node->varName << ") {\n";

      indentLevel++;
      for (auto &n : node->block) n->accept(this);
      indentLevel--;
      output << ind() << "}\n";
      indentLevel--;
      output << ind() << "}\n";

    } else {
      // Simple ascending loop without STEP
      output << ind() << "for (auto var_" << node->varName << " = ";
      emitExpr(node->start.get());
      output << "; var_" << node->varName << " <= ";
      emitExpr(node->end.get());
      output << "; ++var_" << node->varName << ") {\n";
      indentLevel++;
      for (auto &n : node->block) n->accept(this);
      indentLevel--;
      output << ind() << "}\n";
    }
  }

  void visit(SelectStmt *node) override {
    output << ind() << "{\n";
    indentLevel++;
    output << ind() << "auto _sel_ = ";
    emitExpr(node->expr.get());
    output << ";\n";

    bool first = true;
    for (auto &c : node->cases) {
      output << ind() << (first ? "if" : "else if") << " (";
      bool prev = inExprCtx; inExprCtx = true;
      for (size_t i = 0; i < c.expressions.size(); ++i) {
        output << "_sel_ == ";
        c.expressions[i]->accept(this);
        if (i + 1 < c.expressions.size()) output << " || ";
      }
      inExprCtx = prev;
      output << ") {\n";
      indentLevel++;
      for (auto &n : c.block) n->accept(this);
      indentLevel--;
      output << ind() << "}\n";
      first = false;
    }

    if (!node->defaultBlock.empty()) {
      output << ind() << "else {\n";
      indentLevel++;
      for (auto &n : node->defaultBlock) n->accept(this);
      indentLevel--;
      output << ind() << "}\n";
    }

    indentLevel--;
    output << ind() << "}\n";
  }

  void visit(CallExpr *node) override {
    bool isStmt = !inExprCtx;
    if (isStmt) output << ind();

    // Decide bb_ prefix: use it for built-ins, not for user functions
    std::string lo = node->name;
    std::transform(lo.begin(), lo.end(), lo.begin(),
               [](unsigned char c){ return (char)std::tolower(c); });
    bool isUser = (userFunctions.count(lo) > 0);

    if (isUser)
      output << node->name << "(";
    else
      output << "bb_" << node->name << "(";

    bool prev = inExprCtx; inExprCtx = true;
    for (size_t i = 0; i < node->args.size(); ++i) {
      node->args[i]->accept(this);
      if (i + 1 < node->args.size()) output << ", ";
    }
    inExprCtx = prev;

    output << ")";
    if (isStmt) output << ";\n";
  }

  void visit(FunctionDecl *node) override {
    // Determine return type — default auto; could be improved with type hints
    output << "auto " << node->name << "(";
    for (size_t i = 0; i < node->params.size(); ++i) {
      auto &[pname, phint] = node->params[i];
      auto [ptype, defVal] = hintToType(phint);
      output << ptype << " var_" << pname;
      if (i + 1 < node->params.size()) output << ", ";
    }
    output << ") {\n";

    inFunctionBody = true;
    indentLevel = 1;
    for (auto &n : node->body) n->accept(this);
    inFunctionBody = false;

    output << "}\n\n";
    indentLevel = 1; // reset for next function / main
  }

  void visit(ReturnStmt *node) override {
    if (node->value || inFunctionBody) {
      // Normal function return (with or without value)
      output << ind() << "return";
      if (node->value) {
        output << " ";
        emitExpr(node->value.get());
      }
      output << ";\n";
    } else {
      // Bare Return in main body = jump back to Gosub call site
      output << ind() << "goto *__gosub_ret__;\n";
    }
  }

  void visit(ConstDecl *node) override {
    bool prev = inExprCtx; inExprCtx = true;
    if (node->typeHint == "$") {
      output << ind() << "const bbString var_" << node->name << " = ";
    } else {
      std::string type = "auto";
      if      (node->typeHint == "%") type = "int";
      else if (node->typeHint == "#" || node->typeHint == "!") type = "float";
      output << ind() << "constexpr " << type << " var_" << node->name << " = ";
    }
    node->value->accept(this);
    inExprCtx = prev;
    output << ";\n";
  }

  void visit(DimStmt *node) override {
    auto [elemType, defVal] = hintToType(node->typeHint);
    // % → int, no hint → int (handled by hintToType)

    size_t ndim = node->dims.size();
    output << ind() << buildVecType(elemType, ndim) << " var_" << node->name;
    emitVectorCtor(node->dims, elemType, 0);
    output << ";\n";
  }

  void visit(ArrayAccess *node) override {
    output << "var_" << node->name;
    for (auto &idx : node->indices) {
      output << "[";
      emitExpr(idx.get());
      output << "]";
    }
  }

  void visit(ArrayAssignStmt *node) override {
    output << ind() << "var_" << node->name;
    for (auto &idx : node->indices) {
      output << "[";
      emitExpr(idx.get());
      output << "]";
    }
    output << " = ";
    emitExpr(node->value.get());
    output << ";\n";
  }

  void visit(DataStmt *node) override {
    // All Data values are collected up-front in collectData(); skip at runtime.
    (void)node;
  }

  void visit(ReadStmt *node) override {
    // Determine C++ type for the variable from the type hint.
    auto [type, defVal] = hintToType(node->typeHint);

    std::string lo = node->name;
    std::transform(lo.begin(), lo.end(), lo.begin(),
               [](unsigned char c){ return (char)std::tolower(c); });

    if (declaredVars.count(lo) == 0) {
      // Auto-declare the variable (Blitz3D allows implicit declaration)
      output << ind() << type << " var_" << node->name
             << " = (" << type << ")bb_DataRead();\n";
      declaredVars.insert(lo);
    } else {
      output << ind() << "var_" << node->name
             << " = (" << type << ")bb_DataRead();\n";
    }
  }

  void visit(RestoreStmt *node) override {
    if (node->label.empty()) {
      output << ind() << "bb_DataRestore();\n";
    } else {
      // Label-based Restore: reset to the data index recorded at that label.
      output << ind() << "bb_DataRestore(__data_at_" << node->label << "__);\n";
    }
  }

  void visit(TypeDecl *node) override {
    // Emitted via emitTypeDecl() before main(); skipped in main loop.
    (void)node;
  }

  // New TypeName → bb_TypeName_New()
  void visit(NewExpr *node) override {
    output << "bb_" << node->typeName << "_New()";
  }

  // Delete obj → bb_TypeName_Delete(expr); [var = nullptr if simple var]
  void visit(DeleteStmt *node) override {
    std::string typeName = getExprTypeName(node->object.get());
    if (!typeName.empty()) {
      output << ind() << "bb_" << typeName << "_Delete(";
      emitExpr(node->object.get());
      output << ");\n";
      // Null out the local variable to prevent use-after-free
      if (auto *ve = dynamic_cast<VarExpr *>(node->object.get()))
        output << ind() << "var_" << ve->name << " = nullptr;\n";
    } else {
      // Type indeterminate at compile time — warn and best-effort null
      std::cerr << "[warning] Delete: type indeterminate at compile time"
                << " (line " << node->line << ")\n";
      output << ind();
      emitExpr(node->object.get());
      output << " = nullptr; // Delete (type unknown)\n";
    }
  }

  // First TypeName → bb_TypeName_head_
  void visit(FirstExpr *node) override {
    output << "bb_" << node->typeName << "_head_";
  }

  // Last TypeName → bb_TypeName_tail_
  void visit(LastExpr *node) override {
    output << "bb_" << node->typeName << "_tail_";
  }

  // Before(obj) → (obj)->__prev__
  void visit(BeforeExpr *node) override {
    output << "(";
    emitExpr(node->object.get());
    output << ")->__prev__";
  }

  // After(obj) → (obj)->__next__
  void visit(AfterExpr *node) override {
    output << "(";
    emitExpr(node->object.get());
    output << ")->__next__";
  }

  // Insert obj Before/After target → bb_TypeName_InsertBefore/After(obj, target)
  void visit(InsertStmt *node) override {
    std::string typeName = getExprTypeName(node->object.get());
    if (typeName.empty()) typeName = getExprTypeName(node->target.get());
    if (typeName.empty()) {
      output << ind() << "// Insert: could not determine type\n";
      return;
    }
    std::string fn = (node->mode == InsertStmt::BEFORE)
                     ? "bb_" + typeName + "_InsertBefore"
                     : "bb_" + typeName + "_InsertAfter";
    output << ind() << fn << "(";
    emitExpr(node->object.get());
    output << ", ";
    emitExpr(node->target.get());
    output << ");\n";
  }

  // For Each p.TypeName ... Next
  // Emits a deletion-safe while loop that caches __next__ before each body run.
  void visit(ForEachStmt *node) override {
    output << ind() << "{\n";
    indentLevel++;
    output << ind() << "auto *bb_fe_" << node->varName << "_ = bb_"
           << node->typeName << "_head_;\n";
    output << ind() << "while (bb_fe_" << node->varName << "_) {\n";
    indentLevel++;
    output << ind() << "auto *var_" << node->varName << " = bb_fe_"
           << node->varName << "_;\n";
    output << ind() << "bb_fe_" << node->varName << "_ = bb_fe_"
           << node->varName << "_->__next__;\n";
    // Register the iteration variable in varObjectTypes for nested field access
    std::string lo = node->varName;
    std::transform(lo.begin(), lo.end(), lo.begin(),
               [](unsigned char c){ return (char)std::tolower(c); });
    varObjectTypes[lo] = node->typeName;
    for (auto &n : node->block) n->accept(this);
    indentLevel--;
    output << ind() << "}\n";
    indentLevel--;
    output << ind() << "}\n";
  }

  // obj\field — emits as pointer member access: obj->var_field
  void visit(FieldAccess *node) override {
    bool prev = inExprCtx; inExprCtx = true;
    node->object->accept(this);
    inExprCtx = prev;
    output << "->var_" << node->fieldName;
  }

  // obj\field = expr
  void visit(FieldAssignStmt *node) override {
    output << ind();
    bool prev = inExprCtx; inExprCtx = true;
    node->object->accept(this);
    inExprCtx = prev;
    output << "->var_" << node->fieldName << " = ";
    emitExpr(node->value.get());
    output << ";\n";
  }

  void visit(LabelStmt *node) override {
    // Labels must be followed by a statement in C++; use null statement.
    output << "lbl_" << node->name << ":;\n";
  }

  void visit(GotoStmt *node) override {
    output << ind() << "goto lbl_" << node->label << ";\n";
  }

  void visit(GosubStmt *node) override {
    int n = ++gosubCount;
    // Computed-goto trick (GCC extension, supported by MinGW):
    //   __gosub_ret__ holds the address of the unique return label.
    output << ind() << "__gosub_ret__ = &&_gosub_ret_" << n << "_;\n";
    output << ind() << "goto lbl_" << node->label << ";\n";
    output << ind() << "_gosub_ret_" << n << "_:;\n";
  }

  void visit(ExitStmt *node) override {
    output << ind() << "break;\n";
  }

  void visit(EndStmt *node) override {
    output << ind() << "bbEnd(); return 0;\n";
  }

  void visit(Program *node) override {
    for (auto &n : node->nodes) n->accept(this);
  }

private:
  std::stringstream          output;
  std::unordered_set<std::string> userFunctions;
  std::unordered_set<std::string> typeNames;          // registered Type names
  std::unordered_set<std::string> declaredVars;       // lowercase declared var names
  std::unordered_set<std::string> globalVarNames;     // lowercase names of file-scope globals
  std::unordered_map<std::string, std::string> varObjectTypes; // lowercase var → TypeName
  int  indentLevel    = 1;
  bool inExprCtx      = false;
  bool inFunctionBody = false;
  int  gosubCount     = 0;

  std::string ind() const {
    return std::string(static_cast<size_t>(indentLevel) * 4, ' ');
  }

  // Emit a node as a pure expression (no leading indent, no trailing semicolon)
  void emitExpr(ASTNode *node) {
    bool prev = inExprCtx;
    inExprCtx  = true;
    node->accept(this);
    inExprCtx = prev;
  }

  // Emit a full C++ struct + intrusive linked-list management for a TypeDecl.
  // Must be called before main() so that variable declarations can use the type.
  void emitTypeDecl(TypeDecl *td) {
    const std::string sname = "bb_" + td->name;

    // Use "struct bb_TypeName *" (elaborated type specifier) throughout so that
    // user-defined types like "Type Rect" don't collide with runtime functions
    // that share the same bb_ prefix (e.g. bb_Rect from bb_graphics2d.h).
    // In C++, a struct tag can always be named unambiguously via "struct X" even
    // when a function named X is in scope.
    const std::string spname = "struct " + sname + " *";  // pointer type

    // Struct definition
    output << "struct " << sname << " {\n";
    for (auto &f : td->fields) {
      auto [ftype, fdefault] = hintToType(f.typeHint);
      output << "    " << ftype << " var_" << f.name << " = " << fdefault << ";\n";
    }
    output << "    " << spname << "__next__ = nullptr;\n";
    output << "    " << spname << "__prev__ = nullptr;\n";
    output << "};\n";

    // Global linked-list head/tail
    output << "inline " << spname << "bb_" << td->name << "_head_ = nullptr;\n";
    output << "inline " << spname << "bb_" << td->name << "_tail_ = nullptr;\n";

    // bb_TypeName_New() — allocate + append to tail of list
    output << "inline " << spname << "bb_" << td->name << "_New() {\n";
    output << "    struct " << sname << " *p = new struct " << sname << ";\n";
    output << "    p->__prev__ = bb_" << td->name << "_tail_;\n";
    output << "    p->__next__ = nullptr;\n";
    output << "    if (bb_" << td->name << "_tail_) bb_" << td->name << "_tail_->__next__ = p;\n";
    output << "    else bb_" << td->name << "_head_ = p;\n";
    output << "    bb_" << td->name << "_tail_ = p;\n";
    output << "    return p;\n";
    output << "}\n";

    // bb_TypeName_Delete(p) — unlink from list + free
    output << "inline void bb_" << td->name << "_Delete(" << spname << "p) {\n";
    output << "    if (!p) return;\n";
    output << "    if (p->__prev__) p->__prev__->__next__ = p->__next__;\n";
    output << "    else bb_" << td->name << "_head_ = p->__next__;\n";
    output << "    if (p->__next__) p->__next__->__prev__ = p->__prev__;\n";
    output << "    else bb_" << td->name << "_tail_ = p->__prev__;\n";
    output << "    delete p;\n";
    output << "}\n";

    // Helper: unlink p from wherever it currently sits in the list
    // (shared logic used by InsertBefore / InsertAfter)
    output << "inline void bb_" << td->name << "_Unlink(" << spname << "p) {\n";
    output << "    if (p->__prev__) p->__prev__->__next__ = p->__next__;\n";
    output << "    else bb_" << td->name << "_head_ = p->__next__;\n";
    output << "    if (p->__next__) p->__next__->__prev__ = p->__prev__;\n";
    output << "    else bb_" << td->name << "_tail_ = p->__prev__;\n";
    output << "    p->__prev__ = p->__next__ = nullptr;\n";
    output << "}\n";

    // bb_TypeName_InsertBefore(obj, target) — place obj immediately before target
    output << "inline void bb_" << td->name << "_InsertBefore(" << spname << "obj, " << spname << "target) {\n";
    output << "    if (!obj || !target || obj == target) return;\n";
    output << "    bb_" << td->name << "_Unlink(obj);\n";
    output << "    obj->__next__ = target;\n";
    output << "    obj->__prev__ = target->__prev__;\n";
    output << "    if (target->__prev__) target->__prev__->__next__ = obj;\n";
    output << "    else bb_" << td->name << "_head_ = obj;\n";
    output << "    target->__prev__ = obj;\n";
    output << "}\n";

    // bb_TypeName_InsertAfter(obj, target) — place obj immediately after target
    output << "inline void bb_" << td->name << "_InsertAfter(" << spname << "obj, " << spname << "target) {\n";
    output << "    if (!obj || !target || obj == target) return;\n";
    output << "    bb_" << td->name << "_Unlink(obj);\n";
    output << "    obj->__prev__ = target;\n";
    output << "    obj->__next__ = target->__next__;\n";
    output << "    if (target->__next__) target->__next__->__prev__ = obj;\n";
    output << "    else bb_" << td->name << "_tail_ = obj;\n";
    output << "    target->__next__ = obj;\n";
    output << "}\n\n";
  }

  // Helper for ElseIf chains: emit "if (...) { … }" without a leading newline
  void emitElseIf(IfStmt *node) {
    output << "if (";
    bool prev = inExprCtx; inExprCtx = true;
    node->condition->accept(this);
    inExprCtx = prev;
    output << ") {\n";

    indentLevel++;
    for (auto &n : node->thenBlock) n->accept(this);
    indentLevel--;

    if (!node->elseBlock.empty()) {
      if (node->elseBlock.size() == 1 &&
          dynamic_cast<IfStmt *>(node->elseBlock[0].get())) {
        output << ind() << "} else ";
        emitElseIf(static_cast<IfStmt *>(node->elseBlock[0].get()));
        return;
      }
      output << ind() << "} else {\n";
      indentLevel++;
      for (auto &n : node->elseBlock) n->accept(this);
      indentLevel--;
    }
    output << ind() << "}\n";
  }

  // Build nested std::vector type string for an N-dimensional array.
  // buildVecType("int", 2) → "std::vector<std::vector<int>>"
  static std::string buildVecType(const std::string &elemType, size_t ndim) {
    if (ndim == 0) return elemType;
    return "std::vector<" + buildVecType(elemType, ndim - 1) + ">";
  }

  // Emit the constructor argument list for a nested vector.
  // Blitz3D Dim a(N) creates indices 0..N → size N+1.
  // 1D: (N + 1)
  // 2D: (N + 1, std::vector<int>(M + 1))
  void emitVectorCtor(const std::vector<std::unique_ptr<ExprNode>> &dims,
                      const std::string &elemType, size_t idx) {
    output << "(";
    emitExpr(dims[idx].get());
    output << " + 1";
    if (idx + 1 < dims.size()) {
      output << ", ";
      output << buildVecType(elemType, dims.size() - idx - 1);
      emitVectorCtor(dims, elemType, idx + 1);
    }
    output << ")";
  }

  // Recursively collect all DataStmt values and push them into the pool.
  // Walks the full AST so Data inside any block also works.
  // Also records `const size_t __data_at_label__ = N;` for every LabelStmt
  // encountered, enabling "Restore labelname" to jump to the right pool index.
  void collectData(const std::vector<std::unique_ptr<ASTNode>> &nodes,
                   size_t &idx) {
    for (auto &n : nodes) {
      if (auto *lbl = dynamic_cast<LabelStmt *>(n.get())) {
        // Capture the pool index at this label so "Restore lbl" can use it.
        output << "    const size_t __data_at_" << lbl->name
               << "__ = " << idx << ";\n";
      } else if (auto *ds = dynamic_cast<DataStmt *>(n.get())) {
        for (auto &tok : ds->values) {
          output << "    bb_data_pool_.push_back(";
          if (tok.type == TokenType::STRING_LIT) {
            output << "bb_DataVal(bbString(\"" << tok.value << "\"))";
          } else if (tok.type == TokenType::FLOAT_LIT) {
            output << "bb_DataVal(" << tok.value << "f)";
          } else { // INT_LIT (or signed numeric)
            output << "bb_DataVal(" << tok.value << ")";
          }
          output << ");\n";
          ++idx;
        }
      } else if (auto *prog = dynamic_cast<Program *>(n.get())) {
        collectData(prog->nodes, idx);
      } else if (auto *fn = dynamic_cast<FunctionDecl *>(n.get())) {
        collectData(fn->body, idx);
      } else if (auto *if_ = dynamic_cast<IfStmt *>(n.get())) {
        collectData(if_->thenBlock, idx);
        collectData(if_->elseBlock, idx);
      } else if (auto *wh = dynamic_cast<WhileStmt *>(n.get())) {
        collectData(wh->block, idx);
      } else if (auto *rp = dynamic_cast<RepeatStmt *>(n.get())) {
        collectData(rp->block, idx);
      } else if (auto *fr = dynamic_cast<ForStmt *>(n.get())) {
        collectData(fr->block, idx);
      } else if (auto *sel = dynamic_cast<SelectStmt *>(n.get())) {
        for (auto &c : sel->cases) collectData(c.block, idx);
        collectData(sel->defaultBlock, idx);
      } else if (auto *fe = dynamic_cast<ForEachStmt *>(n.get())) {
        collectData(fe->block, idx);
      }
    }
  }

  // Scan AST nodes for Global VarDecl and emit file-scope C++ declarations.
  // Recurses into Program wrappers and FunctionDecl bodies (Global can appear
  // anywhere in Blitz3D and still creates a true global variable).
  void collectGlobals(const std::vector<std::unique_ptr<ASTNode>> &nodes) {
    for (auto &n : nodes) {
      if (auto *prog = dynamic_cast<Program *>(n.get())) {
        collectGlobals(prog->nodes);
      } else if (auto *fn = dynamic_cast<FunctionDecl *>(n.get())) {
        collectGlobals(fn->body);
      } else if (auto *vd = dynamic_cast<VarDecl *>(n.get())) {
        if (vd->scope != VarDecl::GLOBAL) continue;
        std::string lo = vd->name;
        std::transform(lo.begin(), lo.end(), lo.begin(),
               [](unsigned char c){ return (char)std::tolower(c); });
        if (globalVarNames.count(lo)) continue; // skip duplicates
        globalVarNames.insert(lo);
        declaredVars.insert(lo); // prevent implicit re-declaration inside functions
        auto [type, defVal] = hintToType(vd->typeHint);
        output << type << " var_" << vd->name << " = " << defVal << ";\n";
      }
    }
  }

  // Try to determine the type name of an expression from varObjectTypes.
  // Returns "" if the type cannot be determined statically.
  std::string getExprTypeName(ExprNode *expr) {
    if (auto *ve = dynamic_cast<VarExpr *>(expr)) {
      std::string lo = ve->name;
      std::transform(lo.begin(), lo.end(), lo.begin(),
               [](unsigned char c){ return (char)std::tolower(c); });
      auto it = varObjectTypes.find(lo);
      if (it != varObjectTypes.end()) return it->second;
    }
    if (auto *fe = dynamic_cast<FirstExpr *>(expr))  return fe->typeName;
    if (auto *le = dynamic_cast<LastExpr *>(expr))   return le->typeName;
    // Before(p) / After(p) have the same type as p — recurse
    if (auto *be = dynamic_cast<BeforeExpr *>(expr)) return getExprTypeName(be->object.get());
    if (auto *ae = dynamic_cast<AfterExpr *>(expr))  return getExprTypeName(ae->object.get());
    return "";
  }

  // Returns {cppType, defaultValue} for a Blitz3D type hint.
  // "$" → {"bbString", "\"\""}, "#"/"!" → {"float", "0.0f"},
  // ".Vec" → {"bb_Vec *", "nullptr"}, "%"/empty → {"int", "0"}
  static std::pair<std::string, std::string>
  hintToType(const std::string &hint) {
    if (hint == "$")
      return {"bbString", "\"\""};
    if (hint == "#" || hint == "!")
      return {"float", "0.0f"};
    if (!hint.empty() && hint[0] == '.')
      return {"struct bb_" + hint.substr(1) + " *", "nullptr"};
    return {"int", "0"}; // "%" or empty → int
  }

  // Map Blitz3D operators to their C++ equivalents
  static std::string mapOp(const std::string &op) {
    if (op == "=")   return "==";
    if (op == "<>")  return "!=";
    if (op == "AND") return "&";
    if (op == "OR")  return "|";
    if (op == "XOR") return "^";
    if (op == "MOD") return "%";
    if (op == "SHL") return "<<";
    if (op == "SHR") return ">>";
    if (op == "SAR") return ">>";
    return op; // +  -  *  /  <  >  <=  >=
  }
};

#endif // BLITZNEXT_EMITTER_H
