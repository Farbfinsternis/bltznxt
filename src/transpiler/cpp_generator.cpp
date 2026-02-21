#include "cpp_generator.h"
#include "decl.h"
#include "environ.h"
#include "nodes.h"
#include "varnode.h"

CppGenerator::CppGenerator(std::ostream &out)
    : Codegen(out), next_gosub_id(1), in_function(false) {
  indent = "";
}

// AST Dispatcher
// AST Dispatcher
void CppGenerator::generate(Node *node) {
  if (node) {

    node->accept(this);
  } else {
  }
}

// Visitors
void CppGenerator::emitVarDecls(BCEnviron *env, int kind) {
  if (!env || !env->decls)
    return;
  for (Decl *d : env->decls->decls) {
    if (d->kind == kind) {
      emitVarDecl(d);
    }
  }
}

void CppGenerator::emitVarDecl(Decl *d) {
  std::string type = mapType(d->type);
  std::string init = " = 0;";
  if (d->type == Type::float_type) {
    init = " = 0.0f;";
  } else if (d->type == Type::string_type) {
    init = " = \"\";";
  } else if (d->type->structType() || d->type == Type::null_type) {
    init = " = nullptr;";
  }
  emitln(type + " " + d->name + init);
}

void CppGenerator::emitReturnLogic() {
  emitln("");
  emitln("_bb_return_logic:");
  emitln("    if (!_bb_gosub_stack.empty()) {");
  emitln("        int _bb_id = _bb_gosub_stack.back();");
  emitln("        _bb_gosub_stack.pop_back();");
  emitln("        switch (_bb_id) {");
  for (int id : current_gosub_ids) {
    out << "            case " << id << ": goto _bb_ret_" << id << ";\n";
  }
  emitln("            default: ;");
  emitln("        }");
  emitln("    }");
}

void CppGenerator::visit(ProgNode *node) {
  current_gosub_ids.clear();
  in_function = false;

  emitln("#include \"api.h\""); // runtime API
  emitln("");
  emitln("// BltzNext Output");
  emitln("");

  // Global Declarations
  emitVarDecls(node->sem_env, DECL_GLOBAL);

  // Generate Structs
  if (node->structs) {
    for (auto *d : node->structs->decls) {
      generate(d);
    }
  }

  // Generate Data Init Function
  if (node->datas) {
    emitln("void _bb_init_data() {");
    incIndent();
    for (auto *d : node->datas->decls) {
      generate(d);
    }
    decIndent();
    emitln("}");
    emitln("");
  }

  // Generate Functions (excluding main)
  if (node->funcs) {
    for (int i = 0; i < (int)node->funcs->decls.size(); ++i) {
      if (auto *f = dynamic_cast<FuncDeclNode *>(node->funcs->decls[i])) {
        visit(f);
      }
    }
  }

  out << "\nint main() {\n";
  out << "    bbRuntimeInit();\n";
  if (node->datas) {
    out << "    _bb_init_data();\n";
  }
  out << "    std::vector<int> _bb_gosub_stack;\n";

  // Local Declarations for main
  incIndent();
  emitVarDecls(node->sem_env, DECL_LOCAL);
  if (!node->sem_env) {
    out << "    // No semantic environment found!\n";
  }
  out << "\n";

  out << "\n";
  out << "    \n";

  out << "    \n";

  // The original code used `incIndent()` and `decIndent()` around the main
  // body. The new code uses hardcoded 4 spaces for bbRuntimeInit and variable
  // declarations. To maintain consistency for the main statement block, we'll
  // set indent to 4 spaces.
  indent = "    ";

  if (node->stmts)
    generate(node->stmts);

  emitReturnLogic();
  emitln("    return 0;");
  indent = ""; // Reset indent for the closing brace
  emitln("}");
}

void CppGenerator::visit(StmtSeqNode *node) {
  for (int i = 0; i < node->stmts.size(); ++i) {
    generate(node->stmts[i]);
  }
}

void CppGenerator::visit(StmtNode *node) { emitln("// Stmt"); }

void CppGenerator::visit(DeclStmtNode *node) {
  if (auto *v = dynamic_cast<VarDeclNode *>(node->decl)) {
    visit(v);
  }
}

