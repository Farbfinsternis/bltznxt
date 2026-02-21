
#include "nodes.h"
#include "std.h"
#include "visitor.h"

//////////////////////////////
// Sequence of declarations //
//////////////////////////////
void DeclSeqNode::proto(DeclSeq *d, BCEnviron *e) {
  int k;
  for (k = 0; k < decls.size(); ++k) {
    try {
      decls[k]->proto(d, e);
    } catch (Ex &x) {
      if (x.pos < 0)
        x.pos = decls[k]->pos;
      if (!x.file.size())
        x.file = decls[k]->file;
      throw;
    }
  }
}

void DeclSeqNode::semant(BCEnviron *e) {
  int k;
  for (k = 0; k < decls.size(); ++k) {
    try {
      decls[k]->semant(e);
    } catch (Ex &x) {
      if (x.pos < 0)
        x.pos = decls[k]->pos;
      if (!x.file.size())
        x.file = decls[k]->file;
      throw;
    }
  }
}

////////////////////////////
// Simple var declaration //
////////////////////////////
void VarDeclNode::proto(DeclSeq *d, BCEnviron *e) {

  Type *ty = tagType(tag, e);
  if (!ty)
    ty = Type::int_type;
  ConstType *defType = 0;

  if (expr) {
    expr = expr->semant(e);
    expr = expr->castTo(ty, e);
    if (constant || (kind & DECL_PARAM)) {
      ConstNode *c = expr->constNode();
      if (!c)
        semex("Expression must be constant");
      if (ty == Type::int_type)
        ty = d_new<ConstType>(c->intValue());
      else if (ty == Type::float_type)
        ty = d_new<ConstType>(c->floatValue());
      else
        ty = d_new<ConstType>(c->stringValue());
      e->types.push_back(ty);
      delete expr;
      expr = 0;
    }
    if (kind & DECL_PARAM) {
      defType = ty->constType();
      ty = defType->valueType;
    }
  } else if (constant)
    semex("Constants must be initialized");

  std::string m_name = mangle(ident, tag);
  Decl *decl = d->insertDecl(m_name, ty, kind, defType);
  if (!decl)
    semex("Duplicate variable name");
  if (expr)
    sem_var = d_new<DeclVarNode>(decl);
}

void VarDeclNode::semant(BCEnviron *e) {}

//////////////////////////
// Function Declaration //
//////////////////////////
void FuncDeclNode::proto(DeclSeq *d, BCEnviron *e) {
  Type *t = tagType(tag, e);
  if (!t)
    t = Type::int_type;
  a_ptr<DeclSeq> decls(d_new<DeclSeq>());
  params->proto(decls, e);
  sem_type = d_new<FuncType>(t, decls.release(), false, false);
  std::string m_name = mangle(ident, tag);
  sem_decl = d->insertDecl(m_name, sem_type, DECL_FUNC);
  if (!sem_decl) {
    semex("duplicate identifier");
  }
  e->types.push_back(sem_type);
}

void FuncDeclNode::semant(BCEnviron *e) {

  sem_env = d_new<BCEnviron>(genLabel(), sem_type->returnType, 1, e);
  DeclSeq *decls = sem_env->decls;

  int k;
  for (k = 0; k < sem_type->params->size(); ++k) {
    Decl *d = sem_type->params->decls[k];
    if (!decls->insertDecl(d->name, d->type, d->kind))
      semex("duplicate identifier");
  }

  stmts->semant(sem_env);
}

//////////////////////
// Type Declaration //
//////////////////////
void StructDeclNode::proto(DeclSeq *d, BCEnviron *e) {
  sem_type = d_new<StructType>(ident, d_new<DeclSeq>());
  sem_decl = d->insertDecl(ident, sem_type, DECL_STRUCT);
  if (!sem_decl) {
    semex("Duplicate identifier");
  }
  e->types.push_back(sem_type);
}

void StructDeclNode::semant(BCEnviron *e) {
  fields->proto(sem_type->fields, e);
  int k;
  for (k = 0; k < sem_type->fields->size(); ++k)
    sem_type->fields->decls[k]->offset = k * 4;
}

//////////////////////
// Data declaration //
//////////////////////
void DataDeclNode::proto(DeclSeq *d, BCEnviron *e) {
  expr = expr->semant(e);
  ConstNode *c = expr->constNode();
  if (!c)
    semex("Data expression must be constant");
  if (expr->sem_type == Type::string_type)
    str_label = genLabel();
}

void DataDeclNode::semant(BCEnviron *e) {}

////////////////////////
// Vector declaration //
////////////////////////
void VectorDeclNode::proto(DeclSeq *d, BCEnviron *env) {

  Type *ty = tagType(tag, env);
  if (!ty)
    ty = Type::int_type;

  std::vector<int> sizes;
  int k;
  for (k = 0; k < exprs->size(); ++k) {
    ExprNode *e = exprs->exprs[k] = exprs->exprs[k]->semant(env);
    ConstNode *c = e->constNode();
    if (!c)
      semex("Blitz array sizes must be constant");
    int n = c->intValue();
    if (n < 0)
      semex("Blitz array sizes must not be negative");
    sizes.push_back(n + 1);
  }
  std::string label = genLabel();
  sem_type = d_new<VectorType>(label, ty, sizes);
  std::string m_name = mangle(ident, tag);
  sem_decl = d->insertDecl(m_name, sem_type, kind);
  if (!sem_decl) {
    semex("Duplicate identifier");
  }
  env->types.push_back(sem_type);
}

void DeclSeqNode::accept(Visitor *v) { v->visit(this); }
void VarDeclNode::accept(Visitor *v) { v->visit(this); }
void FuncDeclNode::accept(Visitor *v) { v->visit(this); }
void StructDeclNode::accept(Visitor *v) { v->visit(this); }
void DataDeclNode::accept(Visitor *v) { v->visit(this); }
void VectorDeclNode::accept(Visitor *v) { v->visit(this); }
