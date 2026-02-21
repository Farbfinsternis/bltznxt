
#include "nodes.h"
#include "std.h"
#include "visitor.h"

static std::string fileLabel;
static std::map<std::string, std::string> fileMap;

void StmtSeqNode::reset(const std::string &file, const std::string &lab) {
  fileLabel = "";
  fileMap.clear();

  fileMap[file] = lab;
}

////////////////////////
// Statement Sequence //
////////////////////////
void StmtSeqNode::semant(BCEnviron *e) {
  int k;
  for (k = 0; k < stmts.size(); ++k) {
    try {
      stmts[k]->semant(e);
    } catch (Ex &x) {
      if (x.pos < 0)
        x.pos = stmts[k]->pos;
      if (!x.file.size())
        x.file = file;
      throw;
    }
  }
}

/////////////////
// An Include! //
/////////////////
void IncludeNode::semant(BCEnviron *e) {

  label = genLabel();
  fileMap[file] = label;

  stmts->semant(e);
}

///////////////////
// a declaration //
///////////////////
void DeclStmtNode::semant(BCEnviron *e) {
  decl->proto(e->decls, e);
  decl->semant(e);
}

//////////////////////////////
// Dim AND declare an Array //
//////////////////////////////
void DimNode::semant(BCEnviron *e) {
  Type *t = tagType(tag, e);
  std::string m_name = mangle(ident, tag);
  if (Decl *d = e->findDecl(m_name)) {
    ArrayType *a = d->type->arrayType();
    if (!a || a->dims != exprs->size() || (t && a->elementType != t)) {
      semex("Duplicate identifier");
    }
    sem_type = a;
    sem_decl = 0;
  } else {
    if (e->level > 0)
      semex("Array not found in main program");
    if (!t)
      t = Type::int_type;
    sem_type = d_new<ArrayType>(t, exprs->size());
    sem_decl = e->decls->insertDecl(m_name, sem_type, DECL_ARRAY);
    e->types.push_back(sem_type);
  }
  exprs->semant(e);
  exprs->castTo(Type::int_type, e);
}

////////////////
// Assignment //
////////////////
void AssNode::semant(BCEnviron *e) {
  // Evaluate expr first to allow for type inference
  expr = expr->semant(e);

  e->typeHint = expr->sem_type;
  var->semant(e);
  e->typeHint = 0;

  if (var->sem_type->constType())
    semex("Constants can not be assigned to");
  if (var->sem_type->vectorType())
    semex("Blitz arrays can not be assigned to");

  // expr is already semanted, but we need to cast it to var's type
  expr = expr->castTo(var->sem_type, e);
}

//////////////////////////
// Expression statement //
//////////////////////////
void ExprStmtNode::semant(BCEnviron *e) { expr = expr->semant(e); }

////////////////
// user label //
////////////////
void LabelNode::semant(BCEnviron *e) {
  if (Label *l = e->findLabel(ident)) {
    if (l->def >= 0)
      semex("duplicate label");
    l->def = pos;
    l->data_sz = data_sz;
  } else
    e->insertLabel(ident, pos, -1, data_sz);
  ident = e->funcLabel + ident;
}

//////////////////
// Restore data //
//////////////////
void RestoreNode::semant(BCEnviron *e) {
  if (e->level > 0)
    e = e->globals;

  if (ident.size() == 0)
    sem_label = 0;
  else {
    sem_label = e->findLabel(ident);
    if (!sem_label)
      sem_label = e->insertLabel(ident, -1, pos, -1);
  }
}

////////////////////
// Goto statement //
////////////////////
void GotoNode::semant(BCEnviron *e) {
  if (!e->findLabel(ident)) {
    e->insertLabel(ident, -1, pos, -1);
  }
  ident = e->funcLabel + ident;
}