void CppGenerator::visit(VarDeclNode *node) {
  if (node->expr) {
    emit(indent);
    out << node->sem_var->sem_decl->name << " = ";
    generate(node->expr);
    out << ";\n";
  }
}

void CppGenerator::visit(ExprStmtNode *node) {
  emit(indent);
  generate(node->expr); // visit expr
  out << ";\n";
}

void CppGenerator::visit(CallNode *node) {
  out << node->sem_decl->name << "(";
  if (node->exprs) {
    for (int i = 0; i < node->exprs->exprs.size(); ++i) {
      if (i > 0)
        out << ", ";
      generate(node->exprs->exprs[i]);
    }
  }
  out << ")";
}

void CppGenerator::visit(ExprNode *node) { out << "/* Expr */"; }

void CppGenerator::visit(StringConstNode *node) {
  out << "std::string(\"" << node->value << "\")";
}

void CppGenerator::visit(IntConstNode *node) { out << node->value << "LL"; }

void CppGenerator::visit(FloatConstNode *node) {
  // Ensure float literals always have a decimal point (e.g. "0.0f" not "0f")
  float v = node->value;
  if (v == (int)v && v >= -1e6f && v <= 1e6f) {
    out << (int)v << ".0f";
  } else {
    out << v << "f";
  }
}

void CppGenerator::visit(VarExprNode *node) {
  if (node->var)
    generate(node->var);
}

void CppGenerator::visit(VarNode *node) {
  // Abstract
}

void CppGenerator::visit(IdentVarNode *node) {
  if (node->sem_decl)
    out << node->sem_decl->name;
  else
    out << node->ident;
}

void CppGenerator::visit(AssNode *node) {
  emit(indent);
  if (node->var)
    generate(node->var);
  out << " = ";
  if (node->expr)
    generate(node->expr);
  out << ";\n";
}

void CppGenerator::visit(ForNode *node) {
  // Basic FOR loop generation
  // For i = from To to Step step
  // Transpiles to: for (int i = from; i <= to; i += step)
  // Note: declaring 'int' here is a hack to avoid pre-declaring vars

  emit(indent);
  out << "for (";
  if (node->var)
    generate(node->var);
  out << " = ";
  if (node->fromExpr)
    generate(node->fromExpr);
  out << "; ";

  // Condition
  if (node->var)
    generate(node->var);
  out << " <= "; // Assuming positive step for now
  if (node->toExpr)
    generate(node->toExpr);
  out << "; ";

  // Step
  if (node->var)
    generate(node->var);
  out << " += ";
  if (node->stepExpr)
    generate(node->stepExpr);
  else
    out << "1"; // Default step

  out << ") {\n";

  incIndent();
  if (node->stmts)
    generate(node->stmts);
  decIndent();

  emitln("}");
}

void CppGenerator::visit(BinExprNode *node) {
  if (node->op == SHR) {
    // Logical Shift Right (unsigned 32-bit)
    out << "((bb_int)((uint32_t)";
    if (node->lhs)
      generate(node->lhs);
    out << " >> ";
    if (node->rhs)
      generate(node->rhs);
    out << "))";
    return;
  }

  out << "(";
  if (node->lhs)
    generate(node->lhs);

  switch (node->op) {
  case AND:
    out << " & ";
    break;
  case OR:
    out << " | ";
    break;
  case XOR:
    out << " ^ ";
    break;
  case SHL:
    out << " << ";
    break;
  case SAR:
    out << " >> "; // Arithmetic shift
    break;
  default:
    out << " op" << node->op << " ";
  }

  if (node->rhs)
    generate(node->rhs);
  out << ")";
}

void CppGenerator::visit(UniExprNode *node) {
  if (node->op == '-') {
    out << "-";
  } else if (node->op == '+') {
    out << "+";
  } else if (node->op == NOT) {
    out << "!";
  } else if (node->op == ABS) {
    out << "bbAbs(";
  } else if (node->op == SGN) {
    out << "bbSgn(";
  }

  if (node->expr)
    generate(node->expr);

  if (node->op == ABS || node->op == SGN)
    out << ")";
}

