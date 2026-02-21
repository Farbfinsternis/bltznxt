#include "environ.h"
#include "std.h"
#include <list>
#include <vector>

BCEnviron::BCEnviron(const std::string &f, Type *r, int l, BCEnviron *gs)
    : funcLabel(f), returnType(r), level(l), globals(gs), typeHint(0) {
  decls = d_new<DeclSeq>();
  typeDecls = d_new<DeclSeq>();
  funcDecls = d_new<DeclSeq>();
  if (globals)
    globals->children.push_back(this);
}

BCEnviron::~BCEnviron() {
  // std::cout << "BCEnviron destructor " << this << std::endl;
  /*if (globals)
    globals->children.remove(this);*/
  /*while (children.size())
    delete children.back();
  for (; labels.size(); labels.pop_back())
    delete labels.back();

  // delete all types
  delete decls;
  delete funcDecls;
  delete typeDecls;

  int k;
  for (k = 0; k < types.size(); ++k)
    delete types[k];*/
}

Decl *BCEnviron::findDecl(const std::string &s) {
  for (BCEnviron *e = this; e; e = e->globals) {
    if (Decl *d = e->decls->findDecl(s)) {
      if (d->kind & (DECL_LOCAL | DECL_PARAM)) {
        if (e == this)
          return d;
      } else
        return d;
    }
  }
  return 0;
}

Decl *BCEnviron::findFunc(const std::string &s) {
  for (BCEnviron *e = this; e; e = e->globals) {
    if (Decl *d = e->funcDecls->findDecl(s))
      return d;
  }
  return 0;
}

std::vector<Decl *> BCEnviron::findFunctions(const std::string &s) {
  std::vector<Decl *> out;
  std::vector<std::string> names = {s, s + "_f", s + "_s"};
  for (BCEnviron *e = this; e; e = e->globals) {
    for (const auto &name : names) {
      std::vector<Decl *> d = e->funcDecls->findDecls(name);
      out.insert(out.end(), d.begin(), d.end());
    }
  }
  return out;
}

Type *BCEnviron::findType(const std::string &s) {
  if (s == "%")
    return Type::int_type;
  if (s == "#")
    return Type::float_type;
  if (s == "$")
    return Type::string_type;
  for (BCEnviron *e = this; e; e = e->globals) {
    if (Decl *d = e->typeDecls->findDecl(s))
      return d->type->structType();
  }
  return 0;
}

Label *BCEnviron::findLabel(const std::string &s) {
  int k;
  for (k = 0; k < labels.size(); ++k)
    if (labels[k]->name == s)
      return labels[k];
  return 0;
}

Label *BCEnviron::insertLabel(const std::string &s, int def, int src, int sz) {
  Label *l = d_new<Label>(s, def, src, sz);
  labels.push_back(l);
  return l;
}

std::string BCEnviron::setBreak(const std::string &s) {
  std::string t = breakLabel;
  breakLabel = s;
  return t;
}