/////////////////////
// Gosub statement //
/////////////////////
void GosubNode::semant(BCEnviron *e) {
  if (e->level > 0)
    semex("'Gosub' may not be used inside a function");
  if (!e->findLabel(ident))
    e->insertLabel(ident, -1, pos, -1);
  ident = e->funcLabel + ident;
}

//////////////////
// If statement //
//////////////////
void IfNode::semant(BCEnviron *e) {
  expr = expr->semant(e);
  expr = expr->castTo(Type::int_type, e);
  stmts->semant(e);
  if (elseOpt)
    elseOpt->semant(e);
}

///////////
// Break //
///////////
void ExitNode::semant(BCEnviron *e) {
  sem_brk = e->breakLabel;
  if (!sem_brk.size())
    semex("break must appear inside a loop");
}

/////////////////////
// While statement //
/////////////////////
void WhileNode::semant(BCEnviron *e) {
  expr = expr->semant(e);
  expr = expr->castTo(Type::int_type, e);
  std::string brk = e->setBreak(sem_brk = genLabel());
  stmts->semant(e);
  e->setBreak(brk);
}

///////////////////
// For/Next loop //
///////////////////
ForNode::ForNode(VarNode *var, ExprNode *from, ExprNode *to, ExprNode *step,
                 StmtSeqNode *ss, int np)
    : var(var), fromExpr(from), toExpr(to), stepExpr(step), stmts(ss),
      nextPos(np) {}

ForNode::~ForNode() {
  /*delete stmts;
  delete stepExpr;
  delete toExpr;
  delete fromExpr;
  delete var;*/
}

void ForNode::semant(BCEnviron *e) {
  var->semant(e);
  Type *ty = var->sem_type;
  if (ty->constType())
    semex("Index variable can not be constant");
  if (ty != Type::int_type && ty != Type::float_type) {
    semex("index variable must be integer or real");
  }
  fromExpr = fromExpr->semant(e);
  fromExpr = fromExpr->castTo(ty, e);
  toExpr = toExpr->semant(e);
  toExpr = toExpr->castTo(ty, e);
  stepExpr = stepExpr->semant(e);
  stepExpr = stepExpr->castTo(ty, e);

  if (!stepExpr->constNode())
    semex("Step value must be constant");

  std::string brk = e->setBreak(sem_brk = genLabel());
  stmts->semant(e);
  e->setBreak(brk);
}

///////////////////////////////
// For each object of a type //
///////////////////////////////
void ForEachNode::semant(BCEnviron *e) {
  var->semant(e);
  Type *ty = var->sem_type;

  if (ty->structType() == 0)
    semex("Index variable is not a NewType");
  Type *t = e->findType(typeIdent);
  if (!t)
    semex("Type name not found");
  if (t != ty)
    semex("Type mismatch");

  std::string brk = e->setBreak(sem_brk = genLabel());
  stmts->semant(e);
  e->setBreak(brk);
}

////////////////////////////
// Return from a function //
////////////////////////////
void ReturnNode::semant(BCEnviron *e) {
  if (e->level <= 0 && expr) {
    semex("Main program cannot return a value");
  }
  if (e->level > 0) {
    if (!expr) {
      if (e->returnType == Type::float_type) {
        expr = d_new<FloatConstNode>(0.0f);
      } else if (e->returnType == Type::string_type) {
        expr = d_new<StringConstNode>("");
      } else if (e->returnType->structType()) {
        expr = d_new<NullNode>();
      } else {
        expr = d_new<IntConstNode>(0);
      }
    }
    expr = expr->semant(e);
    expr = expr->castTo(e->returnType, e);
    returnLabel = e->funcLabel + "_leave";
  }
}

//////////////////////
// Delete statement //
//////////////////////
void DeleteNode::semant(BCEnviron *e) {
  expr = expr->semant(e);
  if (expr->sem_type->structType() == 0)
    semex("Can't delete non-Newtype");
}

