
#include "nodes.h"
#include "std.h"

std::set<std::string> Node::usedfuncs;

///////////////////////////////
// generic exception thrower //
///////////////////////////////
void Node::ex() { ex("INTERNAL COMPILER ERROR"); }

void Node::ex(const std::string &e) { throw Ex(e, -1, ""); }

void Node::ex(const std::string &e, int pos) { throw Ex(e, pos, ""); }

void Node::ex(const std::string &e, int pos, const std::string &f) {
  throw Ex(e, pos, f);
}

void Node::semex(const std::string &e) { throw Ex(e, pos, file); }

///////////////////////////////
// Generate a local variable //
///////////////////////////////
VarNode *Node::genLocal(BCEnviron *e, Type *ty) {
  std::string t = genLabel();
  Decl *d = e->decls->insertDecl(t, ty, DECL_LOCAL);
  return d_new<DeclVarNode>(d);
}

/////////////////////////////////////////////////
// if type is const, return const value else 0 //
/////////////////////////////////////////////////
ConstNode *Node::constValue(Type *ty) {
  ConstType *c = ty->constType();
  if (!c)
    return 0;
  ty = c->valueType;
  if (ty == Type::int_type)
    return d_new<IntConstNode>(c->intValue);
  if (ty == Type::float_type)
    return d_new<FloatConstNode>(c->floatValue);
  return d_new<StringConstNode>(c->stringValue);
}

///////////////////////////////////////////////////////
// work out var offsets - return size of local frame //
///////////////////////////////////////////////////////
int Node::enumVars(BCEnviron *e) {
  // calc offsets
  int p_size = 0, l_size = 0;
  int k;
  for (k = 0; k < e->decls->size(); ++k) {
    Decl *d = e->decls->decls[k];
    if (d->kind & DECL_PARAM) {
      d->offset = p_size + 20;
      p_size += 4;
    } else if (d->kind & DECL_LOCAL) {
      d->offset = -4 - l_size;
      l_size += 4;
    }
  }
  return l_size;
}

//////////////////////////////
// initialize all vars to 0 //
//////////////////////////////

/////////////////////////////////
// calculate the type of a tag //
/////////////////////////////////
Type *Node::tagType(const std::string &tag, BCEnviron *e) {
  Type *t;
  if (tag.size()) {
    t = e->findType(tag);
    if (!t)
      ex("Type \"" + tag + "\" not found");
  } else
    t = 0;
  return t;
}

std::string Node::mangle(const std::string &ident, const std::string &tag) {
  if (tag.size() == 0 || tag == "%")
    return ident;
  if (tag == "#")
    return ident + "_f";
  if (tag == "$")
    return ident + "_s";
  if (tag == ".")
    return ident; // This shouldn't happen with the way parseVar is called, but
                  // safety first
  return ident + "_v_" + tag;
}

////////////////////////////////
// Generate a fresh ASM label //
////////////////////////////////
std::string Node::genLabel() {
  static int cnt;
  return "_" + itoa(++cnt & 0x7fffffff);
}

//////////////////////////////////////////////////////
// create a stmt-type function call with int result //
//////////////////////////////////////////////////////
