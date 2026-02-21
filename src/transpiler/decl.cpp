
#include "decl.h"
#include "std.h"
#include "type.h"
#include <cstring> // For memcpy

Decl::~Decl() {}

DeclSeq::DeclSeq() {}

void Decl::getName(char *buff) {
  int sz = name.size();
  std::memcpy(buff, name.data(), sz);
  buff[sz] = 0;
}

DeclSeq::~DeclSeq() {
  /*for (; decls.size(); decls.pop_back())
    delete decls.back();*/
}

Decl *DeclSeq::findDecl(const std::string &s) {
  std::vector<Decl *>::iterator it;
  for (it = decls.begin(); it != decls.end(); ++it) {
    if ((*it)->name == s)
      return *it;
  }
  return 0;
}

std::vector<Decl *> DeclSeq::findDecls(const std::string &s) {
  std::vector<Decl *> out;
  std::vector<Decl *>::iterator it;
  for (it = decls.begin(); it != decls.end(); ++it) {
    if ((*it)->name == s)
      out.push_back(*it);
  }
  return out;
}

Decl *DeclSeq::insertDecl(const std::string &s, Type *t, int kind,
                          ConstType *d) {
  if (kind != DECL_FUNC && findDecl(s))
    return 0;
  decls.push_back(d_new<Decl>(s, t, kind, d));
  return decls.back();
}