///////////////////////////
// Delete each of a type //
///////////////////////////
void DeleteEachNode::semant(BCEnviron *e) {
  Type *t = e->findType(typeIdent);
  if (!t || t->structType() == 0)
    semex("Specified name is not a NewType name");
}

///////////////////////////
// Insert object in list //
///////////////////////////
void InsertNode::semant(BCEnviron *e) {
  expr1 = expr1->semant(e);
  expr2 = expr2->semant(e);
  StructType *t1 = expr1->sem_type->structType();
  StructType *t2 = expr2->sem_type->structType();
  if (!t1 || !t2)
    semex("Illegal expression type");
  if (t1 != t2)
    semex("Objects types are differnt");
}

////////////////////////
// A select statement //
////////////////////////
void SelectNode::semant(BCEnviron *e) {
  expr = expr->semant(e);
  Type *ty = expr->sem_type;
  if (ty->structType())
    semex("Select cannot be used with objects");

  // we need a temp var
  Decl *d = e->decls->insertDecl(genLabel(), expr->sem_type, DECL_LOCAL);
  sem_temp = d_new<DeclVarNode>(d);

  for (int k = 0; k < cases.size(); ++k) {
    CaseNode *c = cases[k];
    c->exprs->semant(e);
    c->exprs->castTo(ty, e);
    c->stmts->semant(e);
  }
  if (defStmts)
    defStmts->semant(e);
}

////////////////////////////
// Repeat...Until/Forever //
////////////////////////////
void RepeatNode::semant(BCEnviron *e) {
  sem_brk = genLabel();
  std::string brk = e->setBreak(sem_brk);
  stmts->semant(e);
  e->setBreak(brk);
  if (expr) {
    expr = expr->semant(e);
    expr = expr->castTo(Type::int_type, e);
  }
}

///////////////
// Read data //
///////////////
void ReadNode::semant(BCEnviron *e) {
  var->semant(e);
  if (var->sem_type->constType())
    semex("Constants can not be modified");
  if (var->sem_type->structType())
    semex("Data can not be read into an object");
}

void StmtSeqNode::accept(Visitor *v) { v->visit(this); }
void IncludeNode::accept(Visitor *v) { v->visit(this); }
void DeclStmtNode::accept(Visitor *v) { v->visit(this); }
void DimNode::accept(Visitor *v) { v->visit(this); }
void AssNode::accept(Visitor *v) { v->visit(this); }
void ExprStmtNode::accept(Visitor *v) { v->visit(this); }
void LabelNode::accept(Visitor *v) { v->visit(this); }
void GotoNode::accept(Visitor *v) { v->visit(this); }
void GosubNode::accept(Visitor *v) { v->visit(this); }
void IfNode::accept(Visitor *v) { v->visit(this); }
void ExitNode::accept(Visitor *v) { v->visit(this); }
void WhileNode::accept(Visitor *v) { v->visit(this); }
void ForNode::accept(Visitor *v) { v->visit(this); }
void ForEachNode::accept(Visitor *v) { v->visit(this); }
void ReturnNode::accept(Visitor *v) { v->visit(this); }
void DeleteNode::accept(Visitor *v) { v->visit(this); }
void DeleteEachNode::accept(Visitor *v) { v->visit(this); }
void InsertNode::accept(Visitor *v) { v->visit(this); }
void SelectNode::accept(Visitor *v) { v->visit(this); }
void RepeatNode::accept(Visitor *v) { v->visit(this); }
void ReadNode::accept(Visitor *v) { v->visit(this); }
void RestoreNode::accept(Visitor *v) { v->visit(this); }
void CaseNode::accept(Visitor *v) {
  // CaseNode is typically visited by SelectNode, but for completeness:
  // v->visit(this); // Need verify if Visitor has visit(CaseNode*)
  // If not, maybe it shouldn't be Visited directly?
  // But Node::accept is pure virtual. If CaseNode inherits Node, it needs
  // implementation.
}
