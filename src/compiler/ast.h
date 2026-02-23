#ifndef BLITZNEXT_AST_H
#define BLITZNEXT_AST_H

#include "token.h"
#include <memory>
#include <string>
#include <utility>
#include <vector>

// Forward declarations
class LiteralExpr;
class BinaryExpr;
class UnaryExpr;
class VarExpr;
class VarDecl;
class AssignStmt;
class IfStmt;
class WhileStmt;
class RepeatStmt;
class ForStmt;
class SelectStmt;
class CallExpr;
class FunctionDecl;
class ReturnStmt;
class ConstDecl;
class DimStmt;
class ArrayAccess;
class ArrayAssignStmt;
class ExitStmt;
class EndStmt;
class LabelStmt;
class GotoStmt;
class GosubStmt;
class DataStmt;
class ReadStmt;
class RestoreStmt;
class TypeDecl;
class NewExpr;
class DeleteStmt;
class FieldAccess;
class FieldAssignStmt;
class FirstExpr;
class LastExpr;
class BeforeExpr;
class AfterExpr;
class InsertStmt;
class ForEachStmt;
class Program;

// ---- Visitor ----

class ASTVisitor {
public:
  virtual ~ASTVisitor() = default;
  virtual void visit(LiteralExpr  *node) = 0;
  virtual void visit(BinaryExpr   *node) = 0;
  virtual void visit(UnaryExpr    *node) = 0;
  virtual void visit(VarExpr      *node) = 0;
  virtual void visit(VarDecl      *node) = 0;
  virtual void visit(AssignStmt   *node) = 0;
  virtual void visit(IfStmt       *node) = 0;
  virtual void visit(WhileStmt    *node) = 0;
  virtual void visit(RepeatStmt   *node) = 0;
  virtual void visit(ForStmt      *node) = 0;
  virtual void visit(SelectStmt   *node) = 0;
  virtual void visit(CallExpr     *node) = 0;
  virtual void visit(FunctionDecl *node) = 0;
  virtual void visit(ReturnStmt   *node) = 0;
  virtual void visit(ConstDecl    *node) = 0;
  virtual void visit(DimStmt      *node) = 0;
  virtual void visit(ArrayAccess  *node) = 0;
  virtual void visit(ArrayAssignStmt *node) = 0;
  virtual void visit(ExitStmt     *node) = 0;
  virtual void visit(EndStmt      *node) = 0;
  virtual void visit(LabelStmt    *node) = 0;
  virtual void visit(GotoStmt     *node) = 0;
  virtual void visit(GosubStmt    *node) = 0;
  virtual void visit(DataStmt     *node) = 0;
  virtual void visit(ReadStmt     *node) = 0;
  virtual void visit(RestoreStmt  *node) = 0;
  virtual void visit(TypeDecl      *node) = 0;
  virtual void visit(NewExpr       *node) = 0;
  virtual void visit(DeleteStmt    *node) = 0;
  virtual void visit(FieldAccess   *node) = 0;
  virtual void visit(FieldAssignStmt *node) = 0;
  virtual void visit(FirstExpr     *node) = 0;
  virtual void visit(LastExpr      *node) = 0;
  virtual void visit(BeforeExpr    *node) = 0;
  virtual void visit(AfterExpr     *node) = 0;
  virtual void visit(InsertStmt    *node) = 0;
  virtual void visit(ForEachStmt   *node) = 0;
  virtual void visit(Program       *node) = 0;
};

// ---- Base nodes ----

class ASTNode {
public:
  int line = 0; // source line where this node originates (0 = unknown)
  virtual ~ASTNode() = default;
  virtual void accept(ASTVisitor *v) = 0;
};

class ExprNode : public ASTNode {};

class StmtNode : public ASTNode {};

// ---- Expressions ----

class LiteralExpr : public ExprNode {
public:
  Token token;
  explicit LiteralExpr(Token t) : token(std::move(t)) {}
  void accept(ASTVisitor *v) override { v->visit(this); }
};

