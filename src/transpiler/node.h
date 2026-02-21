
#ifndef NODE_H
#define NODE_H

#include "environ.h"
#include "ex.h"
#include "std.h"
#include "toker.h"

struct VarNode;
struct ConstNode;

struct Visitor;

struct Node {
  int pos;
  std::string file;
  Node() : pos(-1) {}
  virtual void accept(Visitor *v) = 0;

  Node *set(int p, const std::string &f) {
    pos = p;
    file = f;
    return this;
  }

  void semex(const std::string &e);

  // used user funcs...
  static std::set<std::string> usedfuncs;

  // helper funcs
  static void ex();
  static void ex(const std::string &e);
  static void ex(const std::string &e, int pos);
  static void ex(const std::string &e, int pos, const std::string &f);

  static ConstNode *constValue(Type *ty);
  static std::string genLabel();
  static VarNode *genLocal(BCEnviron *e, Type *ty);

  static int enumVars(BCEnviron *e);
  static Type *tagType(const std::string &s, BCEnviron *e);
  static std::string mangle(const std::string &ident, const std::string &tag);
};

#endif
