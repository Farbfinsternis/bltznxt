
#ifndef VARNODE_H
#define VARNODE_H

#include "node.h"

struct VarNode : public Node {
  Type *sem_type;

  virtual bool isObjParam();

  // addr of var
  virtual void semant(BCEnviron *e) = 0;
};

#include "decl.h"

struct DeclVarNode : public VarNode {
  Decl *sem_decl;
  DeclVarNode(Decl *d = 0) : sem_decl(d) {
    if (d)
      sem_type = d->type;
  }
  void semant(BCEnviron *e);

  bool isObjParam();
  void accept(Visitor *v);
};

struct IdentVarNode : public DeclVarNode {
  std::string ident, tag;
  IdentVarNode(const std::string &i, const std::string &t) : ident(i), tag(t) {}
  void semant(BCEnviron *e);
  void accept(Visitor *v);
};

struct ArrayVarNode : public VarNode {
  std::string ident, tag;
  ExprSeqNode *exprs;
  Decl *sem_decl;
  ArrayVarNode(const std::string &i, const std::string &t, ExprSeqNode *e)
      : ident(i), tag(t), exprs(e) {}
  ~ArrayVarNode() { /*delete exprs;*/ }
  void semant(BCEnviron *e);
  void accept(Visitor *v);
};

struct FieldVarNode : public VarNode {
  ExprNode *expr;
  std::string ident, tag;
  Decl *sem_field;
  FieldVarNode(ExprNode *e, const std::string &i, const std::string &t)
      : expr(e), ident(i), tag(t) {}
  ~FieldVarNode() { /*delete expr;*/ }
  void semant(BCEnviron *e);
  void accept(Visitor *v);
};

struct VectorVarNode : public VarNode {
  ExprNode *expr;
  ExprSeqNode *exprs;
  VectorType *vec_type;
  VectorVarNode(ExprNode *e, ExprSeqNode *es) : expr(e), exprs(es) {}
  ~VectorVarNode() {
    /*delete expr;
    delete exprs;*/
  }
  void semant(BCEnviron *e);
  void accept(Visitor *v);
};

#endif