// Rewritten to use bbAdd etc.
void CppGenerator::visit(ArithExprNode *node) {
  switch (node->op) {
  case '+':
    out << "bbAdd(";
    break;
  case '-':
    out << "bbSub(";
    break;
  case '*':
    out << "bbMul(";
    break;
  case '/':
    out << "bbDiv(";
    break;
  case MOD:
    out << "bbMod(";
    break;
  default:
    out << "bbOp" << node->op << "(";
  }

  if (node->lhs)
    generate(node->lhs);
  out << ", ";
  if (node->rhs)
    generate(node->rhs);
  out << ")";
}

void CppGenerator::visit(RelExprNode *node) {
  if (node->lhs->sem_type && node->lhs->sem_type->stringType()) {
    out << "(_bbStrCompare(";
    generate(node->lhs);
    out << ", ";
    generate(node->rhs);
    out << ") ";
    switch (node->op) {
    case '=':
      out << "==";
      break;
    case NE:
      out << "!=";
      break;
    case '<':
      out << "<";
      break;
    case '>':
      out << ">";
      break;
    case LE:
      out << "<=";
      break;
    case GE:
      out << ">=";
      break;
    default:
      out << "==";
      break; // Fallback
    }
    out << " 0)";
  } else {
    out << "(";
    if (node->lhs)
      generate(node->lhs);

    switch (node->op) {
    case '<':
      out << " < ";
      break;
    case '>':
      out << " > ";
      break;
    case '=':
      out << " == ";
      break;
    case NE:
      out << " != ";
      break;
    case LE:
      out << " <= ";
      break;
    case GE:
      out << " >= ";
      break;
    default:
      out << " " << (char)node->op << " ";
    }

    if (node->rhs)
      generate(node->rhs);
    out << ")";
  }
}

void CppGenerator::visit(CastNode *node) {
  // If we have explicit casts
  // For now simple pass-through or basic C-style cast
  // Blitz casts are essentially conversions
  if (node->type == Type::string_type) {
    // cast to string
    out << "bbToString(";
    if (node->expr)
      generate(node->expr);
    out << ")";
  } else if (node->type == Type::int_type) {
    out << "((bb_int)";
    if (node->expr)
      generate(node->expr);
    out << ")";
  } else if (node->type == Type::float_type) {
    out << "((bb_float)";
    if (node->expr)
      generate(node->expr);
    out << ")";
  } else {
    // Fallback
    out << "(";
    if (node->expr)
      generate(node->expr);
    out << ")";
  }
}

// Legacy IR overrides - empty/unused
void CppGenerator::emit(const std::string &s) { out << s; }
void CppGenerator::emitln(const std::string &s) { out << indent << s << "\n"; }

void CppGenerator::visit(WhileNode *node) {
  emit(indent);
  out << "while (";
  if (node->expr)
    generate(node->expr);
  out << ") {\n";
  incIndent();
  if (node->stmts)
    generate(node->stmts);
  decIndent();
  emitln("}");
}

// Flatten "else if" logic
void CppGenerator::visit(IfNode *node) {
  emit(indent);
  out << "if (";
  if (node->expr)
    generate(node->expr);
  out << ") {\n";
  incIndent();
  if (node->stmts)
    generate(node->stmts);
  decIndent();
  emitln("}");

  if (node->elseOpt) {
    // Optimization: Check if elseOpt is a single IfNode (ElseIf pattern)
    // StmtSeqNode -> stmts vector
    bool isElseIf = false;
    if (node->elseOpt->stmts.size() == 1) {
      if (IfNode *elseIfNode =
              dynamic_cast<IfNode *>(node->elseOpt->stmts[0])) {
        // Yes, it's an ElseIf!
        out << indent << "else ";
        // Recursively visit without extra braces/indent
        visit(elseIfNode);
        return;
      }
    }

    emitln("else {");
    incIndent();
    generate(node->elseOpt);
    decIndent();
    emitln("}");
  }
}

void CppGenerator::visit(ExitNode *node) { emitln("break;"); }

void CppGenerator::visit(ReturnNode *node) {
  emit(indent);
  if (node->expr) {
    out << "return ";
    generate(node->expr);
    out << ";\n";
  } else {
    out << "goto _bb_return_logic;\n";
  }
}

