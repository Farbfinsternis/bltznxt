
#include "nodes.h"
#include "std.h"
#include "visitor.h"

//////////////////////////////////
// Common get/set for variables //
//////////////////////////////////

bool VarNode::isObjParam() { return false; }

//////////////////
// Declared var //
//////////////////
void DeclVarNode::semant(BCEnviron *e) {}

bool DeclVarNode::isObjParam() {
  return sem_type->structType() && sem_decl->kind == DECL_PARAM;
}

///////////////
// Ident var //
///////////////
void IdentVarNode::semant(BCEnviron *e) {
  if (sem_decl)
    return;
  Type *t = tagType(tag, e);
  if (!t)
    t = Type::int_type;
  std::string m_name = mangle(ident, tag);
  if (sem_decl = e->findDecl(m_name)) {
    if (!(sem_decl->kind & (DECL_GLOBAL | DECL_LOCAL | DECL_PARAM))) {
      ex("Identifier '" + sem_decl->name + "' may not be used like this");
    }
    Type *ty = sem_decl->type;
    if (ty->constType())
      ty = ty->constType()->valueType;
    if (tag.size() && t != ty)
      ex("Variable type mismatch");
  } else {
    // ugly auto decl!
    // Auto-promote to Float if implied by assignment
    if (tag.size() == 0 && e->typeHint == Type::float_type) {
      t = Type::float_type;
      m_name = mangle(ident, "#"); // Auto-decl as float
    }
    sem_decl = e->decls->insertDecl(m_name, t, DECL_LOCAL);
  }
  sem_type = sem_decl->type;
}

/////////////////
// Indexed Var //
/////////////////
void ArrayVarNode::semant(BCEnviron *e) {
  exprs->semant(e);
  exprs->castTo(Type::int_type, e);
  Type *t = e->findType(tag);
  std::string m_name = mangle(ident, tag);
  sem_decl = e->findDecl(m_name);
  if (!sem_decl || !(sem_decl->kind & DECL_ARRAY))
    ex("Array not found");
  ArrayType *a = sem_decl->type->arrayType();
  if (t && t != a->elementType)
    ex("array type mismtach");
  if (a->dims != exprs->size())
    ex("incorrect number of dimensions");
  sem_type = a->elementType;
}

///////////////
// Field var //
///////////////
void FieldVarNode::semant(BCEnviron *e) {
  expr = expr->semant(e);
  StructType *s = expr->sem_type->structType();
  if (!s)
    ex("Variable must be a Type");
  sem_field = s->fields->findDecl(ident);
  if (!sem_field)
    ex("Type field not found");
  sem_type = sem_field->type;
}

////////////////
// Vector var //
////////////////
void VectorVarNode::semant(BCEnviron *e) {
  expr = expr->semant(e);
  vec_type = expr->sem_type->vectorType();
  if (!vec_type)
    ex("Variable must be a Blitz array");
  if (vec_type->sizes.size() != exprs->size())
    ex("Incorrect number of subscripts");
  exprs->semant(e);
  exprs->castTo(Type::int_type, e);
  int k;
  for (k = 0; k < exprs->size(); ++k) {
    if (ConstNode *t = exprs->exprs[k]->constNode()) {
      if (t->intValue() >= vec_type->sizes[k]) {
        ex("Blitz array subscript out of range");
      }
    }
  }
  sem_type = vec_type->elementType;
}

void DeclVarNode::accept(Visitor *v) { v->visit(this); }
void IdentVarNode::accept(Visitor *v) { v->visit(this); }
void ArrayVarNode::accept(Visitor *v) { v->visit(this); }
void FieldVarNode::accept(Visitor *v) { v->visit(this); }
void VectorVarNode::accept(Visitor *v) { v->visit(this); }
