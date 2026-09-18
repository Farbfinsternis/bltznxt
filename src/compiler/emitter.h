#ifndef BLITZNEXT_EMITTER_H
#define BLITZNEXT_EMITTER_H

#include "ast.h"
#include "commands.h"
#include "lexer.h" // toLower
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

class Emitter : public ASTVisitor {
  // Zielmarke fuer ein festes Array in einer Parameterliste: es wird als
  // Referenz uebergeben, also darf convFor() nichts darum wickeln.
  static constexpr const char *kVecHint = "[]";

  // Ein Name, den ein Rumpf anlegt: Schreibweise, Tag und - fuer
  // "Local a[3]" - die Groesse des festen Arrays (BUG-59). Steht hier
  // oben, weil collectLocals() den Typ in seiner Parameterliste fuehrt.
  struct LocalDecl {
    std::string name;   // kleingeschrieben
    std::string hint;
    ExprNode   *vecSize = nullptr;
  };

public:
  void emit(Program *prog, const std::string &outputPath) {
    output.str("");
    userFunctions.clear();
    typeNames.clear();
    varObjectTypes.clear();
    dimObjectTypes_.clear();
    dimHints_.clear();
    declaredVars.clear();
    globalVarNames.clear();
    hoistedConsts_.clear();
    pinned_.clear();
    pinCount_ = 0;
    userFuncDecls_.clear();
    writesState_.clear();
    writesBusy_.clear();
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
        userFuncDecls_[lo] = fn;
      } else if (auto *td = dynamic_cast<TypeDecl *>(n.get())) {
        typeNames.insert(toLower(td->name));
      }
    }

    output << "#include \"bb_runtime.h\"\n\n";

    // Alle Typnamen vorab deklarieren, damit ein Feld auf einen erst
    // spaeter erklaerten Typ zeigen darf: in KBSplines.bb steht "Field
    // keylist.KeyFrame[...]" in Type Motion, und Type KeyFrame kommt
    // danach. Fuer ein blosses "struct X *" genuegte die implizite
    // Deklaration, innerhalb eines Template-Arguments
    // (std::array<struct X *, N>) ist sie unnoetig heikel.
    {
      bool anyType = false;
      for (auto &n : prog->nodes)
        if (auto *td = dynamic_cast<TypeDecl *>(n.get())) {
          output << "struct bb_" << toLower(td->name) << ";\n";
          anyType = true;
        }
      if (anyType) output << "\n";
    }

    // Emit constants at file scope. In Blitz3D a Const belongs to the whole
    // program, not to the statement stream: parseStmtSeq puts it into its own
    // list (consts->push_back( parseVarDecl( DECL_GLOBAL,true ) )). Emitting it
    // inside main() would hide it from every function (BUG-33).
    //
    // Vor den Typen, nicht danach: seit BUG-59 darf ein Feld ein festes
    // Array sein, und dessen Groesse ist in C++ Teil des Typs. In
    // KBSplines.bb steht "Field keylist.KeyFrame[nkeyframes-1]" - stuende
    // das constexpr erst hinter dem Struct, waere der Name dort unbekannt.
    // Umgekehrt kann ein Const nie einen Typ brauchen: sein Wert muss
    // konstant sein.
    collectConsts(prog->nodes);

    // Emit type struct definitions + linked-list helpers (before functions)
    for (auto &n : prog->nodes)
      if (auto *td = dynamic_cast<TypeDecl *>(n.get()))
        emitTypeDecl(td);

    // Emit global variable declarations at file scope (visible to all functions)
    collectGlobals(prog->nodes);

    // Forward-declare all Dim'd arrays as empty vectors at file scope so they
    // are visible to user functions (like globals) and forward references
    // (array used before its Dim in the token stream) compile correctly in C++.
    // The actual size-initialisation is emitted at the original Dim position
    // inside main() by visit(DimStmt*) as an assignment.
    hoistedDims_.clear();
    collectDims(prog->nodes);
    if (!globalVarNames.empty() || !hoistedDims_.empty() ||
        !hoistedConsts_.empty()) output << "\n";

    // Forward-declare every function first, so that calls do not depend on
    // the order of definition — mutual recursion included.
    bool anyFn = false;
    for (auto &n : prog->nodes)
      if (auto *fn = dynamic_cast<FunctionDecl *>(n.get())) {
        emitFunctionSignature(fn, /*withDefaults=*/true);
        output << ";\n";
        anyFn = true;
      }
    if (anyFn) output << "\n";

    // Emit user function bodies before main()
    for (auto &n : prog->nodes)
      if (dynamic_cast<FunctionDecl *>(n.get()))
        n->accept(this);

    output << "int main(int argc, char** argv) {\n";
    output << "    bbInit(argc, argv);\n";
    output << "    int __gosub_ret__ = 0;\n";

    // Emit Data pool initialisation + label index constants.
    size_t dataIdx = 0;
    collectData(prog->nodes, dataIdx);

    // Declare the locals of main() up front when it contains a label.
    hoistedLocals_.clear();
    hoistLocals(prog->nodes);

    // Emit everything that is not a FunctionDecl or TypeDecl
    for (auto &n : prog->nodes)
      if (!dynamic_cast<FunctionDecl *>(n.get()) &&
          !dynamic_cast<TypeDecl *>(n.get()))
        emitStmt(n.get());

    output << "    bbEnd();\n";
    output << "    return 0;\n";
    // Gosub return dispatch table — only emitted when Gosub is used.
    // Placed after return 0 so it never executes via fall-through;
    // only reachable via "goto __gosub_dispatch__" from a bare Return.
    if (gosubCount > 0) {
      output << "    __gosub_dispatch__:\n";
      output << "    switch (__gosub_ret__) {\n";
      for (int i = 1; i <= gosubCount; ++i)
        output << "      case " << i << ": goto _gosub_ret_" << i << "_;\n";
      output << "    }\n";
    }
    output << "}\n";

    std::ofstream f(outputPath + ".cpp");
    if (!f.is_open()) {
      std::cerr << "[Emitter] Cannot open output file: " << outputPath
                << ".cpp\n";
      return;
    }
    f << output.str();
  }

  // ------------------------------------------------------------------ helpers

  // Escapes a raw Blitz3D string value for embedding in a C++ string literal.
  // In Blitz3D strings are byte-literal (no escape processing), so any
  // backslash or double-quote in the source must be escaped for C++.
  static std::string escapeCppString(const std::string &s) {
    std::string out;
    out.reserve(s.size());
    for (unsigned char c : s) {
      switch (c) {
        case '\\': out += "\\\\"; break;
        case '"':  out += "\\\""; break;
        case '\r': out += "\\r";  break;
        case '\t': out += "\\t";  break;
        default:
          if (c < 0x20) {
            // Other control characters → \xNN
            char buf[5];
            std::snprintf(buf, sizeof(buf), "\\x%02x", c);
            out += buf;
          } else {
            out += static_cast<char>(c);
          }
      }
    }
    return out;
  }

  // ------------------------------------------------------------------ visitors

  void visit(LiteralExpr *node) override {
    // Wrapped in bbString: a bare C++ literal is a const char*, so
    // "text" + n would be pointer arithmetic instead of concatenation.
    if (node->token.type == TokenType::STRING_LIT)
      output << "bbString(\"" << escapeCppString(node->token.value) << "\")";
    else
      output << node->token.value;
  }

  void visit(BinaryExpr *node) override {
    bool prev = inExprCtx;
    inExprCtx  = true;

    if (node->op == "^") {
      // Blitz3D ^ is power, not XOR
      output << "std::pow(";
      emitOperand(node->left.get());
      output << ", ";
      emitOperand(node->right.get());
      output << ")";
    } else if (node->op == "MOD") {
      // C++ "%" is integers only, Blitz3D's Mod is not: the reference emits
      // __bbMod for two ints and __bbFMod (fmod) as soon as one side is a
      // float. _bb_mod in bb_math.h makes that same choice from the operand
      // types, which is where the emitter has to leave it - it has no type
      // information of its own (BUG-73).
      output << "_bb_mod(";
      emitOperand(node->left.get());
      output << ", ";
      emitOperand(node->right.get());
      output << ")";
    } else if (node->op == "SHR") {
      // SHR is a logical (unsigned) right shift — cast left operand to unsigned
      output << "((int)((unsigned int)(";
      emitIntegerContext(node->left.get());
      output << ") >> (";
      emitIntegerContext(node->right.get());
      output << ")))";
    } else if (node->op == "AND" || node->op == "OR" || node->op == "XOR" ||
               node->op == "SHL" || node->op == "SAR") {
      // BinExprNode::semant wandelt beide Seiten nach int, mit derselben
      // CastNode wie eine Bedingung: ein String per atoi, ein Float gerundet
      // (BUG-54). Ohne die Wandlung lehnte g++ "x# And 3" ab
      // ("invalid operands of types 'float' and 'int'").
      output << "(";
      emitIntegerContext(node->left.get());
      output << " " << mapOp(node->op) << " ";
      emitIntegerContext(node->right.get());
      output << ")";
    } else {
      output << "(";
      emitOperand(node->left.get());
      output << " " << mapOp(node->op) << " ";
      emitOperand(node->right.get());
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
    // No name needs special treatment here: Pi is a reserved word since
    // BUG-35 and reaches the emitter as a float literal, never as a VarExpr.
    output << "var_" << toLower(node->name);
  }

  void visit(VarDecl *node) override {
    std::string lo = node->name;
    std::transform(lo.begin(), lo.end(), lo.begin(),
               [](unsigned char c){ return (char)std::tolower(c); });

    if (node->scope == VarDecl::GLOBAL) {
      // File-scope declaration already emitted by collectGlobals().
      // Register metadata and only emit initializer assignment if present.
      declaredVars.insert(lo);
      varHints_[lo] = node->typeHint;
      if (!node->typeHint.empty() && node->typeHint[0] == '.')
        varObjectTypes[lo] = toLower(node->typeHint.substr(1));
      if (node->initValue) {
        output << ind() << "var_" << lo << " = ";
        bool prev = inExprCtx; inExprCtx = true;
        emitConverted(node->initValue.get(), node->typeHint); // BUG-53
        inExprCtx = prev;
        output << ";\n";
      }
      return;
    }

    // LOCAL variable declaration
    auto [type, defVal] = declType(node->typeHint, node->vecSize.get());

    // Already declared at the top of the body by hoistLocals() — the
    // declaration here would be jumped over by a Goto/Gosub.
    if (hoistedLocals_.count(lo)) {
      if (node->initValue) {
        output << ind() << "var_" << lo << " = ";
        bool prev = inExprCtx; inExprCtx = true;
        emitConverted(node->initValue.get(), node->typeHint); // BUG-53
        inExprCtx = prev;
        output << ";\n";
      }
      return;
    }

    output << ind() << type << " var_" << lo;
    if (node->initValue) {
      output << " = ";
      bool prev = inExprCtx; inExprCtx = true;
      emitConverted(node->initValue.get(), node->typeHint); // BUG-53
      inExprCtx = prev;
    } else {
      output << " = " << defVal;
    }
    output << ";\n";

    declaredVars.insert(lo);
    varHints_[lo] = node->typeHint;

    // Remember object type for Delete statement code-gen
    if (!node->typeHint.empty() && node->typeHint[0] == '.')
      varObjectTypes[lo] = toLower(node->typeHint.substr(1));
  }

  void visit(AssignStmt *node) override {
    std::string lo = node->name;
    std::transform(lo.begin(), lo.end(), lo.begin(),
                   [](unsigned char c){ return (char)std::tolower(c); });
    if (declaredVars.count(lo) == 0) {
      // Implicit global — Blitz3D allows bare assignment without Local/Global
      auto [type, defVal] = hintToType(node->typeHint);
      output << ind() << type << " var_" << lo << " = ";
      bool prev = inExprCtx; inExprCtx = true;
      emitConverted(node->value.get(), node->typeHint); // BUG-53
      inExprCtx = prev;
      output << ";\n";
      declaredVars.insert(lo);
      varHints_[lo] = node->typeHint;
      // Same bookkeeping as visit(VarDecl): without it a later "Delete p" on a
      // variable that "p.T = New T" introduced would fall into the
      // type-indeterminate path and emit a bare null assignment (BUG-47).
      if (!node->typeHint.empty() && node->typeHint[0] == '.')
        varObjectTypes[lo] = toLower(node->typeHint.substr(1));
    } else {
      // Der Zieltyp steht in der Deklaration, nicht am Tag dieser Zuweisung:
      // "Local s$" und ein spaeteres "s = 42" muessen dasselbe tun (BUG-53).
      std::string target = node->typeHint;
      if (target.empty()) {
        auto ith = varHints_.find(lo);
        if (ith != varHints_.end()) target = ith->second;
      }
      output << ind() << "var_" << lo << " = ";
      bool prev = inExprCtx; inExprCtx = true;
      emitConverted(node->value.get(), target);
      inExprCtx = prev;
      output << ";\n";
    }
  }

  void visit(IfStmt *node) override {
    output << ind() << "if (";
    bool prev = inExprCtx; inExprCtx = true;
    emitIntegerContext(node->condition.get());
    inExprCtx = prev;
    output << ") {\n";

    indentLevel++;
    for (auto &n : node->thenBlock) emitStmt(n.get());
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
      for (auto &n : node->elseBlock) emitStmt(n.get());
      indentLevel--;
    }
    output << ind() << "}\n";
  }

  void visit(WhileStmt *node) override {
    output << ind() << "while (";
    bool prev = inExprCtx; inExprCtx = true;
    emitIntegerContext(node->condition.get());
    inExprCtx = prev;
    output << ") {\n";
    indentLevel++;
    for (auto &n : node->block) emitStmt(n.get());
    indentLevel--;
    output << ind() << "}\n";
  }

  void visit(RepeatStmt *node) override {
    if (node->condition) {
      // Repeat … Until cond  →  do { … } while (!(cond));
      output << ind() << "do {\n";
      indentLevel++;
      for (auto &n : node->block) emitStmt(n.get());
      indentLevel--;
      output << ind() << "} while (!(";
      bool prev = inExprCtx; inExprCtx = true;
      emitIntegerContext(node->condition.get());
      inExprCtx = prev;
      output << "));\n";
    } else {
      // Repeat … Forever  →  while (true) { … }
      output << ind() << "while (true) {\n";
      indentLevel++;
      for (auto &n : node->block) emitStmt(n.get());
      indentLevel--;
      output << ind() << "}\n";
    }
  }

  // The loop variable of a For is an ordinary Blitz3D variable, not a fresh
  // one belonging to the loop (BUG-19). In the reference, ForNode::semant
  // resolves it with "var->semant(e)" - the same call every other variable
  // reference goes through - and ForNode::translate reads and writes it with
  // var->load()/var->store(). Three consequences, all of them observable:
  //
  //   * A Global of the same name IS the loop variable; the loop writes it.
  //   * After the loop the variable keeps the value that failed the test,
  //     so "For i = 1 To 3" leaves i at 4.
  //   * The body may assign to it, and that assignment moves the loop.
  //
  // Emitting "for (auto var_i = ...)" broke all three: it shadowed the global,
  // dropped the final value, and turned "i = i + 1" in the body into a
  // redeclaration that g++ rejected against code the user never wrote.
  void visit(ForStmt *node) override {
    const std::string v = toLower(node->varName);

    // Writes the loop counter: either the plain variable or, for the array
    // and field forms, the access expression itself (BUG-30). ArrayAccess and
    // FieldAccess already emit exactly the lvalue that is needed here.
    auto counter = [&]() {
      if (node->target) emitExpr(node->target.get());
      else              output << "var_" << v;
    };

    // Declare only what does not exist yet, and in the enclosing scope so it
    // outlives the loop - the same rule visit(AssignStmt*) uses for an
    // implicitly created variable. An untagged variable is an int in Blitz3D,
    // which is what hintToType() returns for an empty hint. An array element
    // or a field is never declared here: it already exists.
    if (!node->target && declaredVars.count(v) == 0) {
      auto [type, defVal] = hintToType(node->typeHint);
      output << ind() << type << " var_" << v << " = " << defVal << ";\n";
      declaredVars.insert(v);
      varHints_[v] = node->typeHint;
    }

    // Start, Grenze und Schrittweite nehmen den Typ des Zaehlers an, wie in
    // ForNode::semant (compiler/stmtnode.cpp). Am Original gemessen (BUG-95,
    // BUG-109): "For i = 1 To 1.9" laeuft zweimal, "For i = 0.6 To 3" beginnt
    // bei 1, "Step 3.5" zaehlt 0,4,8, "For i = '1' To 2" laeuft. Eine
    // Zeichenkette wird dabei ueber atoi bzw. atof zur Zahl ("To '10'" laeuft
    // zehnmal, "To '3.7'" dreimal) - der Vergleich mit dem Zaehler als
    // Zeichenkette aus BUG-79 darf hier nie greifen.
    std::string hint = node->target ? lvalueHintOf(node->target.get())
                                    : node->typeHint;
    if (!node->target && hint.empty()) {
      auto ith = varHints_.find(v);
      hint = (ith != varHints_.end()) ? ith->second : std::string("?");
    }
    auto start = [&]() {
      bool prev = inExprCtx; inExprCtx = true;
      emitConverted(node->start.get(), hint);
      inExprCtx = prev;
    };
    // Unbekannter Zaehlertyp: wenigstens zur Zahl machen, wie bisher.
    auto bound = [&](ExprNode *e) {
      if (!convFor(hint)) {
        output << "bb_ToNum(";
        emitExpr(e);
        output << ")";
        return;
      }
      bool prev = inExprCtx; inExprCtx = true;
      emitConverted(e, hint);
      inExprCtx = prev;
    };

    if (node->step) {
      // A STEP may be negative, so the direction of the comparison is decided
      // at run time. The step itself is hoisted: the reference demands a
      // constant one ("Step value must be constant" in ForNode::semant), so
      // evaluating it once changes nothing.
      //
      // The end expression is NOT hoisted. ForNode::translate emits
      // toExpr->translate(g) at the condition label, which every iteration
      // jumps to, so Blitz3D re-reads the bound on each pass - "For i = 1 To n"
      // follows an n that the body changes. It appears twice below because the
      // ternary picks the direction, but only one arm is ever evaluated, so it
      // is read exactly once per iteration. The loop without STEP already
      // behaved this way; hoisting it here had made the two forms disagree.
      output << ind() << "{\n";
      indentLevel++;

      output << ind() << "const auto _step_" << v << " = ";
      bound(node->step.get());
      output << ";\n";

      output << ind() << "for (";
      counter();
      output << " = ";
      start();
      output << "; (_step_" << v << " > 0 ? ";
      counter();
      output << " <= ";
      bound(node->end.get());
      output << " : ";
      counter();
      output << " >= ";
      bound(node->end.get());
      output << "); ";
      counter();
      output << " += _step_" << v << ") {\n";

      indentLevel++;
      for (auto &n : node->block) emitStmt(n.get());
      indentLevel--;
      output << ind() << "}\n";
      indentLevel--;
      output << ind() << "}\n";

    } else {
      // Simple ascending loop without STEP
      output << ind() << "for (";
      counter();
      output << " = ";
      start();
      output << "; ";
      counter();
      output << " <= ";
      bound(node->end.get());
      output << "; ++";
      counter();
      output << ") {\n";
      indentLevel++;
      for (auto &n : node->block) emitStmt(n.get());
      indentLevel--;
      output << ind() << "}\n";
    }
  }

  void visit(SelectStmt *node) override {
    // Erst den passenden Case bestimmen, dann seinen Rumpf ausfuehren - wie
    // SelectNode::translate: der Ausdruck wird einmal ausgewertet, die Cases
    // der Reihe nach verglichen, bis einer passt. Der Zwischenwert lebt nur im
    // inneren Block; danach bleibt nur die Nummer des Case in einem int ohne
    // Initialisierer. Ueber den darf ein Goto oder ein Gosub-Ruecksprung in
    // einen Rumpf hinein springen, ueber "auto _sel_ = ..." verbietet C++ das
    // (BUG-158).
    output << ind() << "{\n";
    indentLevel++;
    output << ind() << "int _selc_;\n";
    output << ind() << "{\n";
    indentLevel++;
    output << ind() << "auto _sel_ = ";
    emitExpr(node->expr.get());
    output << ";\n";

    int num = 0;
    for (auto &c : node->cases) {
      ++num;
      output << ind() << (num == 1 ? "if" : "else if") << " (";
      bool prev = inExprCtx; inExprCtx = true;
      for (size_t i = 0; i < c.expressions.size(); ++i) {
        // Nicht "_sel_ == ...": ein Case wandelt seinen Wert auf den Typ des
        // Select-Ausdrucks (am Original gemessen, siehe bb_CaseEq). Fuer zwei
        // Zahlen ist der Helfer genau der Vergleich, der hier vorher stand.
        output << "bb_CaseEq(_sel_, ";
        c.expressions[i]->accept(this);
        output << ")";
        if (i + 1 < c.expressions.size()) output << " || ";
      }
      inExprCtx = prev;
      output << ") _selc_ = " << num << ";\n";
    }
    output << ind() << (num == 0 ? "" : "else ") << "_selc_ = 0;\n";
    indentLevel--;
    output << ind() << "}\n";

    num = 0;
    for (auto &c : node->cases) {
      ++num;
      output << ind() << (num == 1 ? "if" : "else if") << " (_selc_ == " << num << ") {\n";
      indentLevel++;
      for (auto &n : c.block) emitStmt(n.get());
      indentLevel--;
      output << ind() << "}\n";
    }
    const bool first = (num == 0);

    if (!node->defaultBlock.empty()) {
      // Without a single Case there is no 'if' for an 'else' to attach to,
      // and the emitted C++ did not compile (BUG-18). Blitz3D allows the form:
      // the SELECT branch of parseStmtSeq reads DEFAULT straight after the
      // expression, and SelectNode::translate emits the default body
      // unconditionally after the comparisons - with no comparison to jump
      // away, it simply always runs.
      output << ind() << (first ? "{\n" : "else {\n");
      indentLevel++;
      for (auto &n : node->defaultBlock) emitStmt(n.get());
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

    if (isUser) {
      output << "fn_" << lo << "(";
    } else {
      // Built-ins keep the runtime spelling: bb_Print, never bb_print.
      const char *canon = canonicalCommand(node->name);
      output << "bb_" << (canon ? canon : node->name.c_str()) << "(";
    }

    // Jedes Argument geht an einen Parameter bekannten Typs, also wird es wie
    // eine Zuweisung umgewandelt (BUG-53). Die Zieltypen kommen bei eigenen
    // Funktionen aus der Deklaration, bei Befehlen aus der erzeugten Tabelle -
    // "Text 10,20,zaehler" braucht dort das "s$" des dritten Parameters.
    std::vector<std::string> ptypes = paramHintsOf(lo, isUser);
    bool prev = inExprCtx; inExprCtx = true;
    for (size_t i = 0; i < node->args.size(); ++i) {
      if (i < ptypes.size()) emitConverted(node->args[i].get(), ptypes[i]);
      else                   emitOperand(node->args[i].get());
      if (i + 1 < node->args.size()) output << ", ";
    }
    inExprCtx = prev;

    output << ")";
    if (isStmt) output << ";\n";
  }

  // "int fn_name(int var_a, bbString var_b)" — shared by the forward
  // declaration and the definition so the two can never drift apart.
  //
  // Vorgabewerte (BUG-49) stehen nur in der Vorwaertsdeklaration: C++ erlaubt
  // ein Vorgabeargument genau einmal je Funktion.
  //
  // Und nur fuer den ABSCHLIESSENDEN Lauf von Parametern mit Vorgabe. Das ist
  // keine Einschraenkung gegenueber Blitz3D, sondern dessen gemessene Regel:
  // Pflicht ist dort alles bis zum letzten Parameter ohne Vorgabe, "F(a=1,b)"
  // verlangt also beide Argumente. Eine Vorgabe vor einem Parameter ohne
  // Vorgabe kann somit nie weggelassen werden - sie in C++ auszulassen aendert
  // nichts an der Bedeutung, waehrend sie zu setzen dort ein Fehler waere.
  void emitFunctionSignature(FunctionDecl *node, bool withDefaults = false) {
    auto [rtype, rdefault] = hintToType(node->returnHint);
    size_t firstTrailingDefault = node->params.size();
    while (firstTrailingDefault > 0 &&
           node->params[firstTrailingDefault - 1].defaultValue)
      --firstTrailingDefault;

    output << rtype << " fn_" << toLower(node->name) << "(";
    for (size_t i = 0; i < node->params.size(); ++i) {
      auto &p = node->params[i];
      auto [ptype, defVal] = declType(p.hint, p.vecSize.get());
      output << ptype;
      // Ein festes Array wird als Referenz uebergeben. Am laufenden
      // Original gemessen: eine Funktion, die v[0] beschreibt, aendert
      // das Array des Aufrufers (BUG-59). Die Groesse steckt im Typ,
      // also lehnt schon C++ ein a[3] an einem v[2] ab - dort ist es
      // "Illegal type conversion".
      if (p.vecSize) output << "&";
      output << " var_" << toLower(p.name);
      if (withDefaults && i >= firstTrailingDefault && p.defaultValue) {
        output << " = ";
        emitConverted(p.defaultValue.get(), p.hint); // Zieltyp wie ueberall
      }
      if (i + 1 < node->params.size()) output << ", ";
    }
    output << ")";
  }

  void visit(FunctionDecl *node) override {
    auto [rtype, rdefault] = hintToType(node->returnHint);
    auto savedDefault = returnDefault_;
    auto savedRetHint = returnHint_;
    returnDefault_ = rdefault;
    returnHint_    = node->returnHint; // Zielt fuer "Return <wert>" (BUG-53)
    emitFunctionSignature(node);
    output << " {\n";

    // Save outer declaredVars, start fresh for this function scope.
    // Parameters are pre-registered so that assignments to them inside
    // the body are emitted as plain assignments, not re-declarations.
    auto savedDeclaredVars = declaredVars;
    declaredVars.clear();
    // Preserve global variable names so assignments to globals inside
    // functions are plain assignments, not local re-declarations.
    for (auto &gname : globalVarNames) declaredVars.insert(gname);
    // varObjectTypes is scoped to the body as well: a parameter or local named
    // "p" must not leave its object type behind for the main program to reuse
    // (BUG-56 registers parameters here, hoistLocals() already registered
    // locals, so without this a later "Delete p" outside could pick the wrong
    // type helper).
    auto savedObjectTypes = varObjectTypes;
    auto savedVarHints    = varHints_;
    for (auto &[pname, phint, pdef, pvec] : node->params) {
      (void)pdef; (void)pvec;
      std::string lo = pname;
      std::transform(lo.begin(), lo.end(), lo.begin(),
                     [](unsigned char c){ return (char)std::tolower(c); });
      declaredVars.insert(lo);
      varHints_[lo] = phint;
      // Object parameters must be known by type inside the body, or a field
      // access or Delete on them falls into the type-indeterminate path.
      if (!phint.empty() && phint[0] == '.')
        varObjectTypes[lo] = toLower(phint.substr(1));
    }

    auto savedHoisted = hoistedLocals_;
    hoistedLocals_.clear();

    inFunctionBody = true;
    indentLevel = 1;
    hoistLocals(node->body);
    for (auto &n : node->body) emitStmt(n.get());
    inFunctionBody = false;
    hoistedLocals_ = savedHoisted;

    // Blitz3D functions may just end; C++ may not fall off a non-void
    // function, so close every body with the type's default value.
    output << ind() << "return " << rdefault << ";\n";

    // Restore outer scope's declared vars and object types.
    declaredVars = savedDeclaredVars;
    varObjectTypes = savedObjectTypes;
    varHints_      = savedVarHints;
    returnDefault_ = savedDefault;
    returnHint_    = savedRetHint;

    output << "}\n\n";
    indentLevel = 1; // reset for next function / main
  }

  void visit(ReturnStmt *node) override {
    if (node->value || inFunctionBody) {
      // Normal function return. A bare "Return" in a function returns the
      // default value of its type — with a concrete return type C++ needs one.
      if (!node->value && inFunctionBody) {
        output << ind() << "return " << returnDefault_ << ";\n";
        return;
      }
      output << ind() << "return";
      if (node->value) {
        output << " ";
        emitConverted(node->value.get(), returnHint_); // BUG-53
      }
      output << ";\n";
    } else {
      // Bare Return in main body = jump back to Gosub call site
      output << ind() << "goto __gosub_dispatch__;\n";
    }
  }

  void visit(ConstDecl *node) override {
    // Already emitted at file scope by collectConsts().
    if (hoistedConsts_.count(toLower(node->name))) return;
    bool prev = inExprCtx; inExprCtx = true;
    if (node->typeHint == "$") {
      output << ind() << "const bbString var_" << toLower(node->name) << " = ";
    } else {
      std::string type = "auto";
      if      (node->typeHint == "%") type = "int";
      else if (node->typeHint == "#") type = "float";
      output << ind() << "constexpr " << type << " var_" << toLower(node->name) << " = ";
    }
    node->value->accept(this);
    inExprCtx = prev;
    output << ";\n";
  }

  void visit(DimStmt *node) override {
    auto [elemType, defVal] = hintToType(node->typeHint);
    size_t ndim = node->dims.size();
    std::string lo = node->name;
    std::transform(lo.begin(), lo.end(), lo.begin(), ::tolower);
    noteDimObjectType(lo, node->typeHint); // auch ein Dim in einem Block
    if (hoistedDims_.count(lo)) {
      // Already forward-declared at top of main() — emit re-initialisation
      // (handles both the original Dim and any subsequent re-Dim calls).
      output << ind() << "var_" << lo << " = "
             << buildVecType(elemType, ndim);
      emitVectorCtor(node->dims, elemType, 0);
      output << ";\n";
    } else {
      // Dim inside a block (if/for/while) — no hoisting, emit full declaration.
      output << ind() << buildVecType(elemType, ndim) << " var_" << lo;
      emitVectorCtor(node->dims, elemType, 0);
      output << ";\n";
    }
  }

  void visit(ArrayAccess *node) override {
    output << "var_" << toLower(node->name);
    for (auto &idx : node->indices) {
      output << ".at(";
      emitIntegerContext(idx.get());
      output << ")";
    }
  }

  void visit(ArrayAssignStmt *node) override {
    output << ind() << "var_" << toLower(node->name);
    for (auto &idx : node->indices) {
      output << ".at(";
      emitIntegerContext(idx.get());
      output << ")";
    }
    output << " = ";
    // Wie jede Zuweisung auf den Elementtyp wandeln: "a(0) = 2.5" legt im
    // Original 2 ab (BUG-95). Ohne Dim in Sicht bleibt der Wert, wie er ist.
    auto it = dimHints_.find(toLower(node->name));
    bool prev = inExprCtx; inExprCtx = true;
    if (it != dimHints_.end()) emitConverted(node->value.get(), it->second);
    else                       emitExpr(node->value.get());
    inExprCtx = prev;
    output << ";\n";
  }

  void visit(DataStmt *node) override {
    // All Data values are collected up-front in collectData(); skip at runtime.
    (void)node;
  }

  void visit(ReadStmt *node) override {
    std::string lo = node->name;
    std::transform(lo.begin(), lo.end(), lo.begin(),
               [](unsigned char c){ return (char)std::tolower(c); });

    // Der Zieltyp steht in der Deklaration, nicht am Tag dieses Read - wie bei
    // einer Zuweisung (BUG-53). "Local x# : Read x" liest im Original eine
    // Kommazahl, "Local s$ : Read s" eine Zeichenkette (BUG-85).
    std::string hint = node->typeHint;
    if (hint.empty() && declaredVars.count(lo)) {
      auto ith = varHints_.find(lo);
      if (ith != varHints_.end()) hint = ith->second;
    }
    auto [type, defVal] = hintToType(hint);

    if (declaredVars.count(lo) == 0) {
      // Auto-declare the variable (Blitz3D allows implicit declaration)
      output << ind() << type << " var_" << lo
             << " = (" << type << ")bb_DataRead();\n";
      declaredVars.insert(lo);
    } else {
      output << ind() << "var_" << lo
             << " = (" << type << ")bb_DataRead();\n";
    }
  }

  // Der Wert wandelt sich beim Zuweisen von selbst: bb_DataVal traegt
  // Konvertierungsoperatoren nach int, float und bbString (bb_runtime.h). Wo
  // ein Tag dabeisteht, wird trotzdem ausdruecklich gewandelt - `Read a$`
  // soll auch dann eine Zeichenkette liefern, wenn in der Data-Zeile eine
  // Zahl steht.
  void visit(DataReadExpr *node) override {
    if (node->typeHint.empty()) {
      output << "bb_DataRead()";
    } else {
      auto [type, defVal] = hintToType(node->typeHint);
      (void)defVal;
      output << "(" << type << ")bb_DataRead()";
    }
  }

  void visit(RestoreStmt *node) override {
    if (node->label.empty()) {
      output << ind() << "bb_DataRestore();\n";
    } else {
      // Label-based Restore: reset to the data index recorded at that label.
      output << ind() << "bb_DataRestore(__data_at_" << toLower(node->label)
             << "__);\n";
    }
  }

  void visit(TypeDecl *node) override {
    // Emitted via emitTypeDecl() before main(); skipped in main loop.
    (void)node;
  }

  // New TypeName → bb_TypeName_New()
  void visit(NewExpr *node) override {
    output << "bb_" << toLower(node->typeName) << "_New()";
  }

  // Delete obj → bb_TypeName_Delete(expr); [var = nullptr if simple var]
  void visit(DeleteStmt *node) override {
    // "Delete Each <Typ>" — die ganze Liste leeren. bb_T_Delete haengt den
    // Knoten aus der Liste aus, der Kopf ruckt also nach; die Schleife braucht
    // deshalb keinen eigenen Zeiger auf das naechste Element.
    if (!node->eachTypeName.empty()) {
      const std::string t = toLower(node->eachTypeName);
      output << ind() << "while (bb_" << t << "_head_) bb_" << t
             << "_Delete(bb_" << t << "_head_);\n";
      return;
    }

    // "Delete Null" ist gueltig und tut nichts (DeleteNode::semant laesst
    // Null zu, BUG-45). Bis dahin wurde daraus "0 = nullptr;" und g++ brach ab.
    if (auto *le = dynamic_cast<LiteralExpr *>(node->object.get());
        le && le->isNull) {
      output << ind() << "// Delete Null\n";
      return;
    }

    std::string typeName = getExprTypeName(node->object.get());
    if (!typeName.empty()) {
      output << ind() << "bb_" << toLower(typeName) << "_Delete(";
      emitExpr(node->object.get());
      output << ");\n";
      // Null out the local variable to prevent use-after-free
      if (auto *ve = dynamic_cast<VarExpr *>(node->object.get()))
        output << ind() << "var_" << toLower(ve->name) << " = nullptr;\n";
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
    output << "bb_" << toLower(node->typeName) << "_head_";
  }

  // Last TypeName → bb_TypeName_tail_
  void visit(LastExpr *node) override {
    output << "bb_" << toLower(node->typeName) << "_tail_";
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
    const std::string v = toLower(node->varName);
    const std::string t = toLower(node->typeName);
    // Register the iteration variable in varObjectTypes for nested field access
    varObjectTypes[v] = t;

    if (declaredVars.count(v)) {
      // Der Zaehler ist schon eine Variable des Rumpfs - vorab deklariert
      // (hoistLocals), ein Global oder ein Parameter - und die Schleife
      // schreibt in genau diese (BUG-90). Wie _bbObjEachNext in
      // bbruntime/basic.cpp: nach dem letzten Durchlauf haelt er Null, nach
      // Exit das Objekt, bei dem abgebrochen wurde. Der Nachfolger wird vor
      // dem Rumpf gemerkt, damit "Delete q" darin sicher bleibt.
      output << ind() << "{\n";
      indentLevel++;
      output << ind() << "struct bb_" << t << " *bb_fe_" << v << "_ = nullptr;\n";
      output << ind() << "for (var_" << v << " = bb_" << t << "_head_; var_" << v
             << "; var_" << v << " = bb_fe_" << v << "_) {\n";
      indentLevel++;
      output << ind() << "bb_fe_" << v << "_ = var_" << v << "->__next__;\n";
      for (auto &n : node->block) emitStmt(n.get());
      indentLevel--;
      output << ind() << "}\n";
      indentLevel--;
      output << ind() << "}\n";
      return;
    }

    output << ind() << "{\n";
    indentLevel++;
    output << ind() << "auto *bb_fe_" << v << "_ = bb_"
           << t << "_head_;\n";
    output << ind() << "while (bb_fe_" << v << "_) {\n";
    indentLevel++;
    output << ind() << "auto *var_" << v << " = bb_fe_"
           << v << "_;\n";
    output << ind() << "bb_fe_" << v << "_ = bb_fe_"
           << v << "_->__next__;\n";
    for (auto &n : node->block) emitStmt(n.get());
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
    output << "->var_" << toLower(node->fieldName);
  }

  // obj\field = expr
  // Eine Zuweisung an ein Feld wandelt auf dessen Typ, genau wie die an eine
  // Variable: "glist\\player = Str(...)" schreibt in ein Integer-Feld und ist
  // im Original gueltig (BUG-157). Ohne die Wandlung stand ein bbString in
  // einem int und g++ brach ab.
  void visit(FieldAssignStmt *node) override {
    output << ind();
    bool prev = inExprCtx; inExprCtx = true;
    node->object->accept(this);
    inExprCtx = prev;
    output << "->var_" << toLower(node->fieldName) << " = ";
    // Ein Feld ohne Tag ist ein Integer - genau wie eine Variable ohne Tag.
    // Nur wenn der Typ des Feldes gar nicht bekannt ist (unbekanntes Objekt)
    // oder es ein Objektfeld ist, bleibt der Wert unangetastet.
    const bool known = fieldTypeKnown(node->object.get(), node->fieldName);
    const std::string hint = fieldHintOf(node->object.get(), node->fieldName);
    bool p2 = inExprCtx; inExprCtx = true;
    auto *rd = dynamic_cast<DataReadExpr *>(node->value.get());
    if (!known || (!hint.empty() && hint[0] == '.')) {
      emitExpr(node->value.get());
    } else if (rd && rd->typeHint.empty()) {
      // "Read o\feld": bb_DataVal wandelt sich selbst in den Feldtyp, genau
      // wie bei "Read x" in eine Variable; bb_ToInt(bb_DataVal) waere mehrdeutig.
      output << "(" << hintToType(hint).first << ")";
      emitExpr(node->value.get());
    } else {
      emitConverted(node->value.get(), hint);
    }
    inExprCtx = p2;
    output << ";\n";
  }

  // Der Elementtag eines festen Arrays "a[n]" hinter einer Variablen oder
  // einem Feld, "?" wenn er sich nicht bestimmen laesst.
  std::string vectorElemHintOf(ExprNode *base) {
    if (auto *ve = dynamic_cast<VarExpr *>(base)) {
      auto it = varHints_.find(toLower(ve->name));
      return it != varHints_.end() ? it->second : std::string("?");
    }
    if (auto *fa = dynamic_cast<FieldAccess *>(base))
      if (fieldTypeKnown(fa->object.get(), fa->fieldName))
        return fieldHintOf(fa->object.get(), fa->fieldName);
    return "?";
  }

  // Der Tag eines beschreibbaren Ziels (Array-Element, Feld, Element eines
  // festen Arrays), "?" wenn er sich hier nicht bestimmen laesst.
  std::string lvalueHintOf(ExprNode *e) {
    if (auto *aa = dynamic_cast<ArrayAccess *>(e)) {
      auto it = dimHints_.find(toLower(aa->name));
      return it != dimHints_.end() ? it->second : std::string("?");
    }
    if (auto *fa = dynamic_cast<FieldAccess *>(e))
      return fieldTypeKnown(fa->object.get(), fa->fieldName)
                 ? fieldHintOf(fa->object.get(), fa->fieldName)
                 : std::string("?");
    if (auto *va = dynamic_cast<VectorAccess *>(e))
      return vectorElemHintOf(va->base.get());
    return "?";
  }

  // Kennt der Emitter den Typ dieses Feldes ueberhaupt?
  bool fieldTypeKnown(ExprNode *object, const std::string &field) {
    const std::string tname = objectTypeOf(object);
    if (tname.empty()) return false;
    auto it = typeFieldHints_.find(tname);
    if (it == typeFieldHints_.end()) return false;
    return it->second.count(toLower(field)) != 0;
  }

  // Der Typ eines Feldes, oder "" wenn er sich hier nicht bestimmen laesst.
  std::string fieldHintOf(ExprNode *object, const std::string &field) {
    const std::string tname = objectTypeOf(object);
    if (tname.empty()) return "";
    auto it = typeFieldHints_.find(tname);
    if (it == typeFieldHints_.end()) return "";
    auto f = it->second.find(toLower(field));
    return (f == it->second.end()) ? std::string() : f->second;
  }

  // Der Typname hinter einem Ausdruck: eine Variable mit Objekttyp, ein Feld
  // mit Objekttyp oder ein New/First/Last/Before/After.
  std::string objectTypeOf(ExprNode *e) {
    if (auto *fa = dynamic_cast<FieldAccess *>(e)) {
      const std::string h = fieldHintOf(fa->object.get(), fa->fieldName);
      return (h.size() > 1 && h[0] == '.') ? toLower(h.substr(1)) : std::string();
    }
    if (auto *ne = dynamic_cast<NewExpr *>(e)) return toLower(ne->typeName);
    return toLower(getExprTypeName(e));
  }

  // a[i] - ein Element eines festen Arrays. Die Groesse steckt im C++-Typ
  // (std::array), der Index ist derselbe wie im Quelltext: die Referenz
  // rechnet "basis + index*4" ohne jede Verschiebung, "a[n]" hat also die
  // Indizes 0..n und n+1 Elemente (BUG-59).
  void visit(VectorAccess *node) override {
    bool prev = inExprCtx; inExprCtx = true;
    node->base->accept(this);
    output << "[";
    emitIntegerContext(node->index.get());
    output << "]";
    inExprCtx = prev;
  }

  // a[i] = wert. Der Zielausdruck emittiert sich selbst als lvalue - das
  // gilt fuer eine Variable ebenso wie fuer eine ganze Kette wie
  // "k\\feld", weil visit(FieldAccess*) genau das schon liefert.
  void visit(VectorAssignStmt *node) override {
    output << ind();
    bool prev = inExprCtx; inExprCtx = true;
    node->base->accept(this);
    output << "[";
    emitIntegerContext(node->index.get());
    output << "] = ";
    emitConverted(node->value.get(), vectorElemHintOf(node->base.get()));
    inExprCtx = prev;
    output << ";\n";
  }

  void visit(LabelStmt *node) override {
    // Labels must be followed by a statement in C++; use null statement.
    output << "lbl_" << toLower(node->name) << ":;\n";
  }

  void visit(GotoStmt *node) override {
    output << ind() << "goto lbl_" << toLower(node->label) << ";\n";
  }

  void visit(GosubStmt *node) override {
    int n = ++gosubCount;
    // Portable Gosub: store return-site ID, jump to subroutine.
    // A bare Return emits "goto __gosub_dispatch__" which dispatches back
    // via a switch table emitted at the end of main().
    output << ind() << "__gosub_ret__ = " << n << ";\n";
    output << ind() << "goto lbl_" << toLower(node->label) << ";\n";
    output << ind() << "_gosub_ret_" << n << "_:;\n";
  }

  void visit(ExitStmt *node) override {
    output << ind() << "break;\n";
  }

  void visit(EndStmt *node) override {
    // In einer Funktion kehrte "return 0" nur aus der Funktion zurueck, und
    // das Programm lief weiter - ein stilles Falschergebnis, das der Parser
    // bis BUG-58 verdeckte (dort beendete ein "End" den Funktionsrumpf).
    if (inFunctionBody)
      output << ind() << "bbEnd(); std::exit(0);\n";
    else
      output << ind() << "bbEnd(); return 0;\n";
  }

  void visit(Program *node) override {
    for (auto &n : node->nodes) emitStmt(n.get());
  }

private:
  std::stringstream          output;
  std::unordered_set<std::string> userFunctions;
  std::unordered_set<std::string> typeNames;          // registered Type names
  std::unordered_set<std::string> declaredVars;       // lowercase declared var names
  std::unordered_set<std::string> hoistedLocals_;     // declared up front (Goto-safe)
  std::unordered_set<std::string> vecVars_;           // feste Arrays des laufenden Rumpfes
  std::string returnDefault_ = "0";                  // default value of the current function
  std::string returnHint_;                           // Blitz-Rueckgabetag der laufenden Funktion
  // Skalartag jeder bekannten Variablen ("%", "#", "$" oder ""). Nur fuer die
  // Umwandlung an Zuweisungsgrenzen (BUG-53) noetig; Objekttypen fuehrt
  // varObjectTypes getrennt, weil sie dort einen Typnamen statt eines Tags
  // brauchen.
  std::unordered_map<std::string, std::string> varHints_;
  std::unordered_set<std::string> globalVarNames;     // lowercase names of file-scope globals
  std::unordered_set<std::string> hoistedConsts_;     // lowercase names of file-scope constants
  std::unordered_map<const ExprNode *, std::string> pinned_; // operand -> temp name
  std::unordered_map<std::string, const FunctionDecl *> userFuncDecls_;
  std::unordered_map<std::string, bool> writesState_; // memo per function
  std::unordered_set<std::string>      writesBusy_;   // recursion guard
  int  pinCount_      = 0;                           // numbers the __seqN__ temporaries
  std::unordered_set<std::string> hoistedDims_;       // lowercase names of forward-declared Dim arrays
  // Dim-Array -> Objekttyp seiner Elemente (klein), nur fuer "Dim a.T(n)".
  std::unordered_map<std::string, std::string> dimObjectTypes_;
  // Dim-Array -> Tag seiner Elemente, fuer die Umwandlung beim Zuweisen.
  std::unordered_map<std::string, std::string> dimHints_;
  void noteDimObjectType(const std::string &lo, const std::string &hint) {
    dimHints_[lo] = hint;
    if (!hint.empty() && hint[0] == '.')
      dimObjectTypes_[lo] = toLower(hint.substr(1));
  }
  std::unordered_map<std::string, std::string> varObjectTypes; // lowercase var → TypeName
  // Typname → Feldname → Tag ("%", "#", "$" oder ".Typ"), fuer BUG-157
  std::unordered_map<std::string, std::unordered_map<std::string, std::string>> typeFieldHints_;
  int  indentLevel    = 1;
  bool inExprCtx      = false;
  bool inFunctionBody = false;
  int  gosubCount     = 0;

  // ---- Evaluation order (BUG-29) ------------------------------------------
  //
  // C++ does not say in which order the operands of "+" are evaluated, so
  // "a + F()" with an F that writes a had no meaning fixed by the source: the
  // C++ compiler picked one. Blitz3D does not define an order either - its
  // Tile::eval in codegen_x86/tile.cpp decides by register need - but there
  // the choice is made once, at compile time, by one backend. Ours would
  // differ between g++ and MSVC and between optimisation levels, so the
  // decision was taken to promise more than the reference does and pin the
  // order left to right.
  //
  // How: in an operand list that contains a call to a user function, every
  // operand up to and including the last such call is written out into a
  // numbered temporary first, in source order. What follows stays in place
  // and is therefore evaluated after them. Only *user* functions count: a
  // built-in cannot write a Blitz3D variable, so an expression made of
  // built-ins alone keeps the code it always had.
  //
  // The temporaries and their statement go into a braced block, so a Goto
  // can never jump across one of these initialisations (the reason
  // hoistLocals() exists, BUG-23). Nothing inside a body declares a C++ name
  // that has to outlive its statement - hoistLocals() has already pulled
  // every local to the top - so the braces cost nothing.

  // The operands of a node, in the order Blitz3D reads them.
  std::vector<ExprNode *> operandsOf(ExprNode *e) {
    std::vector<ExprNode *> out;
    if (auto *be = dynamic_cast<BinaryExpr *>(e)) {
      out.push_back(be->left.get()); out.push_back(be->right.get());
    } else if (auto *ue = dynamic_cast<UnaryExpr *>(e)) {
      out.push_back(ue->expr.get());
    } else if (auto *ce = dynamic_cast<CallExpr *>(e)) {
      for (auto &a : ce->args) out.push_back(a.get());
    } else if (auto *aa = dynamic_cast<ArrayAccess *>(e)) {
      for (auto &i : aa->indices) out.push_back(i.get());
    } else if (auto *fa = dynamic_cast<FieldAccess *>(e)) {
      out.push_back(fa->object.get());
    } else if (auto *va = dynamic_cast<VectorAccess *>(e)) {
      out.push_back(va->base.get());
      out.push_back(va->index.get());
    } else if (auto *be2 = dynamic_cast<BeforeExpr *>(e)) {
      out.push_back(be2->object.get());
    } else if (auto *ae = dynamic_cast<AfterExpr *>(e)) {
      out.push_back(ae->object.get());
    }
    return out;
  }

  // Does this user function change anything a sibling operand could see?
  // Writing a global, an array element or a field counts, and so does
  // anything that touches the Data cursor or a type list. A built-in it
  // calls does not: no built-in can write a Blitz3D variable. Everything
  // not understood counts as "yes", and so does a recursive cycle - the
  // safe direction is to write the order out, not to leave it open.
  bool writesUserState(const std::string &lowerName) {
    auto memo = writesState_.find(lowerName);
    if (memo != writesState_.end()) return memo->second;
    auto it = userFuncDecls_.find(lowerName);
    if (it == userFuncDecls_.end()) return true;      // unknown: assume it does
    if (!writesBusy_.insert(lowerName).second) return true; // cycle
    bool w = blockWrites(it->second->body);
    writesBusy_.erase(lowerName);
    writesState_[lowerName] = w;
    return w;
  }

  bool blockWrites(const std::vector<std::unique_ptr<ASTNode>> &nodes) {
    for (auto &n : nodes) if (stmtWrites(n.get())) return true;
    return false;
  }

  bool stmtWrites(ASTNode *n) {
    if (!n) return false;
    if (auto *as = dynamic_cast<AssignStmt *>(n)) {
      // A Local of the same name shadows the global; counting that as a
      // write is the harmless direction.
      if (globalVarNames.count(toLower(as->name))) return true;
      return exprWrites(as->value.get());
    }
    if (dynamic_cast<ArrayAssignStmt *>(n) || dynamic_cast<FieldAssignStmt *>(n) ||
        dynamic_cast<VectorAssignStmt *>(n) ||
        dynamic_cast<DimStmt *>(n)         || dynamic_cast<ReadStmt *>(n) ||
        dynamic_cast<RestoreStmt *>(n)     || dynamic_cast<DeleteStmt *>(n) ||
        dynamic_cast<InsertStmt *>(n))
      return true;
    if (auto *vd = dynamic_cast<VarDecl *>(n)) {
      if (vd->scope == VarDecl::GLOBAL) return true;
      return exprWrites(vd->initValue.get());
    }
    if (auto *ce = dynamic_cast<CallExpr *>(n))    return exprWrites(ce);
    if (auto *rs = dynamic_cast<ReturnStmt *>(n))  return exprWrites(rs->value.get());
    if (auto *is = dynamic_cast<IfStmt *>(n))
      return exprWrites(is->condition.get()) || blockWrites(is->thenBlock) ||
             blockWrites(is->elseBlock);
    if (auto *ws = dynamic_cast<WhileStmt *>(n))
      return exprWrites(ws->condition.get()) || blockWrites(ws->block);
    if (auto *rp = dynamic_cast<RepeatStmt *>(n))
      return exprWrites(rp->condition.get()) || blockWrites(rp->block);
    if (auto *fs = dynamic_cast<ForStmt *>(n))
      return fs->target || exprWrites(fs->start.get()) ||
             exprWrites(fs->end.get()) || exprWrites(fs->step.get()) ||
             blockWrites(fs->block);
    if (auto *fe = dynamic_cast<ForEachStmt *>(n)) return blockWrites(fe->block);
    if (auto *sl = dynamic_cast<SelectStmt *>(n)) {
      if (exprWrites(sl->expr.get())) return true;
      for (auto &c : sl->cases) {
        for (auto &e : c.expressions) if (exprWrites(e.get())) return true;
        if (blockWrites(c.block)) return true;
      }
      return blockWrites(sl->defaultBlock);
    }
    if (auto *pr = dynamic_cast<Program *>(n))     return blockWrites(pr->nodes);
    if (dynamic_cast<LabelStmt *>(n) || dynamic_cast<GotoStmt *>(n) ||
        dynamic_cast<ExitStmt *>(n)  || dynamic_cast<EndStmt *>(n) ||
        dynamic_cast<DataStmt *>(n)  || dynamic_cast<ConstDecl *>(n))
      return false;
    if (auto *e = dynamic_cast<ExprNode *>(n))     return exprWrites(e);
    return true;                                    // not understood
  }

  // Does evaluating this expression change state? A New links a new object
  // into its type list, and a call to a user function may do anything that
  // function does.
  bool exprWrites(ExprNode *e) {
    if (!e) return false;
    if (dynamic_cast<NewExpr *>(e)) return true;
    if (auto *ce = dynamic_cast<CallExpr *>(e))
      if (userFunctions.count(toLower(ce->name)) &&
          writesUserState(toLower(ce->name))) return true;
    for (auto *c : operandsOf(e)) if (exprWrites(c)) return true;
    return false;
  }

  // Any call at all, built-in included: two of them in one expression have
  // an order between themselves even when neither writes a variable.
  bool containsCall(ExprNode *e) {
    if (!e) return false;
    if (dynamic_cast<CallExpr *>(e) || dynamic_cast<NewExpr *>(e)) return true;
    for (auto *c : operandsOf(e)) if (containsCall(c)) return true;
    return false;
  }

  // Can this subtree see anything a user function could have changed? Only
  // then does its position relative to a call matter. A local variable or a
  // parameter cannot: Blitz3D has no way to reach another function's frame,
  // so a call can neither read nor write it. A global, an array element, a
  // field of an object and the type lists can all be reached, and so can
  // anything a second call returns.
  bool readsMutableState(ExprNode *e) {
    if (!e) return false;
    if (auto *ve = dynamic_cast<VarExpr *>(e))
      return globalVarNames.count(toLower(ve->name)) != 0;
    if (dynamic_cast<ArrayAccess *>(e))  return true;
    if (dynamic_cast<FieldAccess *>(e))  return true;
    if (dynamic_cast<VectorAccess *>(e)) return true;
    if (dynamic_cast<FirstExpr *>(e))    return true;
    if (dynamic_cast<LastExpr *>(e))     return true;
    if (dynamic_cast<BeforeExpr *>(e))   return true;
    if (dynamic_cast<AfterExpr *>(e))    return true;
    if (dynamic_cast<CallExpr *>(e))     return true;   // built-ins too
    if (dynamic_cast<NewExpr *>(e))      return true;
    for (auto *c : operandsOf(e)) if (readsMutableState(c)) return true;
    return false;
  }

  // A list of things evaluated one after another - the operands of a node,
  // or the expressions of one statement. Everything up to and including the
  // last one that calls a user function is written out, in this order.
  void pinList(const std::vector<ExprNode *> &ops) {
    for (size_t i = 0; i < ops.size(); ++i) {
      pinExpr(ops[i]);                     // inner order first
      // Nothing follows the last one, so it needs no temporary of its own:
      // it is evaluated after everything written out above it.
      if (i + 1 == ops.size()) continue;
      // Freeze this operand only if some later one could notice the
      // difference: one of the two changes something the other sees, or
      // both call something and the calls need an order between them.
      bool needed = false;
      for (size_t j = i + 1; j < ops.size() && !needed; ++j)
        needed = (exprWrites(ops[i]) && readsMutableState(ops[j])) ||
                 (exprWrites(ops[j]) && readsMutableState(ops[i])) ||
                 (containsCall(ops[i]) && containsCall(ops[j]));
      if (!needed) continue;
      std::string name = "__seq" + std::to_string(++pinCount_) + "__";
      output << ind() << "auto " << name << " = ";
      bool prev = inExprCtx; inExprCtx = true;
      emitOperand(ops[i]);
      inExprCtx = prev;
      output << ";\n";
      pinned_[ops[i]] = name;
    }
  }

  // Walks one expression and writes out the temporaries it needs.
  void pinExpr(ExprNode *e) {
    if (!e) return;
    pinList(operandsOf(e));
  }

  // An operand: the temporary if this one was written out, else the node.
  void emitOperand(ExprNode *e) {
    auto it = pinned_.find(e);
    if (it != pinned_.end()) { output << it->second; return; }
    e->accept(this);
  }

  // The expressions of a statement that are evaluated exactly once, right
  // here. Deliberately missing: the condition of While / Until and the limit
  // and Step of For. Those are read again on every pass (BUG-19), so writing
  // them out in front of the loop would change *when* they run - a worse
  // error than the one this fixes. Inside them the order stays open.
  void pinStmt(ASTNode *n) {
    if (auto *as = dynamic_cast<AssignStmt *>(n)) {
      pinExpr(as->value.get());
    } else if (auto *ce = dynamic_cast<CallExpr *>(n)) {
      pinExpr(ce);
    } else if (auto *rs = dynamic_cast<ReturnStmt *>(n)) {
      pinExpr(rs->value.get());
    } else if (auto *is = dynamic_cast<IfStmt *>(n)) {
      pinExpr(is->condition.get());
    } else if (auto *ss = dynamic_cast<SelectStmt *>(n)) {
      pinExpr(ss->expr.get());
    } else if (auto *vd = dynamic_cast<VarDecl *>(n)) {
      pinExpr(vd->initValue.get());
    } else if (auto *fs = dynamic_cast<ForStmt *>(n)) {
      pinExpr(fs->start.get());           // the start value, once
    } else if (auto *aa = dynamic_cast<ArrayAssignStmt *>(n)) {
      std::vector<ExprNode *> ops;        // indices first, then the value
      for (auto &i : aa->indices) ops.push_back(i.get());
      ops.push_back(aa->value.get());
      pinList(ops);
    } else if (auto *fa = dynamic_cast<FieldAssignStmt *>(n)) {
      pinList({fa->object.get(), fa->value.get()});
    } else if (auto *va = dynamic_cast<VectorAssignStmt *>(n)) {
      pinList({va->base.get(), va->index.get(), va->value.get()});
    } else if (auto *ds = dynamic_cast<DimStmt *>(n)) {
      std::vector<ExprNode *> ops;
      for (auto &d : ds->dims) ops.push_back(d.get());
      pinList(ops);
    }
  }

  // Emit one statement, preceded by the temporaries that fix its order.
  void emitStmt(ASTNode *n) {
    auto savedPins = pinned_;
    pinned_.clear();
    ++indentLevel;
    std::stringstream keep;
    keep.swap(output);                   // collect the declarations apart
    pinStmt(n);
    std::string decls = output.str();
    output.swap(keep);
    --indentLevel;
    if (decls.empty()) {
      n->accept(this);
    } else {
      output << ind() << "{\n";
      ++indentLevel;
      output << decls;
      n->accept(this);
      --indentLevel;
      output << ind() << "}\n";
    }
    pinned_ = savedPins;
  }

  std::string ind() const {
    return std::string(static_cast<size_t>(indentLevel) * 4, ' ');
  }

  // Emit a node as a pure expression (no leading indent, no trailing semicolon)
  void emitExpr(ASTNode *node) {
    bool prev = inExprCtx;
    inExprCtx  = true;
    // An operand that was written out into a temporary for its evaluation
    // order (BUG-29) is named here instead of emitted a second time.
    if (auto *e = dynamic_cast<ExprNode *>(node)) emitOperand(e);
    else node->accept(this);
    inExprCtx = prev;
  }

  // Emit a full C++ struct + intrusive linked-list management for a TypeDecl.
  // Must be called before main() so that variable declarations can use the type.
  void emitTypeDecl(TypeDecl *td) {
    const std::string tname = toLower(td->name); // Types are case-insensitive too
    const std::string sname = "bb_" + tname;

    // Use "struct bb_TypeName *" (elaborated type specifier) throughout so that
    // user-defined types like "Type Rect" don't collide with runtime functions
    // that share the same bb_ prefix (e.g. bb_Rect from bb_graphics2d.h).
    // In C++, a struct tag can always be named unambiguously via "struct X" even
    // when a function named X is in scope.
    const std::string spname = "struct " + sname + " *";  // pointer type

    // Die Tags der Felder merken: eine Zuweisung an ein Feld wandelt wie
    // jede andere (BUG-157).
    for (auto &f : td->fields)
      typeFieldHints_[tname][toLower(f.name)] = f.typeHint;

    // Struct definition
    output << "struct " << sname << " {\n";
    for (auto &f : td->fields) {
      // Ein Feld kann ein festes Array sein ("Field cv#[8]") und seit
      // BUG-60 auch einen Objekttyp tragen ("Field kind.Knoten"). Beides
      // zusammen kommt vor: "Field keylist.KeyFrame[nkeyframes-1]".
      // Als std::array im Struct gehoert es dem Objekt und wird mit ihm
      // angelegt und freigegeben - genau wie im Original, wo bbObjNew
      // ein BBTYPE_VEC-Feld ueber _bbVecAlloc genullt anlegt und
      // bbObjDelete es wieder freigibt.
      auto [ftype, fdefault] = declType(f.typeHint, f.vecSize.get());
      output << "    " << ftype << " var_" << toLower(f.name) << " = " << fdefault << ";\n";
    }
    output << "    " << spname << "__next__ = nullptr;\n";
    output << "    " << spname << "__prev__ = nullptr;\n";
    output << "};\n";

    // Global linked-list head/tail
    output << "inline " << spname << "bb_" << tname << "_head_ = nullptr;\n";
    output << "inline " << spname << "bb_" << tname << "_tail_ = nullptr;\n";

    // bb_TypeName_New() — allocate + append to tail of list
    output << "inline " << spname << "bb_" << tname << "_New() {\n";
    output << "    struct " << sname << " *p = new struct " << sname << ";\n";
    output << "    p->__prev__ = bb_" << tname << "_tail_;\n";
    output << "    p->__next__ = nullptr;\n";
    output << "    if (bb_" << tname << "_tail_) bb_" << tname << "_tail_->__next__ = p;\n";
    output << "    else bb_" << tname << "_head_ = p;\n";
    output << "    bb_" << tname << "_tail_ = p;\n";
    output << "    return p;\n";
    output << "}\n";

    // bb_TypeName_Delete(p) — unlink from list + free
    output << "inline void bb_" << tname << "_Delete(" << spname << "p) {\n";
    output << "    if (!p) return;\n";
    output << "    if (p->__prev__) p->__prev__->__next__ = p->__next__;\n";
    output << "    else bb_" << tname << "_head_ = p->__next__;\n";
    output << "    if (p->__next__) p->__next__->__prev__ = p->__prev__;\n";
    output << "    else bb_" << tname << "_tail_ = p->__prev__;\n";
    output << "    delete p;\n";
    output << "}\n";

    // Helper: unlink p from wherever it currently sits in the list
    // (shared logic used by InsertBefore / InsertAfter)
    output << "inline void bb_" << tname << "_Unlink(" << spname << "p) {\n";
    output << "    if (p->__prev__) p->__prev__->__next__ = p->__next__;\n";
    output << "    else bb_" << tname << "_head_ = p->__next__;\n";
    output << "    if (p->__next__) p->__next__->__prev__ = p->__prev__;\n";
    output << "    else bb_" << tname << "_tail_ = p->__prev__;\n";
    output << "    p->__prev__ = p->__next__ = nullptr;\n";
    output << "}\n";

    // bb_TypeName_InsertBefore(obj, target) — place obj immediately before target
    output << "inline void bb_" << tname << "_InsertBefore(" << spname << "obj, " << spname << "target) {\n";
    output << "    if (!obj || !target || obj == target) return;\n";
    output << "    bb_" << tname << "_Unlink(obj);\n";
    output << "    obj->__next__ = target;\n";
    output << "    obj->__prev__ = target->__prev__;\n";
    output << "    if (target->__prev__) target->__prev__->__next__ = obj;\n";
    output << "    else bb_" << tname << "_head_ = obj;\n";
    output << "    target->__prev__ = obj;\n";
    output << "}\n";

    // bb_TypeName_InsertAfter(obj, target) — place obj immediately after target
    output << "inline void bb_" << tname << "_InsertAfter(" << spname << "obj, " << spname << "target) {\n";
    output << "    if (!obj || !target || obj == target) return;\n";
    output << "    bb_" << tname << "_Unlink(obj);\n";
    output << "    obj->__prev__ = target;\n";
    output << "    obj->__next__ = target->__next__;\n";
    output << "    if (target->__next__) target->__next__->__prev__ = obj;\n";
    output << "    else bb_" << tname << "_tail_ = obj;\n";
    output << "    target->__next__ = obj;\n";
    output << "}\n\n";
  }

  // Helper for ElseIf chains: emit "if (...) { … }" without a leading newline
  void emitElseIf(IfStmt *node) {
    output << "if (";
    bool prev = inExprCtx; inExprCtx = true;
    emitIntegerContext(node->condition.get());
    inExprCtx = prev;
    output << ") {\n";

    indentLevel++;
    for (auto &n : node->thenBlock) emitStmt(n.get());
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
      for (auto &n : node->elseBlock) emitStmt(n.get());
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
    emitIntegerContext(dims[idx].get());
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
        output << "    const size_t __data_at_" << toLower(lbl->name)
               << "__ = " << idx << ";\n";
      } else if (auto *ds = dynamic_cast<DataStmt *>(n.get())) {
        for (auto &tok : ds->values) {
          output << "    bb_data_pool_.push_back(";
          if (tok.type == TokenType::STRING_LIT) {
            output << "bb_DataVal(bbString(\"" << escapeCppString(tok.value)
                   << "\"))";
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
      } else if (dynamic_cast<FunctionDecl *>(n.get())) {
        // **Nicht** in eine Funktion hinein. Data gibt es dort ohnehin nicht
        // ("'Data' can only appear in main program", compiler/parser.cpp:302),
        // und ein `Restore` auf ein Label in einer Funktion lehnt das Original
        // ab, weil Labels funktionslokal sind (BUG-87). Frueher stieg der
        // Sammler hier ab und legte fuer JEDES Label eine globale Konstante
        // `__data_at_x__` an - zwei gleichnamige Labels in verschiedenen
        // Bereichen, im Original voellig zulaessig, ergaben damit erzeugtes
        // C++ mit einer doppelten Deklaration.
        continue;
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

  // Forward-declare all top-level Dim'd arrays as default-constructed (empty)
  // vectors at the beginning of main(), so that forward references (array used
  // before its Dim in the token stream) compile correctly in C++.
  // Recurses into Program sub-nodes (from preprocessor include inlining).
  // The actual size initialisation is emitted at the original Dim position by
  // visit(DimStmt*) as an assignment instead of a declaration.
  void collectDims(const std::vector<std::unique_ptr<ASTNode>> &nodes) {
    for (auto &n : nodes) {
      if (auto *prog = dynamic_cast<Program *>(n.get())) {
        collectDims(prog->nodes);
      } else if (auto *ds = dynamic_cast<DimStmt *>(n.get())) {
        std::string lo = ds->name;
        std::transform(lo.begin(), lo.end(), lo.begin(),
               [](unsigned char c){ return (char)std::tolower(c); });
        if (hoistedDims_.count(lo)) continue; // already forward-declared
        hoistedDims_.insert(lo);
        noteDimObjectType(lo, ds->typeHint);
        auto [elemType, defVal] = hintToType(ds->typeHint);
        output << ind() << buildVecType(elemType, ds->dims.size())
               << " var_" << lo << ";\n";
      }
    }
  }

  // ---- Body-wide locals ----------------------------------------------------
  //
  // Declares every variable a body owns at the top of that body, with its
  // default value; the declaration at the original position becomes a plain
  // assignment (see visit(VarDecl*) and visit(AssignStmt*)).
  //
  // Blitz3D has no block scope. IfNode::semant, WhileNode::semant and
  // ForNode::semant all hand the *same* Environ down to the statements they
  // contain, and IdentVarNode::semant puts an implicitly created variable into
  // "e->decls" - the decl list of the enclosing function or of the program.
  // A variable first written inside an If therefore belongs to the whole body
  // (BUG-16). Emitting its declaration where it is first written gave it C++
  // block scope instead, so it vanished at the closing brace.
  //
  // The same pass also keeps a Goto from jumping over an initialisation, which
  // is what it was originally written for (BUG-23) - that is now a side effect
  // rather than the trigger.
  //
  // A Local without an initialiser emits no code at its original position:
  // VarDeclNode::translate in the reference is "if( expr ) g->code( ... )",
  // so a bare "Local x" inside a loop does not re-zero x on every pass.
  void hoistLocals(const std::vector<std::unique_ptr<ASTNode>> &nodes) {
    std::vector<LocalDecl> found;
    collectLocals(nodes, found);
    for (auto &[lo, hint, vecSize] : found) {
      if (declaredVars.count(lo)) continue; // global, parameter or seen already
      // A Const and a Dim'd array live at file scope, and visit(FunctionDecl*)
      // reseeds declaredVars from globalVarNames alone - inside a function
      // body neither is in it. Without these two guards a mere *read* of a
      // constant in a function would hoist a local of the same name in front
      // of it and silently shadow the value (BUG-72).
      if (hoistedConsts_.count(lo) || hoistedDims_.count(lo)) continue;
      auto [type, defVal] = declType(hint, vecSize);
      output << ind() << type << " var_" << lo << " = " << defVal << ";\n";
      declaredVars.insert(lo);
      hoistedLocals_.insert(lo);
      if (vecSize) vecVars_.insert(lo);
      varHints_[lo] = hint;
      if (!hint.empty() && hint[0] == '.')
        varObjectTypes[lo] = toLower(hint.substr(1));
    }
  }

  // Every name this body brings into being, in source order. In Blitz3D that
  // is not only Local, a first assignment and Read's auto-declaration, but
  // every mere *mention*: IdentVarNode::semant looks the name up with
  // findDecl() and, finding nothing, runs its "ugly auto decl!" —
  // insertDecl( ident,t,DECL_LOCAL ) into the decl list of the enclosing
  // body. Reading and writing go through exactly the same place (BUG-72).
  //
  // Measured against the running original (2026-09-10): a function does not
  // see a name the main program created that way — it wrote "func zz=0" while
  // main still read 42 — so the scope is the body, which is what this pass
  // already models. A function name used without parentheses is not a call
  // but an implicit variable there ("g=0", not 7), because findDecl searches
  // decls while functions live in funcDecls; names are therefore deliberately
  // NOT filtered against userFunctions here.
  //
  // The order of the walk follows the reference's semant order, because the
  // first mention decides the type: AssNode does var before expr, IfNode and
  // WhileNode the condition before the body, RepeatNode the body before the
  // Until condition, ForNode the counter before from/to/step.
  //
  // Does not descend into FunctionDecl — those bodies hoist their own.
  void collectLocals(const std::vector<std::unique_ptr<ASTNode>> &nodes,
                     std::vector<LocalDecl> &out) {
    for (auto &n : nodes) {
      if (auto *vd = dynamic_cast<VarDecl *>(n.get())) {
        // Name before initialiser, unlike VarDeclNode::proto, which semants
        // the expression first. That order only differs for "Local s$ = s",
        // which the reference rejects as a duplicate variable name anyway;
        // taking the name first keeps the hoisted type the tagged one instead
        // of an int that visit(VarDecl*) would then redeclare as a string.
        if (vd->scope == VarDecl::LOCAL)
          addLocal(out, vd->name, vd->typeHint, vd->vecSize.get());
        collectExprLocals(vd->initValue.get(), out);
      } else if (auto *as = dynamic_cast<AssignStmt *>(n.get())) {
        addLocal(out, as->name, as->typeHint);
        collectExprLocals(as->value.get(), out);
      } else if (auto *rd = dynamic_cast<ReadStmt *>(n.get())) {
        addLocal(out, rd->name, rd->typeHint);
      } else if (auto *prog = dynamic_cast<Program *>(n.get())) {
        collectLocals(prog->nodes, out);
      } else if (auto *if_ = dynamic_cast<IfStmt *>(n.get())) {
        collectExprLocals(if_->condition.get(), out);
        collectLocals(if_->thenBlock, out);
        collectLocals(if_->elseBlock, out);
      } else if (auto *wh = dynamic_cast<WhileStmt *>(n.get())) {
        collectExprLocals(wh->condition.get(), out);
        collectLocals(wh->block, out);
      } else if (auto *rp = dynamic_cast<RepeatStmt *>(n.get())) {
        collectLocals(rp->block, out);
        collectExprLocals(rp->condition.get(), out);
      } else if (auto *fr = dynamic_cast<ForStmt *>(n.get())) {
        // The loop variable counts too (BUG-19): since it is declared in front
        // of the loop rather than inside the C++ for-init, a Goto past the
        // whole loop would otherwise cross its initialisation, which is what
        // BUG-23 was about. An array element or a field counter is not a local
        // and declares nothing (BUG-30) — but what stands inside it does.
        if (!fr->target) addLocal(out, fr->varName, fr->typeHint);
        else             collectExprLocals(fr->target.get(), out);
        collectExprLocals(fr->start.get(), out);
        collectExprLocals(fr->end.get(), out);
        collectExprLocals(fr->step.get(), out);
        collectLocals(fr->block, out);
      } else if (auto *sel = dynamic_cast<SelectStmt *>(n.get())) {
        collectExprLocals(sel->expr.get(), out);
        for (auto &c : sel->cases) {
          for (auto &ce : c.expressions) collectExprLocals(ce.get(), out);
          collectLocals(c.block, out);
        }
        collectLocals(sel->defaultBlock, out);
      } else if (auto *fe = dynamic_cast<ForEachStmt *>(n.get())) {
        // Der Zaehler ist eine gewoehnliche Variable des Rumpfs, mit dem
        // Objekttyp der Schleife (ForEachNode::semant verlangt genau den).
        // Bis BUG-90 wurde er nicht vorab deklariert, sondern in der
        // C++-Schleife - eine spaetere Zuweisung ohne Tag legte dann ein int
        // desselben Namens an, und g++ scheiterte; ein Global oder Parameter
        // wurde von der inneren Deklaration verdeckt.
        addLocal(out, fe->varName, "." + fe->typeName);
        collectLocals(fe->block, out);
      } else if (auto *ds = dynamic_cast<DimStmt *>(n.get())) {
        for (auto &d : ds->dims) collectExprLocals(d.get(), out);
      } else if (auto *aas = dynamic_cast<ArrayAssignStmt *>(n.get())) {
        for (auto &i : aas->indices) collectExprLocals(i.get(), out);
        collectExprLocals(aas->value.get(), out);
      } else if (auto *fas = dynamic_cast<FieldAssignStmt *>(n.get())) {
        collectExprLocals(fas->object.get(), out);
        collectExprLocals(fas->value.get(), out);
      } else if (auto *vas = dynamic_cast<VectorAssignStmt *>(n.get())) {
        collectExprLocals(vas->base.get(), out);
        collectExprLocals(vas->index.get(), out);
        collectExprLocals(vas->value.get(), out);
      } else if (auto *rs = dynamic_cast<ReturnStmt *>(n.get())) {
        collectExprLocals(rs->value.get(), out);
      } else if (auto *del = dynamic_cast<DeleteStmt *>(n.get())) {
        collectExprLocals(del->object.get(), out);
      } else if (auto *ins = dynamic_cast<InsertStmt *>(n.get())) {
        collectExprLocals(ins->object.get(), out);
        collectExprLocals(ins->target.get(), out);
      } else if (auto *ex = dynamic_cast<ExprNode *>(n.get())) {
        // A command statement ("Print x", "MoveEntity e,0,0,1") reaches the
        // emitter as a bare expression node, not as a statement node.
        collectExprLocals(ex, out);
      }
      // A ConstDecl is not walked: the reference demands a constant
      // expression there, so it can mention no variable.
    }
  }

  // First mention wins, tag included — the reference types the variable where
  // its decl is inserted and rejects a later contradicting tag with
  // "Variable type mismatch".
  static void addLocal(std::vector<LocalDecl> &out,
                       const std::string &name, const std::string &hint,
                       ExprNode *vecSize = nullptr) {
    std::string lo = toLower(name);
    for (auto &e : out)
      if (e.name == lo) return;
    out.push_back(LocalDecl{lo, hint, vecSize});
  }

  // Every name an expression mentions, in source order. A literal, New, First
  // and Last mention none; an array or field name is not a variable of its
  // own, but the index expressions and the object it hangs off are.
  void collectExprLocals(const ExprNode *e,
                         std::vector<LocalDecl> &out) {
    if (!e) return;
    if (auto *ve = dynamic_cast<const VarExpr *>(e)) {
      addLocal(out, ve->name, ve->typeHint);
    } else if (auto *be = dynamic_cast<const BinaryExpr *>(e)) {
      collectExprLocals(be->left.get(), out);
      collectExprLocals(be->right.get(), out);
    } else if (auto *ue = dynamic_cast<const UnaryExpr *>(e)) {
      collectExprLocals(ue->expr.get(), out);
    } else if (auto *ce = dynamic_cast<const CallExpr *>(e)) {
      for (auto &a : ce->args) collectExprLocals(a.get(), out);
    } else if (auto *aa = dynamic_cast<const ArrayAccess *>(e)) {
      for (auto &i : aa->indices) collectExprLocals(i.get(), out);
    } else if (auto *fa = dynamic_cast<const FieldAccess *>(e)) {
      collectExprLocals(fa->object.get(), out);
    } else if (auto *va = dynamic_cast<const VectorAccess *>(e)) {
      collectExprLocals(va->base.get(), out);
      collectExprLocals(va->index.get(), out);
    } else if (auto *bx = dynamic_cast<const BeforeExpr *>(e)) {
      collectExprLocals(bx->object.get(), out);
    } else if (auto *ax = dynamic_cast<const AfterExpr *>(e)) {
      collectExprLocals(ax->object.get(), out);
    }
  }

  // Emit every top-level Const at file scope, in source order so that one
  // constant may build on an earlier one. Function bodies are not searched:
  // a Const inside a function is rejected by the semantic pass, and if one
  // ever reached the emitter, visit(ConstDecl*) still emits it in place.
  void collectConsts(const std::vector<std::unique_ptr<ASTNode>> &nodes) {
    for (auto &n : nodes) {
      if (auto *prog = dynamic_cast<Program *>(n.get())) {
        collectConsts(prog->nodes);
      } else if (auto *cd = dynamic_cast<ConstDecl *>(n.get())) {
        std::string lo = toLower(cd->name);
        if (hoistedConsts_.count(lo)) continue; // skip duplicates
        hoistedConsts_.insert(lo);
        declaredVars.insert(lo); // never re-declared as an implicit variable
        if (cd->typeHint == "$") {
          output << "const bbString var_" << lo << " = ";
        } else {
          std::string type = "auto";
          if      (cd->typeHint == "%") type = "int";
          else if (cd->typeHint == "#") type = "float";
          output << "constexpr " << type << " var_" << lo << " = ";
        }
        bool prev = inExprCtx; inExprCtx = true;
        cd->value->accept(this);
        inExprCtx = prev;
        output << ";\n";
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
        varHints_[lo] = vd->typeHint;
        // Ein Global mit Objekttyp ("Global glist.tGList") muss auch als
        // solches bekannt sein - sonst findet eine Zuweisung an eines seiner
        // Felder den Feldtyp nicht (BUG-157).
        if (!vd->typeHint.empty() && vd->typeHint[0] == '.')
          varObjectTypes[lo] = toLower(vd->typeHint.substr(1));
        auto [type, defVal] = declType(vd->typeHint, vd->vecSize.get());
        output << type << " var_" << lo << " = " << defVal << ";\n";
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
    // Ein Element eines "Dim feld.Punkt(n)" (BUG-52). Ohne diesen Zweig wurde
    // "Delete feld(0)" zu "feld.at(0) = nullptr" - das Objekt blieb in der
    // Liste, ein stilles Falschergebnis.
    if (auto *aa = dynamic_cast<ArrayAccess *>(expr)) {
      auto it = dimObjectTypes_.find(toLower(aa->name));
      if (it != dimObjectTypes_.end()) return it->second;
    }
    if (auto *fe = dynamic_cast<FirstExpr *>(expr))  return fe->typeName;
    if (auto *le = dynamic_cast<LastExpr *>(expr))   return le->typeName;
    // Before(p) / After(p) have the same type as p — recurse
    if (auto *be = dynamic_cast<BeforeExpr *>(expr)) return getExprTypeName(be->object.get());
    if (auto *ae = dynamic_cast<AfterExpr *>(expr))  return getExprTypeName(ae->object.get());
    return "";
  }

  // Die Parametertags einer Funktion oder eines Befehls, in Reihenfolge. Leer,
  // wo nichts bekannt ist - dann bleibt das Argument unangetastet. Ein Eintrag
  // ohne Typ in der Befehlstabelle (z.B. "val?" bei Print) bedeutet ausdruecklich
  // "beliebig" und darf ebenfalls nicht gewandelt werden.
  std::vector<std::string> paramHintsOf(const std::string &lo, bool isUser) {
    std::vector<std::string> out;
    if (isUser) {
      auto it = userFuncDecls_.find(lo);
      if (it == userFuncDecls_.end() || !it->second) return out;
      // Ein festes Array wird als Referenz uebergeben und darf nicht
      // durch bb_ToInt und Verwandte laufen - der Marker haelt es aus
      // der Umwandlung heraus (BUG-59).
      for (auto &p : it->second->params)
        out.push_back(p.vecSize ? std::string(kVecHint) : p.hint);
      return out;
    }
    for (const auto &c : kCommands) {
      if (toLower(c.name) != lo) continue;
      std::string spec = c.params;
      size_t pos = 0;
      while (pos <= spec.size()) {
        size_t comma = spec.find(',', pos);
        std::string one = spec.substr(pos, comma == std::string::npos
                                              ? std::string::npos
                                              : comma - pos);
        if (!one.empty() && one.back() == '?') one.pop_back();
        std::string tag;
        if (!one.empty() && (one.back() == '%' || one.back() == '#' ||
                             one.back() == '$'))
          tag = std::string(1, one.back());
        // Kein Tag heisst "beliebig" - als Sonderfall markieren, damit
        // convFor() nicht faelschlich auf Integer zurueckfaellt.
        out.push_back(tag.empty() ? std::string("?") : tag);
        if (comma == std::string::npos) break;
        pos = comma + 1;
      }
      break;
    }
    return out;
  }

  // Der Umwandlungshelfer fuer einen Zieltyp, oder nullptr wenn keiner noetig
  // ist. Objektziele wandeln nie (BUG-53, am Original gemessen).
  static const char *convFor(const std::string &hint) {
    if (hint == "?") return nullptr;                    // beliebiger Parameter
    if (hint == kVecHint) return nullptr;               // festes Array
    if (!hint.empty() && hint[0] == '.') return nullptr; // Objektziel
    if (hint == "$") return "bb_Str";
    if (hint == "#") return "bb_ToFloat";
    return "bb_ToInt"; // "%" und ohne Tag - in Blitz3D beides Integer
  }

  // Ein Wert, der an ein Ziel bekannten Typs geht. Der Emitter kennt den
  // Zieltyp, nicht den Typ des Ausdrucks - deshalb wird immer gewrappt und die
  // C++-Ueberladungsaufloesung entscheidet, ob ueberhaupt etwas passiert (siehe
  // bb_string.h). Ein Literal, das ohnehin schon passt, bleibt unangetastet,
  // damit das Emittat lesbar bleibt.
  bool literalAlreadyFits(ExprNode *e, const std::string &hint) {
    auto *le = dynamic_cast<LiteralExpr *>(e);
    if (!le) return false;
    switch (le->token.type) {
      case TokenType::STRING_LIT: return hint == "$";
      case TokenType::FLOAT_LIT:  return hint == "#";
      default:                    return hint != "$" && hint != "#";
    }
  }

  // BUG-61: stmtnode.cpp casts conditions/Dim bounds to int;
  // varnode.cpp does the same for both array forms. Preserve emitExpr's
  // pinned operands and loop re-evaluation; evaluate the operand once.
  void emitIntegerContext(ExprNode *e) {
    output << "bb_IntegerContext(";
    emitExpr(e);
    output << ")";
  }

  void emitConverted(ExprNode *e, const std::string &hint) {
    // Ueber emitExpr(), nicht emitOperand(): der gewrappte Wert ist immer ein
    // Ausdruck. Ohne gesetztes inExprCtx haelt sich ein Aufruf darin fuer eine
    // Anweisung und schreibt Einrueckung und Semikolon mitten in die Klammer -
    // "Return Len(s)" wurde so zu "bb_ToInt(  bb_Len(...);\n)".
    const char *fn = convFor(hint);
    if (!fn || literalAlreadyFits(e, hint)) { emitExpr(e); return; }
    // "Read arr(i)", "Read v[i]": bb_DataVal wandelt sich selbst in den
    // Zieltyp - und schneidet dabei ab wie das Original, statt zu runden
    // (BUG-95). bb_ToInt(bb_DataVal) waere ausserdem mehrdeutig.
    if (dynamic_cast<DataReadExpr *>(e)) {
      output << "(" << hintToType(hint).first << ")";
      emitExpr(e);
      return;
    }
    output << fn << "(";
    emitExpr(e);
    output << ")";
  }

  // Returns {cppType, defaultValue} for a Blitz3D type hint.
  // "$" → {"bbString", "\"\""}, "#" → {"float", "0.0f"},
  // ".Vec" → {"bb_Vec *", "nullptr"}, "%"/empty → {"int", "0"}
  static std::pair<std::string, std::string>
  hintToType(const std::string &hint) {
    if (hint == "$")
      return {"bbString", "\"\""};
    if (hint == "#")
      return {"float", "0.0f"};
    if (!hint.empty() && hint[0] == '.')
      return {"struct bb_" + toLower(hint.substr(1)) + " *", "nullptr"};
    return {"int", "0"}; // "%" or empty → int
  }

  // Rendert einen Ausdruck in eine Zeichenkette, statt ihn in den Strom zu
  // schreiben. Gebraucht wird das dort, wo der Wert Teil eines Typnamens
  // wird - die Groesse eines festen Arrays steht in C++ im Typ.
  std::string exprToString(ExprNode *e) {
    std::stringstream tmp;
    std::swap(output, tmp);
    bool prev = inExprCtx; inExprCtx = true;
    e->accept(this);
    inExprCtx = prev;
    std::swap(output, tmp);
    return tmp.str();
  }

  // Typ und Anfangswert einer Deklaration, feste Arrays eingeschlossen.
  //
  // "Local a[3]" wird zu std::array<int,(3)+1> - das "+1", weil "a[n]" im
  // Original die Indizes 0..n hat: VectorDeclNode::proto legt
  // "sizes.push_back( n+1 )" ab. Die Groesse bleibt der Ausdruck aus dem
  // Quelltext; ein Const ist dort zulaessig und steht bei uns als constexpr
  // auf Dateiebene, taugt also als Feldgroesse. Dass sie konstant sein muss,
  // prueft die semantische Phase - hier faellt es sonst als C++-Meldung an.
  //
  // "{}" als Anfangswert nullt jedes Element, wie _bbVecAlloc es mit seinem
  // memset tut; fuer bbString ist es der Standardkonstruktor, also "".
  std::pair<std::string, std::string> declType(const std::string &hint,
                                               ExprNode *vecSize) {
    auto [type, defVal] = hintToType(hint);
    if (!vecSize) return {type, defVal};
    return {"std::array<" + type + ", (" + exprToString(vecSize) + ") + 1>",
            "{}"};
  }

  // Map Blitz3D operators to their C++ equivalents
  static std::string mapOp(const std::string &op) {
    if (op == "=")   return "==";
    if (op == "<>")  return "!=";
    if (op == "AND") return "&";
    if (op == "OR")  return "|";
    if (op == "XOR") return "^";
    if (op == "SHL") return "<<";
    // MOD and SHR are handled as special cases in visit(BinaryExpr*) — not
    // reached here
    if (op == "SAR") return ">>";
    return op; // +  -  *  /  <  >  <=  >=
  }
};

#endif // BLITZNEXT_EMITTER_H