// --- Select / Case ---
void CppGenerator::visit(SelectNode *node) {
  // Evaluate selector into a temp variable, then emit if-else chain
  emit(indent);
  out << "{\n";
  incIndent();
  emit(indent);
  out << "auto _sel_val = (";
  if (node->expr)
    generate(node->expr);
  out << ");\n";

  bool first = true;
  for (auto *c : node->cases) {
    emit(indent);
    if (!first)
      out << "else ";
    out << "if (";
    // Each case can match multiple expressions
    for (int i = 0; i < (int)c->exprs->exprs.size(); i++) {
      if (i > 0)
        out << " || ";
      out << "_sel_val == (";
      generate(c->exprs->exprs[i]);
      out << ")";
    }
    out << ") {\n";
    incIndent();
    if (c->stmts)
      generate(c->stmts);
    decIndent();
    emitln("}");
    first = false;
  }
  if (node->defStmts) {
    emitln("else {");
    incIndent();
    generate(node->defStmts);
    decIndent();
    emitln("}");
  }
  decIndent();
  emitln("}");
}

// --- Gosub / Label ---
void CppGenerator::visit(GosubNode *node) {
  int id = next_gosub_id++;
  current_gosub_ids.push_back(id);
  emit(indent);
  out << "_bb_gosub_stack.push_back(" << id << "); goto _label_" << node->ident
      << "; _bb_ret_" << id << ": ;\n";
}

void CppGenerator::visit(LabelNode *node) {
  out << "_label_" << node->ident << ": ;\n";
}

void CppGenerator::visit(GotoNode *node) {
  emit(indent);
  out << "goto _label_" << node->ident << ";\n";
}

// --- Repeat / Until ---
void CppGenerator::visit(RepeatNode *node) {
  emitln("do {");
  incIndent();
  if (node->stmts)
    generate(node->stmts);
  decIndent();
  emit(indent);
  out << "} while (!(";
  if (node->expr)
    generate(node->expr);
  else
    out << "0";
  out << "));\n";
}

// --- Include ---
void CppGenerator::visit(IncludeNode *node) {
  // Include was already resolved by the parser (stmts contains the parsed
  // AST)
  out << "// Include: " << node->file << "\n";
  if (node->stmts)
    generate(node->stmts);
}

// --- Arrays ---
void CppGenerator::visit(DimNode *node) {
  emit(indent);
  out << "bbDim(\"" << node->sem_decl->name << "\", {";
  if (node->exprs) {
    generate(node->exprs);
  }
  out << "});\n";
}

void CppGenerator::visit(ArrayVarNode *node) {
  out << "bbArrayAccess(\"" << node->sem_decl->name << "\", {";
  if (node->exprs) {
    generate(node->exprs);
  }
  out << "})";
}

void CppGenerator::visit(DeclSeqNode *node) {
  for (auto *d : node->decls) {
    generate(d);
  }
}

void CppGenerator::visit(ExprSeqNode *node) {
  for (int i = 0; i < node->exprs.size(); ++i) {
    if (i > 0)
      out << ", ";
    generate(node->exprs[i]);
  }
}

