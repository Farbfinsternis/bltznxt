#ifndef CPP_GENERATOR_H
#define CPP_GENERATOR_H

#include "codegen.h"
#include "visitor.h"
#include <iostream>
#include <string>
#include <vector>

// Forward declarations for AST nodes
struct Node;
struct Type;
struct Decl;
struct BCEnviron;
struct ProgNode;
struct StmtSeqNode;
struct StmtNode;
struct ExprStmtNode;
struct CallNode;
struct ExprNode;
struct StringConstNode;
struct IntConstNode;
struct FloatConstNode;
struct ForNode;
struct BinExprNode;
struct UniExprNode;
struct ArithExprNode;
struct RelExprNode;
struct CastNode;
struct VarExprNode;
struct VarNode;
struct IdentVarNode;
struct AssNode;
struct WhileNode;
struct IfNode;
struct ExitNode;
struct ReturnNode;
struct SelectNode;
struct CaseNode;
struct GosubNode;
struct LabelNode;
struct RepeatNode;
struct IncludeNode;
struct ExprSeqNode;
struct FuncDeclNode;
struct DeclStmtNode;
struct VarDeclNode;
struct DimNode;
struct ArrayVarNode;
struct StructDeclNode;
struct FieldVarNode;
struct NewNode;
struct FirstNode;
struct LastNode;
struct AfterNode;
struct BeforeNode;
struct NullNode;
struct ForEachNode;
struct DeleteNode;
struct DeleteEachNode;
struct ObjectCastNode;
struct ObjectHandleNode;
struct ReadNode;
struct RestoreNode;
struct DataDeclNode;
struct InsertNode;

class CppGenerator : public Codegen, public Visitor {
public:
  CppGenerator(std::ostream &out);

  // AST Dispatcher
  void generate(Node *node);

  // AST Visitors
  void visit(ProgNode *node) override;
  void visit(StmtSeqNode *node) override;
  void visit(StmtNode *node); // Helper, not in Visitor
  void visit(ExprStmtNode *node) override;
  void visit(CallNode *node) override;
  void visit(ExprNode *node); // Helper, not in Visitor
  void visit(StringConstNode *node) override;
  void visit(IntConstNode *node) override;
  void visit(FloatConstNode *node) override;
  void visit(ForNode *node) override;
  void visit(BinExprNode *node) override;
  void visit(UniExprNode *node) override;
  void visit(ArithExprNode *node) override;
  void visit(RelExprNode *node) override;
  void visit(CastNode *node) override;
  void visit(VarExprNode *node) override;
  void visit(VarNode *node); // Helper, not in Visitor
  void visit(IdentVarNode *node) override;
  void visit(DeclVarNode *node) override;
  void visit(VectorVarNode *node) override;
  void visit(AssNode *node) override;
  void visit(WhileNode *node) override;
  void visit(IfNode *node) override;
  void visit(ExitNode *node) override;
  void visit(ReturnNode *node) override;
  void visit(SelectNode *node) override;
  void visit(GosubNode *node) override;
  void visit(GotoNode *node) override;
  void visit(LabelNode *node) override;
  void visit(RepeatNode *node) override;
  void visit(IncludeNode *node) override;
  void visit(FuncDeclNode *node) override;
  void visit(DeclStmtNode *node) override;
  void visit(VarDeclNode *node) override;
  void visit(DimNode *node) override;
  void visit(ArrayVarNode *node) override;
  void visit(ExprSeqNode *node) override;
  void visit(DeclSeqNode *node) override;
  void visit(StructDeclNode *node) override;
  void visit(FieldVarNode *node) override;
  void visit(NewNode *node) override;
  void visit(FirstNode *node) override;
  void visit(LastNode *node) override;
  void visit(AfterNode *node) override;
  void visit(BeforeNode *node) override;
  void visit(NullNode *node) override;
  void visit(ForEachNode *node) override;
  void visit(DeleteNode *node) override;
  void visit(DeleteEachNode *node) override;
  void visit(ObjectCastNode *node) override;
  void visit(ObjectHandleNode *node) override;
  void visit(ReadNode *node) override;
  void visit(RestoreNode *node) override;
  void visit(DataDeclNode *node) override;
  void visit(VectorDeclNode *node) override;
  void visit(InsertNode *node) override;

private:
  void emit(const std::string &s);
  void emitln(const std::string &s);

  void emitVarDecls(BCEnviron *env, int kind);
  void emitVarDecl(Decl *d);
  void emitReturnLogic();

  std::string mapType(Type *t);

  std::string indent;
  void incIndent() { indent += "    "; }
  void decIndent() {
    if (indent.length() >= 4)
      indent = indent.substr(0, indent.length() - 4);
  }

  int next_gosub_id;
  std::vector<int> current_gosub_ids;
  bool in_function;
};

#endif
