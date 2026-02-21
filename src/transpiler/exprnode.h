
#ifndef EXPRNODE_H
#define EXPRNODE_H

#include "node.h"

struct ConstNode; // is constant int,float or string

struct ExprNode : public Node {
  Type *sem_type;
  ExprNode() : sem_type(0) {}
  ExprNode(Type *t) : sem_type(t) {}

  ExprNode *castTo(Type *ty, BCEnviron *e);
  ExprNode *semant(BCEnviron *e, Type *ty);

  virtual ExprNode *semant(BCEnviron *e) = 0;

  virtual ConstNode *constNode() { return 0; }
};

struct ExprSeqNode : public Node {
  std::vector<ExprNode *> exprs;
  ~ExprSeqNode() {
    /*for (; exprs.size(); exprs.pop_back())
      delete exprs.back();*/
  }
  void push_back(ExprNode *e) { exprs.push_back(e); }
  int size() { return exprs.size(); }
  void semant(BCEnviron *e);

  void castTo(DeclSeq *ds, BCEnviron *e, bool userlib);
  void castTo(Type *t, BCEnviron *e);
  void accept(Visitor *v);
};

#include "varnode.h"

struct CastNode : public ExprNode {
  ExprNode *expr;
  Type *type;
  CastNode(ExprNode *ex, Type *ty) : expr(ex), type(ty) {}
  ~CastNode() { /*delete expr;*/ }
  ExprNode *semant(BCEnviron *e);
  void accept(Visitor *v);
};

struct CallNode : public ExprNode {
  std::string ident, tag;
  ExprSeqNode *exprs;
  Decl *sem_decl;
  CallNode(const std::string &i, const std::string &t, ExprSeqNode *e)
      : ident(i), tag(t), exprs(e) {}
  ~CallNode() { /*delete exprs;*/ }
  ExprNode *semant(BCEnviron *e);
  void accept(Visitor *v);
};

struct VarExprNode : public ExprNode {
  VarNode *var;
  VarExprNode(VarNode *v) : var(v) {}
  ~VarExprNode() { /*delete var;*/ }
  ExprNode *semant(BCEnviron *e);
  void accept(Visitor *v);
};

struct ConstNode : public ExprNode {
  ExprNode *semant(BCEnviron *e) { return this; }
  ConstNode *constNode() { return this; }
  virtual int intValue() = 0;
  virtual float floatValue() = 0;
  virtual std::string stringValue() = 0;
};

struct IntConstNode : public ConstNode {
  int value;
  IntConstNode(int n);

  int intValue();
  float floatValue();
  std::string stringValue();
  void accept(Visitor *v);
};

struct FloatConstNode : public ConstNode {
  float value;
  FloatConstNode(float f);

  int intValue();
  float floatValue();
  std::string stringValue();
  void accept(Visitor *v);
};

struct StringConstNode : public ConstNode {
  std::string value;
  StringConstNode(const std::string &s);

  int intValue();
  float floatValue();
  std::string stringValue();
  void accept(Visitor *v);
};

struct UniExprNode : public ExprNode {
  int op;
  ExprNode *expr;
  UniExprNode(int op, ExprNode *expr) : op(op), expr(expr) {}
  ~UniExprNode() { /*delete expr;*/ }
  ExprNode *constize();
  ExprNode *semant(BCEnviron *e);
  void accept(Visitor *v);
};

// and, or, eor, lsl, lsr, asr
struct BinExprNode : public ExprNode {
  int op;
  ExprNode *lhs, *rhs;
  BinExprNode(int op, ExprNode *lhs, ExprNode *rhs)
      : op(op), lhs(lhs), rhs(rhs) {}
  ~BinExprNode() {
    /*delete lhs;
    delete rhs;*/
  }
  ExprNode *semant(BCEnviron *e);
  void accept(Visitor *v);
};

// *,/,Mod,+,-
struct ArithExprNode : public ExprNode {
  int op;
  ExprNode *lhs, *rhs;
  ArithExprNode(int op, ExprNode *lhs, ExprNode *rhs)
      : op(op), lhs(lhs), rhs(rhs) {}
  ~ArithExprNode() {
    /*delete lhs;
    delete rhs;*/
  }
  ExprNode *semant(BCEnviron *e);
  void accept(Visitor *v); // Note: CppGenerator might visit ArithExprNode
                           // directly or via BinExprNode
};

//<,=,>,<=,<>,>=
struct RelExprNode : public ExprNode {
  int op;
  ExprNode *lhs, *rhs;
  Type *opType;
  RelExprNode(int op, ExprNode *lhs, ExprNode *rhs)
      : op(op), lhs(lhs), rhs(rhs) {}
  ~RelExprNode() {
    /*delete lhs;
    delete rhs;*/
  }
  ExprNode *semant(BCEnviron *e);
  void accept(Visitor *v);
};

struct NewNode : public ExprNode {
  std::string ident;
  NewNode(const std::string &i) : ident(i) {}
  ExprNode *semant(BCEnviron *e);
  void accept(Visitor *v);
};

struct FirstNode : public ExprNode {
  std::string ident;
  FirstNode(const std::string &i) : ident(i) {}
  ExprNode *semant(BCEnviron *e);
  void accept(Visitor *v);
};

struct LastNode : public ExprNode {
  std::string ident;
  LastNode(const std::string &i) : ident(i) {}
  ExprNode *semant(BCEnviron *e);
  void accept(Visitor *v);
};

struct AfterNode : public ExprNode {
  ExprNode *expr;
  AfterNode(ExprNode *e) : expr(e) {}
  ~AfterNode() { /*delete expr;*/ }
  ExprNode *semant(BCEnviron *e);
  void accept(Visitor *v);
};

struct BeforeNode : public ExprNode {
  ExprNode *expr;
  BeforeNode(ExprNode *e) : expr(e) {}
  ~BeforeNode() { /*delete expr;*/ }
  ExprNode *semant(BCEnviron *e);
  void accept(Visitor *v);
};

struct NullNode : public ExprNode {
  ExprNode *semant(BCEnviron *e);
  void accept(Visitor *v);
};

struct SelfNode : public ExprNode {
  ExprNode *semant(BCEnviron *e);
  void accept(Visitor *v);
};

struct MethodCallNode : public ExprNode {
  ExprNode *expr;
  std::string ident, tag;
  ExprSeqNode *exprs;
  MethodCallNode(ExprNode *e, const std::string &i, const std::string &t,
                 ExprSeqNode *es)
      : expr(e), ident(i), tag(t), exprs(es) {}
  ~MethodCallNode() {
    delete expr;
    delete exprs;
  }
  ExprNode *semant(BCEnviron *e) override;
  void accept(Visitor *v) override;
};

struct ObjectCastNode : public ExprNode {
  ExprNode *expr;
  std::string type_ident;
  ObjectCastNode(ExprNode *e, const std::string &t) : expr(e), type_ident(t) {}
  ~ObjectCastNode() { /*delete expr;*/ }
  ExprNode *semant(BCEnviron *e);
  void accept(Visitor *v);
};

struct ObjectHandleNode : public ExprNode {
  ExprNode *expr;
  ObjectHandleNode(ExprNode *e) : expr(e) {}
  ~ObjectHandleNode() { /*delete expr;*/ }
  ExprNode *semant(BCEnviron *e);
  void accept(Visitor *v);
};

#endif
