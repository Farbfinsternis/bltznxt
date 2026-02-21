
/*

  The parser builds an abstact syntax tree from input tokens.

*/

#ifndef PARSER_H
#define PARSER_H

#include "nodes.h"
#include "toker.h"

class Parser {
public:
  Parser(Toker &t);

  ProgNode *parse(const std::string &main);

  template <class T, class... Args> T *createNode(int p, Args &&...args) {
    T *n = d_new<T>(std::forward<Args>(args)...);
    n->pos = p;
    n->file = incfile;
    return n;
  }

  template <class T> T *setPos(T *n, int p) {
    if (n) {
      n->pos = p;
      n->file = incfile;
    }
    return n;
  }

private:
  std::string incfile;
  std::string mainDir; // Directory of the main source file
  std::set<std::string> included;
  Toker *toker, *main_toker;
  std::map<std::string, DimNode *> arrayDecls;

  DeclSeqNode *consts;
  DeclSeqNode *structs;
  DeclSeqNode *funcs;
  DeclSeqNode *datas;

  StmtSeqNode *parseStmtSeq(int scope);
  void parseStmtSeq(StmtSeqNode *stmts, int scope);

  void ex(const std::string &s);
  void exp(const std::string &s);

  std::string parseIdent();
  void parseChar(int c);
  std::string parseTypeTag();

  VarNode *parseVar();
  VarNode *parseVar(const std::string &ident, const std::string &tag);
  CallNode *parseCall(const std::string &ident, const std::string &tag);
  IfNode *parseIf();

  DeclNode *parseVarDecl(int kind, bool constant);
  DimNode *parseArrayDecl();
  DeclNode *parseFuncDecl();
  DeclNode *parseMethodDecl();
  DeclNode *parseStructDecl();

  ExprSeqNode *parseExprSeq();

  ExprNode *parseExpr(bool opt);
  ExprNode *parseExpr1(bool opt);   // And, Or, Eor
  ExprNode *parseExpr2(bool opt);   //<,=,>,<=,<>,>=
  ExprNode *parseExpr3(bool opt);   //+,-
  ExprNode *parseExpr4(bool opt);   // Lsr,Lsr,Asr
  ExprNode *parseExpr5(bool opt);   //*,/,Mod
  ExprNode *parseExpr6(bool opt);   //^
  ExprNode *parseUniExpr(bool opt); //+,-,Not,~
  ExprNode *parsePrimary(bool opt);
};

#endif
