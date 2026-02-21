
#ifndef VISITOR_H
#define VISITOR_H

struct Node;
struct ProgNode;
struct StmtSeqNode;
struct StmtNode;
struct ExprStmtNode;
struct DimNode;
struct AssNode;
struct LabelNode;
struct GotoNode;
struct GosubNode;
struct IfNode;
struct ExitNode;
struct WhileNode;
struct ForNode;
struct ForEachNode;
struct ReturnNode;
struct DeleteNode;
struct DeleteEachNode;
struct InsertNode;
struct SelectNode;
struct RepeatNode;
struct ReadNode;
struct RestoreNode;
struct IncludeNode;
struct DeclStmtNode;

struct DeclSeqNode;
struct DeclNode;
struct VarDeclNode;
struct FuncDeclNode;
struct StructDeclNode;
struct DataDeclNode;
struct VectorDeclNode;

struct ExprSeqNode;
struct ExprNode;
struct CastNode;
struct CallNode;
struct VarExprNode;
struct BinExprNode;
struct UniExprNode;
struct ArithExprNode; // Maybe unnecessary if covered by Bin/Uni? But
                      // cpp_generator has it.
struct RelExprNode;
struct NewNode;
struct FirstNode;
struct LastNode;
struct AfterNode;
struct BeforeNode;
struct NullNode;
struct ObjectCastNode;
struct ObjectHandleNode;
struct ConstNode;
struct IntConstNode;
struct FloatConstNode;
struct StringConstNode;

struct VarNode;
struct DeclVarNode;
struct IdentVarNode;
struct ArrayVarNode;
struct FieldVarNode;
struct VectorVarNode;

struct Visitor {
  virtual ~Visitor() {}

  // Program
  virtual void visit(ProgNode *node) = 0;

  // Statements
  virtual void visit(StmtSeqNode *node) = 0;
  virtual void visit(ExprStmtNode *node) = 0;
  virtual void visit(DimNode *node) = 0;
  virtual void visit(AssNode *node) = 0;
  virtual void visit(LabelNode *node) = 0;
  virtual void visit(GotoNode *node) = 0;
  virtual void visit(GosubNode *node) = 0;
  virtual void visit(IfNode *node) = 0;
  virtual void visit(ExitNode *node) = 0;
  virtual void visit(WhileNode *node) = 0;
  virtual void visit(ForNode *node) = 0;
  virtual void visit(ForEachNode *node) = 0;
  virtual void visit(ReturnNode *node) = 0;
  virtual void visit(DeleteNode *node) = 0;
  virtual void visit(DeleteEachNode *node) = 0;
  virtual void visit(InsertNode *node) = 0;
  virtual void visit(SelectNode *node) = 0;
  virtual void visit(RepeatNode *node) = 0;
  virtual void visit(ReadNode *node) = 0;
  virtual void visit(RestoreNode *node) = 0;
  virtual void visit(IncludeNode *node) = 0;
  virtual void visit(DeclStmtNode *node) = 0;

  // Decls
  virtual void visit(DeclSeqNode *node) = 0;
  virtual void visit(VarDeclNode *node) = 0;
  virtual void visit(FuncDeclNode *node) = 0;
  virtual void visit(StructDeclNode *node) = 0;
  virtual void visit(DataDeclNode *node) = 0;
  virtual void visit(VectorDeclNode *node) = 0;

  // Expressions
  virtual void visit(ExprSeqNode *node) = 0;
  virtual void visit(CastNode *node) = 0;
  virtual void visit(CallNode *node) = 0;
  virtual void visit(VarExprNode *node) = 0;
  virtual void visit(BinExprNode *node) = 0;
  virtual void visit(UniExprNode *node) = 0;
  virtual void visit(ArithExprNode *node) = 0;
  virtual void visit(RelExprNode *node) = 0;
  virtual void visit(NewNode *node) = 0;
  virtual void visit(FirstNode *node) = 0;
  virtual void visit(LastNode *node) = 0;
  virtual void visit(AfterNode *node) = 0;
  virtual void visit(BeforeNode *node) = 0;
  virtual void visit(NullNode *node) = 0;
  virtual void visit(ObjectCastNode *node) = 0;
  virtual void visit(ObjectHandleNode *node) = 0;

  // Consts
  virtual void visit(IntConstNode *node) = 0;
  virtual void visit(FloatConstNode *node) = 0;
  virtual void visit(StringConstNode *node) = 0;

  // Vars
  virtual void visit(DeclVarNode *node) = 0;
  virtual void visit(IdentVarNode *node) = 0;
  virtual void visit(ArrayVarNode *node) = 0;
  virtual void visit(FieldVarNode *node) = 0;
  virtual void visit(VectorVarNode *node) = 0;
};

#endif