// --- Custom Types ---
void CppGenerator::visit(StructDeclNode *node) {
  emitln("struct bb_type_" + node->ident + " {");
  incIndent();
  if (node->fields) {
    for (auto *d : node->fields->decls) {
      if (auto *vdecl = dynamic_cast<VarDeclNode *>(d)) {
        std::string type = "bb_int";
        std::string init = " = 0";
        if (vdecl->tag == "#") {
          type = "bb_float";
          init = " = 0.0f";
        } else if (vdecl->tag == "$") {
          type = "bb_string";
          init = " = \"\"";
        } else if (vdecl->tag != "") {
          type = "bb_type_" + vdecl->tag + "*";
          init = " = nullptr";
        }
        emitln(type + " " + vdecl->sem_var->sem_decl->name + init + ";");
      }
    }
  }
  emitln("bb_type_" + node->ident + " *_next = nullptr;");
  emitln("bb_type_" + node->ident + " *_prev = nullptr;");
  decIndent();
  emitln("};");
  emitln("");
  emitln("bb_type_" + node->ident + " *_bb_type_" + node->ident +
         "_head = nullptr;");
  emitln("bb_type_" + node->ident + " *_bb_type_" + node->ident +
         "_tail = nullptr;");
  emitln("");
  emitln("bb_type_" + node->ident + "* bbNew_" + node->ident + "() {");
  incIndent();
  emitln("bb_type_" + node->ident + " *obj = new bb_type_" + node->ident +
         "();");
  emitln("if (_bb_type_" + node->ident + "_tail) {");
  emitln("    _bb_type_" + node->ident + "_tail->_next = obj;");
  emitln("    obj->_prev = _bb_type_" + node->ident + "_tail;");
  emitln("    _bb_type_" + node->ident + "_tail = obj;");
  emitln("} else {");
  emitln("    _bb_type_" + node->ident + "_head = _bb_type_" + node->ident +
         "_tail = obj;");
  emitln("}");
  emitln("return obj;");
  decIndent();
  emitln("}");
  emitln("");
  emitln("void bbDelete_" + node->ident + "(bb_type_" + node->ident +
         " *obj) {");
  incIndent();
  emitln("if (!obj) return;");
  emitln("if (obj->_prev) obj->_prev->_next = obj->_next;");
  emitln("else _bb_type_" + node->ident + "_head = obj->_next;");
  emitln("if (obj->_next) obj->_next->_prev = obj->_prev;");
  emitln("else _bb_type_" + node->ident + "_tail = obj->_prev;");
  emitln("delete obj;");
  decIndent();
  emitln("}");
  emitln("");
}
void CppGenerator::visit(FieldVarNode *node) {
  generate(node->expr); // The object pointer
  out << "->" << node->sem_field->name;
}

void CppGenerator::visit(NewNode *node) {
  out << "bbNew_" << node->ident << "()";
}

void CppGenerator::visit(NullNode *node) { out << "nullptr"; }

void CppGenerator::visit(FirstNode *node) {
  out << "_bb_type_" << node->ident << "_head";
}

void CppGenerator::visit(LastNode *node) {
  out << "_bb_type_" << node->ident << "_tail";
}

void CppGenerator::visit(AfterNode *node) {
  generate(node->expr);
  out << "->_next";
}

void CppGenerator::visit(BeforeNode *node) {
  generate(node->expr);
  out << "->_prev";
}

void CppGenerator::visit(DeleteNode *node) {
  if (auto *st = node->expr->sem_type->structType()) {
    emit(indent);
    out << "bbDelete_" << st->ident << "(";
    generate(node->expr);
    out << ");\n";
  }
}

void CppGenerator::visit(ForEachNode *node) {
  emit(indent);
  out << "for (bb_type_" << node->typeIdent << " *";
  generate(node->var);
  out << " = _bb_type_" << node->typeIdent << "_head; ";
  generate(node->var);
  out << "; ) {\n";
  incIndent();

  // Safe iteration: get next pointer BEFORE running body
  emit(indent);
  out << "bb_type_" << node->typeIdent << " *__bb_next = ";
  generate(node->var);
  out << "->_next;\n";

  if (node->stmts)
    generate(node->stmts);

  emit(indent);
  generate(node->var);
  out << " = __bb_next;\n";

  decIndent();
  emitln("}");
}

void CppGenerator::visit(DeleteEachNode *node) {
  emit(indent);
  out << "while (_bb_type_" << node->typeIdent << "_head) bbDelete_"
      << node->typeIdent << "(_bb_type_" << node->typeIdent << "_head);\n";
}

void CppGenerator::visit(ObjectCastNode *node) {
  // (type*)expr
  out << "((bb_type_" << node->type_ident << "*)";
  generate(node->expr);
  out << ")";
}

void CppGenerator::visit(ObjectHandleNode *node) {
  // Just the pointer for now
  generate(node->expr);
}

void CppGenerator::visit(ReadNode *node) {
  emit(indent);
  generate(node->var);
  out << " = ";
  if (node->var->sem_type == Type::int_type)
    out << "bbReadInt()";
  else if (node->var->sem_type == Type::float_type)
    out << "bbReadFloat()";
  else if (node->var->sem_type == Type::string_type)
    out << "bbReadString()";
  out << ";\n";
}

void CppGenerator::visit(RestoreNode *node) {
  emit(indent);
  out << "bbRestore(\"" << node->ident << "\");\n";
}