class BinaryExpr : public ExprNode {
public:
  std::string op;
  std::unique_ptr<ExprNode> left, right;
  BinaryExpr(std::string op, std::unique_ptr<ExprNode> l,
             std::unique_ptr<ExprNode> r)
      : op(std::move(op)), left(std::move(l)), right(std::move(r)) {}
  void accept(ASTVisitor *v) override { v->visit(this); }
};

class UnaryExpr : public ExprNode {
public:
  std::string op;
  std::unique_ptr<ExprNode> expr;
  UnaryExpr(std::string op, std::unique_ptr<ExprNode> e)
      : op(std::move(op)), expr(std::move(e)) {}
  void accept(ASTVisitor *v) override { v->visit(this); }
};

class VarExpr : public ExprNode {
public:
  std::string name;
  explicit VarExpr(std::string n) : name(std::move(n)) {}
  void accept(ASTVisitor *v) override { v->visit(this); }
};

class CallExpr : public ExprNode {
public:
  std::string name;
  std::vector<std::unique_ptr<ExprNode>> args;
  explicit CallExpr(std::string n) : name(std::move(n)) {}
  void accept(ASTVisitor *v) override { v->visit(this); }
};

// ---- Statements ----

class VarDecl : public StmtNode {
public:
  enum Scope { LOCAL, GLOBAL };
  Scope scope;
  std::string name;
  std::string typeHint; // #  %  !  $
  std::unique_ptr<ExprNode> initValue;
  VarDecl(Scope s, std::string n, std::string th,
          std::unique_ptr<ExprNode> init = nullptr)
      : scope(s), name(std::move(n)), typeHint(std::move(th)),
        initValue(std::move(init)) {}
  void accept(ASTVisitor *v) override { v->visit(this); }
};

class AssignStmt : public StmtNode {
public:
  std::string name;
  std::unique_ptr<ExprNode> value;
  AssignStmt(std::string n, std::unique_ptr<ExprNode> v)
      : name(std::move(n)), value(std::move(v)) {}
  void accept(ASTVisitor *v) override { v->visit(this); }
};

class IfStmt : public StmtNode {
public:
  std::unique_ptr<ExprNode> condition;
  std::vector<std::unique_ptr<ASTNode>> thenBlock;
  std::vector<std::unique_ptr<ASTNode>> elseBlock; // may contain nested IfStmt
  explicit IfStmt(std::unique_ptr<ExprNode> cond)
      : condition(std::move(cond)) {}
  void accept(ASTVisitor *v) override { v->visit(this); }
};

class WhileStmt : public StmtNode {
public:
  std::unique_ptr<ExprNode> condition;
  std::vector<std::unique_ptr<ASTNode>> block;
  explicit WhileStmt(std::unique_ptr<ExprNode> cond)
      : condition(std::move(cond)) {}
  void accept(ASTVisitor *v) override { v->visit(this); }
};

class RepeatStmt : public StmtNode {
public:
  std::unique_ptr<ExprNode> condition; // nullptr = FOREVER
  std::vector<std::unique_ptr<ASTNode>> block;
  void accept(ASTVisitor *v) override { v->visit(this); }
};

class ForStmt : public StmtNode {
public:
  std::string varName;
  std::unique_ptr<ExprNode> start, end, step; // step may be nullptr
  std::vector<std::unique_ptr<ASTNode>> block;
  ForStmt(std::string n, std::unique_ptr<ExprNode> s,
          std::unique_ptr<ExprNode> e, std::unique_ptr<ExprNode> st = nullptr)
      : varName(std::move(n)), start(std::move(s)), end(std::move(e)),
        step(std::move(st)) {}
  void accept(ASTVisitor *v) override { v->visit(this); }
};

class SelectStmt : public StmtNode {
public:
  struct Case {
    std::vector<std::unique_ptr<ExprNode>> expressions;
    std::vector<std::unique_ptr<ASTNode>> block;
  };
  std::unique_ptr<ExprNode> expr;
  std::vector<Case> cases;
  std::vector<std::unique_ptr<ASTNode>> defaultBlock;
  explicit SelectStmt(std::unique_ptr<ExprNode> e) : expr(std::move(e)) {}
  void accept(ASTVisitor *v) override { v->visit(this); }
};

