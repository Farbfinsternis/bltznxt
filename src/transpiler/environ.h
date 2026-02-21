
/*

  An environ represent a stack frame block.

  */

#ifndef ENVIRON_H
#define ENVIRON_H

#include "decl.h"
#include "label.h"
#include "type.h"
#include <list>
#include <vector>

class BCEnviron {
public:
  int level;
  DeclSeq *decls;
  DeclSeq *funcDecls;
  DeclSeq *typeDecls;

  std::vector<Type *> types;

  std::vector<Label *> labels;
  BCEnviron *globals;
  Type *returnType;
  StructType *currStruct; // Current struct for methods
  std::string funcLabel, breakLabel;
  std::list<BCEnviron *> children; // for delete!

  BCEnviron(const std::string &f, Type *r, int l, BCEnviron *gs);
  ~BCEnviron();

  Decl *findDecl(const std::string &s);
  Decl *findFunc(const std::string &s);
  std::vector<Decl *> findFunctions(const std::string &s);
  Type *findType(const std::string &s);
  Label *findLabel(const std::string &s);
  Label *insertLabel(const std::string &s, int def, int src, int sz);

  Type *typeHint; // Added for auto-type promotion
  std::string setBreak(const std::string &s);
};

#endif