void CppGenerator::visit(DataDeclNode *node) {
  emit(indent);
  out << "bbRegisterData(";
  generate(node->expr);
  out << ", \"" << node->str_label << "\");\n";
}

void CppGenerator::visit(InsertNode *node) {
  if (auto *st = node->expr1->sem_type->structType()) {
    std::string type = st->ident;
    emit(indent);
    out << "// Insert obj1 " << (node->before ? "Before" : "After")
        << " obj2\n";
    emit(indent);
    out << "if (";
    generate(node->expr1);
    out << " && ";
    generate(node->expr2);
    out << ") {\n";
    incIndent();

    // 1. Remove obj1 from list
    emitln("auto *o1 = ");
    generate(node->expr1);
    out << ";\n";
    emitln("auto *o2 = ");
    generate(node->expr2);
    out << ";\n";

    emitln("if (o1->_prev) o1->_prev->_next = o1->_next;");
    emitln("else _bb_type_" + type + "_head = o1->_next;");
    emitln("if (o1->_next) o1->_next->_prev = o1->_prev;");
    emitln("else _bb_type_" + type + "_tail = o1->_prev;");

    // 2. Insert relative to obj2
    if (node->before) {
      emitln("o1->_next = o2;");
      emitln("o1->_prev = o2->_prev;");
      emitln("if (o2->_prev) o2->_prev->_next = o1;");
      emitln("else _bb_type_" + type + "_head = o1;");
      emitln("o2->_prev = o1;");
    } else {
      emitln("o1->_prev = o2;");
      emitln("o1->_next = o2->_next;");
      emitln("if (o2->_next) o2->_next->_prev = o1;");
      emitln("else _bb_type_" + type + "_tail = o1;");
      emitln("o2->_next = o1;");
    }

    decIndent();
    emitln("}");
  }
}

// --- Functions ---
void CppGenerator::visit(FuncDeclNode *node) {
  std::vector<int> old_gosub_ids = current_gosub_ids;
  current_gosub_ids.clear();
  bool old_in_function = in_function;
  in_function = true;

  // Determine return type
  std::string retType = "bb_int";
  if (node->tag == "%")
    retType = "bb_int";
  else if (node->tag == "#")
    retType = "bb_float";
  else if (node->tag == "$")
    retType = "bb_string";

  // Emit function signature
  out << retType << " " << node->sem_decl->name << "(";

  // Params
  if (node->params) {
    bool first = true;
    for (int i = 0; i < (int)node->params->decls.size(); ++i) {
      if (auto *p = dynamic_cast<VarDeclNode *>(node->params->decls[i])) {
        if (!first)
          out << ", ";
        first = false;

        Type *t = node->sem_type->params->decls[i]->type;
        std::string pType = mapType(t);
        out << pType << " " << node->sem_type->params->decls[i]->name;
        if (p->expr) {
          out << " = ";
          generate(p->expr);
        }
      }
    }
  }

  out << ") {\n";
  out << "    std::vector<int> _bb_gosub_stack;\n";

  // We need to set indent context for the function body
  std::string oldIndent = indent;
  indent = "    ";

  // Pre-declare locals using semantic environment
  emitVarDecls(node->sem_env, DECL_LOCAL);

  if (node->stmts)
    generate(node->stmts);

  emitReturnLogic();

  if (retType == "bb_string")
    emitln("    return \"\";");
  else if (retType == "bb_float")
    emitln("    return 0.0f;");
  else
    emitln("    return 0;");

  indent = oldIndent;
  out << "}\n\n";

  current_gosub_ids = old_gosub_ids;
  in_function = old_in_function;
}
std::string CppGenerator::mapType(Type *t) {
  if (!t)
    return "bb_int";
  if (t == Type::int_type)
    return "bb_int";
  if (t == Type::float_type)
    return "bb_float";
  if (t == Type::string_type)
    return "bb_string";
  if (auto *st = t->structType())
    return "bb_type_" + st->ident + "*";
  if (t == Type::null_type)
    return "void*";
  return "bb_int";
}

void CppGenerator::visit(DeclVarNode *node) { emit("/* DeclVarNode */"); }

void CppGenerator::visit(VectorVarNode *node) { emit("/* VectorVarNode */"); }

void CppGenerator::visit(VectorDeclNode *node) { emit("/* VectorDeclNode */"); }