class FunctionDecl : public StmtNode {
public:
  std::string name;
  std::vector<std::pair<std::string, std::string>> params; // (name, typeHint)
  std::vector<std::unique_ptr<ASTNode>> body;
  explicit FunctionDecl(std::string n) : name(std::move(n)) {}
  void accept(ASTVisitor *v) override { v->visit(this); }
};

class ReturnStmt : public StmtNode {
public:
  std::unique_ptr<ExprNode> value; // nullptr = bare Return
  explicit ReturnStmt(std::unique_ptr<ExprNode> v = nullptr)
      : value(std::move(v)) {}
  void accept(ASTVisitor *v) override { v->visit(this); }
};

class ConstDecl : public StmtNode {
public:
  std::string name;
  std::string typeHint; // %  #  !  $  or ""
  std::unique_ptr<ExprNode> value;
  ConstDecl(std::string n, std::string th, std::unique_ptr<ExprNode> val)
      : name(std::move(n)), typeHint(std::move(th)), value(std::move(val)) {}
  void accept(ASTVisitor *v) override { v->visit(this); }
};

// Dim arr%(5)  or  Dim grid%(4,4)
class DimStmt : public StmtNode {
public:
  std::string name;
  std::string typeHint; // %  #  !  $  or ""
  std::vector<std::unique_ptr<ExprNode>> dims;
  DimStmt(std::string n, std::string th)
      : name(std::move(n)), typeHint(std::move(th)) {}
  void accept(ASTVisitor *v) override { v->visit(this); }
};

// arr(i)  or  grid(x, y)  used as an expression
class ArrayAccess : public ExprNode {
public:
  std::string name;
  std::vector<std::unique_ptr<ExprNode>> indices;
  explicit ArrayAccess(std::string n) : name(std::move(n)) {}
  void accept(ASTVisitor *v) override { v->visit(this); }
};

// arr(i) = expr
class ArrayAssignStmt : public StmtNode {
public:
  std::string name;
  std::vector<std::unique_ptr<ExprNode>> indices;
  std::unique_ptr<ExprNode> value;
  ArrayAssignStmt(std::string n, std::unique_ptr<ExprNode> v)
      : name(std::move(n)), value(std::move(v)) {}
  void accept(ASTVisitor *v) override { v->visit(this); }
};

class ExitStmt : public StmtNode {
public:
  void accept(ASTVisitor *v) override { v->visit(this); }
};

class EndStmt : public StmtNode {
public:
  void accept(ASTVisitor *v) override { v->visit(this); }
};

// .labelname  or  labelname:
class LabelStmt : public StmtNode {
public:
  std::string name; // always lowercase
  explicit LabelStmt(std::string n) : name(std::move(n)) {}
  void accept(ASTVisitor *v) override { v->visit(this); }
};

// Goto labelname
class GotoStmt : public StmtNode {
public:
  std::string label; // always lowercase
  explicit GotoStmt(std::string l) : label(std::move(l)) {}
  void accept(ASTVisitor *v) override { v->visit(this); }
};

// Gosub labelname
class GosubStmt : public StmtNode {
public:
  std::string label; // always lowercase
  explicit GosubStmt(std::string l) : label(std::move(l)) {}
  void accept(ASTVisitor *v) override { v->visit(this); }
};

// Data 1, 2.5, "x"  — list of literal tokens
class DataStmt : public StmtNode {
public:
  std::vector<Token> values; // INT_LIT, FLOAT_LIT, STRING_LIT
  void accept(ASTVisitor *v) override { v->visit(this); }
};

// Read varname[hint]
class ReadStmt : public StmtNode {
public:
  std::string name;
  std::string typeHint; // % # ! $ or ""
  ReadStmt(std::string n, std::string th)
      : name(std::move(n)), typeHint(std::move(th)) {}
  void accept(ASTVisitor *v) override { v->visit(this); }
};

// Restore [label]
class RestoreStmt : public StmtNode {
public:
  std::string label; // "" = reset to start; otherwise lowercase label name
  explicit RestoreStmt(std::string l = "") : label(std::move(l)) {}
  void accept(ASTVisitor *v) override { v->visit(this); }
};

