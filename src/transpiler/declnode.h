
#ifndef DECLNODE_H
#define DECLNODE_H

#include "node.h"

struct DeclNode : public Node {
  DeclNode() {}
  virtual void proto(DeclSeq *d, BCEnviron *e) {}
  virtual void semant(BCEnviron *e) {}
};

struct DeclSeqNode : public Node {
  std::vector<DeclNode *> decls;
  DeclSeqNode() {}
  void accept(Visitor *v);
  ~DeclSeqNode() {
    /*for (; decls.size(); decls.pop_back())
      delete decls.back();*/
  }
  void proto(DeclSeq *d, BCEnviron *e);
  void semant(BCEnviron *e);

  void push_back(DeclNode *d) { decls.push_back(d); }
  int size() { return decls.size(); }
};

#include "exprnode.h"
#include "stmtnode.h"

//'kind' shouldn't really be in Parser...
// should probably be LocalDeclNode,GlobalDeclNode,ParamDeclNode
struct VarDeclNode : public DeclNode {
  std::string ident, tag;
  int kind;
  bool constant;
  ExprNode *expr;
  DeclVarNode *sem_var;
  VarDeclNode(const std::string &i, const std::string &t, int k, bool c,
              ExprNode *e)
      : ident(i), tag(t), kind(k), constant(c), expr(e), sem_var(0) {}
  ~VarDeclNode() {
    /*delete expr;
    delete sem_var;*/
  }
  void proto(DeclSeq *d, BCEnviron *e);
  void semant(BCEnviron *e);
  void accept(Visitor *v);
};

struct FuncDeclNode : public DeclNode {
  std::string ident, tag;
  DeclSeqNode *params;
  StmtSeqNode *stmts;
  FuncType *sem_type;
  BCEnviron *sem_env;
  Decl *sem_decl;
  FuncDeclNode(const std::string &i, const std::string &t, DeclSeqNode *p,
               StmtSeqNode *ss)
      : ident(i), tag(t), params(p), stmts(ss), sem_decl(0) {}
  ~FuncDeclNode() {
    /*delete params;
    delete stmts;*/
  }
  void proto(DeclSeq *d, BCEnviron *e);
  void semant(BCEnviron *e);
  void accept(Visitor *v);
};

struct MethodDeclNode : public DeclNode {
  std::string ident, tag;
  DeclSeqNode *params;
  StmtSeqNode *stmts;
  FuncType *sem_type;
  BCEnviron *sem_env;
  Decl *sem_decl;
  MethodDeclNode(const std::string &i, const std::string &t, DeclSeqNode *p,
                 StmtSeqNode *ss)
      : ident(i), tag(t), params(p), stmts(ss), sem_decl(0) {}
  ~MethodDeclNode() {
    /*delete params;
    delete stmts;*/
  }
  void proto(DeclSeq *d, BCEnviron *e);
  void semant(BCEnviron *e);
  void accept(Visitor *v);
};

struct StructDeclNode : public DeclNode {
  std::string ident;
  DeclSeqNode *fields;
  StructType *sem_type;
  Decl *sem_decl;
  StructDeclNode(const std::string &i, DeclSeqNode *f)
      : ident(i), fields(f), sem_decl(0) {}
  ~StructDeclNode() { /*delete fields;*/ }
  void proto(DeclSeq *d, BCEnviron *e);
  void semant(BCEnviron *e);
  void accept(Visitor *v);
};

struct DataDeclNode : public DeclNode {
  ExprNode *expr;
  std::string str_label;
  DataDeclNode(ExprNode *e) : expr(e) {}
  ~DataDeclNode() { /*delete expr;*/ }
  void proto(DeclSeq *d, BCEnviron *e);
  void semant(BCEnviron *e);
  void accept(Visitor *v);
};

struct VectorDeclNode : public DeclNode {
  std::string ident, tag;
  ExprSeqNode *exprs;
  int kind;
  VectorType *sem_type;
  Decl *sem_decl;
  VectorDeclNode(const std::string &i, const std::string &t, ExprSeqNode *e,
                 int k)
      : ident(i), tag(t), exprs(e), kind(k), sem_decl(0) {}
  ~VectorDeclNode() { /*delete exprs;*/ }
  void proto(DeclSeq *d, BCEnviron *e);
  void accept(Visitor *v);
};

#endif