// Type TypeName ... Field x%, y# ... End Type
class TypeDecl : public StmtNode {
public:
  struct Field {
    std::string name;
    std::string typeHint; // %  #  !  $  or "" (default int handle)
  };
  std::string        name;
  std::vector<Field> fields;
  explicit TypeDecl(std::string n) : name(std::move(n)) {}
  void accept(ASTVisitor *v) override { v->visit(this); }
};

// New TypeName — allocates a type instance and inserts into the global list
class NewExpr : public ExprNode {
public:
  std::string typeName;
  explicit NewExpr(std::string tn) : typeName(std::move(tn)) {}
  void accept(ASTVisitor *v) override { v->visit(this); }
};

// Delete obj — removes the instance from its list and frees it
class DeleteStmt : public StmtNode {
public:
  std::unique_ptr<ExprNode> object;
  explicit DeleteStmt(std::unique_ptr<ExprNode> obj) : object(std::move(obj)) {}
  void accept(ASTVisitor *v) override { v->visit(this); }
};

// obj\field — field read expression (Blitz3D uses \ as field separator)
class FieldAccess : public ExprNode {
public:
  std::unique_ptr<ExprNode> object;
  std::string               fieldName;
  FieldAccess(std::unique_ptr<ExprNode> obj, std::string fn)
      : object(std::move(obj)), fieldName(std::move(fn)) {}
  void accept(ASTVisitor *v) override { v->visit(this); }
};

// obj\field = expr — field write statement
class FieldAssignStmt : public StmtNode {
public:
  std::unique_ptr<ExprNode> object;
  std::string               fieldName;
  std::unique_ptr<ExprNode> value;
  FieldAssignStmt(std::unique_ptr<ExprNode> obj, std::string fn,
                  std::unique_ptr<ExprNode> val)
      : object(std::move(obj)), fieldName(std::move(fn)), value(std::move(val)) {}
  void accept(ASTVisitor *v) override { v->visit(this); }
};

// First TypeName — returns bb_TypeName_head_
class FirstExpr : public ExprNode {
public:
  std::string typeName;
  explicit FirstExpr(std::string tn) : typeName(std::move(tn)) {}
  void accept(ASTVisitor *v) override { v->visit(this); }
};

// Last TypeName — returns bb_TypeName_tail_
class LastExpr : public ExprNode {
public:
  std::string typeName;
  explicit LastExpr(std::string tn) : typeName(std::move(tn)) {}
  void accept(ASTVisitor *v) override { v->visit(this); }
};

// Before(obj) — returns obj->__prev__
class BeforeExpr : public ExprNode {
public:
  std::unique_ptr<ExprNode> object;
  explicit BeforeExpr(std::unique_ptr<ExprNode> obj) : object(std::move(obj)) {}
  void accept(ASTVisitor *v) override { v->visit(this); }
};

// After(obj) — returns obj->__next__
class AfterExpr : public ExprNode {
public:
  std::unique_ptr<ExprNode> object;
  explicit AfterExpr(std::unique_ptr<ExprNode> obj) : object(std::move(obj)) {}
  void accept(ASTVisitor *v) override { v->visit(this); }
};

// Insert obj Before/After target
class InsertStmt : public StmtNode {
public:
  enum Mode { BEFORE, AFTER };
  std::unique_ptr<ExprNode> object;
  Mode                      mode;
  std::unique_ptr<ExprNode> target;
  InsertStmt(std::unique_ptr<ExprNode> obj, Mode m,
             std::unique_ptr<ExprNode> tgt)
      : object(std::move(obj)), mode(m), target(std::move(tgt)) {}
  void accept(ASTVisitor *v) override { v->visit(this); }
};

// For Each var.TypeName ... Next
class ForEachStmt : public StmtNode {
public:
  std::string varName;
  std::string typeName;
  std::vector<std::unique_ptr<ASTNode>> block;
  ForEachStmt(std::string vn, std::string tn)
      : varName(std::move(vn)), typeName(std::move(tn)) {}
  void accept(ASTVisitor *v) override { v->visit(this); }
};

// ---- Root ----

class Program : public ASTNode {
public:
  std::vector<std::unique_ptr<ASTNode>> nodes;
  void accept(ASTVisitor *v) override { v->visit(this); }
};

#endif // BLITZNEXT_AST_H
