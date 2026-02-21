
#include "nodes.h"
#include "std.h"
#include "visitor.h"

#include <float.h>
#include <math.h>

//////////////////////////////////
// Cast an expression to a type //
//////////////////////////////////
ExprNode *ExprNode::castTo(Type *ty, BCEnviron *e) {
  if (sem_type == ty)
    return this;
  if (!sem_type->canCastTo(ty)) {
    semex("Illegal type conversion");
  }

  ExprNode *cast = d_new<CastNode>(this, ty);
  return cast->semant(e);
}

ExprNode *CastNode::semant(BCEnviron *e) {
  if (!expr->sem_type) {
    expr = expr->semant(e);
  }

  if (ConstNode *c = expr->constNode()) {
    ExprNode *e;
    if (type == Type::int_type)
      e = d_new<IntConstNode>(c->intValue());
    else if (type == Type::float_type)
      e = d_new<FloatConstNode>(c->floatValue());
    else
      e = d_new<StringConstNode>(c->stringValue());
    return e;
  }

  sem_type = type;
  return this;
}

//////////////////////////////////
// Cast an expression to a type //
//////////////////////////////////

// Sequence of Expressions

void ExprSeqNode::semant(BCEnviron *e) {
  int k;
  for (k = 0; k < exprs.size(); ++k) {
    if (exprs[k])
      exprs[k] = exprs[k]->semant(e);
  }
}

void ExprSeqNode::castTo(DeclSeq *decls, BCEnviron *e, bool cfunc) {
  if (exprs.size() > decls->size())
    semex("Too many parameters");
  int k;
  for (k = 0; k < decls->size(); ++k) {
    Decl *d = decls->decls[k];
    if (k < exprs.size() && exprs[k]) {

      if (cfunc && d->type->structType()) {
        if (exprs[k]->sem_type->structType()) {
        } else if (exprs[k]->sem_type->intType()) {
          exprs[k]->sem_type = Type::void_type;
        } else {
          semex("Illegal type conversion");
        }
        continue;
      }

      exprs[k] = exprs[k]->castTo(d->type, e);

    } else {
      if (!d->defType)
        semex("Not enough parameters");
      ExprNode *expr = constValue(d->defType);
      if (k < exprs.size())
        exprs[k] = expr;
      else
        exprs.push_back(expr);
    }
  }
}

void ExprSeqNode::castTo(Type *t, BCEnviron *e) {
  int k;
  for (k = 0; k < exprs.size(); ++k) {
    exprs[k] = exprs[k]->castTo(t, e);
  }
}

///////////////////
// Function call //
///////////////////
ExprNode *CallNode::semant(BCEnviron *e) {
  Type *retType = e->findType(tag);

  // Semant arguments first to know their types
  exprs->semant(e);

  std::vector<Decl *> funcs = e->findFunctions(ident);

  if (funcs.empty()) {
    // Create a dummy function decl to allow proceeding
    FuncType *ft = d_new<FuncType>(Type::int_type, nullptr, false, false);
    sem_decl = e->funcDecls->insertDecl(ident, Type::int_type, DECL_FUNC);
    sem_decl->type = ft;
    funcs.push_back(sem_decl);
  }

  Decl *bestDecl = 0;
  int bestScore = -1;

  for (int k = 0; k < funcs.size(); ++k) {
    Decl *d = funcs[k];
    if (!(d->kind & DECL_FUNC))
      continue;
    FuncType *f = d->type->funcType();
    if (!f)
      continue;

    // Filter by return type if tag is present
    if (retType && f->returnType != retType) {
      continue;
    }

    // Check param count
    int paramCount = f->params ? f->params->size() : 0;
    int argCount = exprs->exprs.size();
    int minParams = 0;
    if (f->params) {
      for (int i = 0; i < f->params->size(); ++i) {
        if (!f->params->decls[i]->defType)
          minParams++;
      }
    }

    if (argCount < minParams || argCount > paramCount)
      continue;

    // Score arguments
    int currentScore = 1000;
    if (f->params && argCount > 0) {
      for (int i = 0; i < argCount; ++i) {
        Type *argType = exprs->exprs[i]->sem_type;
        Type *paramType = f->params->decls[i]->type;

        if (argType == paramType) {
          // Perfect match
        } else if (argType->canCastTo(paramType)) {
          // Castable (Int->Float etc) - penalty
          currentScore -= 10;
        } else {
          // No match
          currentScore = -1;
          break;
        }
      }
    }

    if (currentScore > bestScore) {
      bestScore = currentScore;
      bestDecl = d;
    }
  }

  if (!bestDecl) {
    std::string sigs = "";
    for (auto d : funcs)
      sigs += d->name + " ";
    semex("Function '" + ident + "' signature not found. Candidates: " + sigs);
  }

  sem_decl = bestDecl;
  FuncType *f = sem_decl->type->funcType();

  // Now apply the casts
  if (f->params)
    exprs->castTo(f->params, e, f->cfunc);

  sem_type = f->returnType;
  return this;
}

/////////////////////////
// Variable expression //
/////////////////////////
ExprNode *VarExprNode::semant(BCEnviron *e) {
  var->semant(e);
  sem_type = var->sem_type;
  ConstType *c = sem_type->constType();
  if (!c)
    return this;
  ExprNode *expr = constValue(c);
  return expr;
}

//////////////////////
// Integer constant //
//////////////////////
IntConstNode::IntConstNode(int n) : value(n) { sem_type = Type::int_type; }

int IntConstNode::intValue() { return value; }

float IntConstNode::floatValue() { return value; }

std::string IntConstNode::stringValue() { return itoa(value); }

////////////////////
// Float constant //
////////////////////
FloatConstNode::FloatConstNode(float f) : value(f) {
  sem_type = Type::float_type;
}

// Fix: Blitz3D Int() truncates, lrintf rounds.
int FloatConstNode::intValue() { return (int)value; }

float FloatConstNode::floatValue() { return value; }

std::string FloatConstNode::stringValue() { return ftoa(value); }

/////////////////////
// String constant //
/////////////////////
StringConstNode::StringConstNode(const std::string &s) : value(s) {
  sem_type = Type::string_type;
}

int StringConstNode::intValue() { return atoi(value); }

float StringConstNode::floatValue() { return (float)atof(value); }

std::string StringConstNode::stringValue() { return value; }

////////////////////
// Unary operator //
////////////////////
ExprNode *UniExprNode::semant(BCEnviron *e) {
  expr = expr->semant(e);
  sem_type = expr->sem_type;
  if (sem_type != Type::int_type && sem_type != Type::float_type)
    semex("Illegal operator for type");
  if (ConstNode *c = expr->constNode()) {
    ExprNode *e;
    if (sem_type == Type::int_type) {
      switch (op) {
      case '+':
        e = d_new<IntConstNode>(+c->intValue());
        break;
      case '-':
        e = d_new<IntConstNode>(-c->intValue());
        break;
      case ABS:
        e = d_new<IntConstNode>(c->intValue() >= 0 ? c->intValue()
                                                   : -c->intValue());
        break;
      case SGN:
        e = d_new<IntConstNode>(
            c->intValue() > 0 ? 1 : (c->intValue() < 0 ? -1 : 0));
        break;
      }
    } else {
      switch (op) {
      case '+':
        e = d_new<FloatConstNode>(+c->floatValue());
        break;
      case '-':
        e = d_new<FloatConstNode>(-c->floatValue());
        break;
      case ABS:
        e = d_new<FloatConstNode>(c->floatValue() >= 0 ? c->floatValue()
                                                       : -c->floatValue());
        break;
      case SGN:
        e = d_new<FloatConstNode>(
            c->floatValue() > 0 ? 1 : (c->floatValue() < 0 ? -1 : 0));
        break;
      }
    }
    return e;
  }
  return this;
}

/////////////////////////////////////////////////////
// boolean expression - accepts ints, returns ints //
/////////////////////////////////////////////////////
ExprNode *BinExprNode::semant(BCEnviron *e) {
  lhs = lhs->semant(e);
  lhs = lhs->castTo(Type::int_type, e);
  rhs = rhs->semant(e);
  rhs = rhs->castTo(Type::int_type, e);
  ConstNode *lc = lhs->constNode(), *rc = rhs->constNode();
  if (lc && rc) {
    ExprNode *expr;
    switch (op) {
    case AND:
      expr = d_new<IntConstNode>(lc->intValue() & rc->intValue());
      break;
    case OR:
      expr = d_new<IntConstNode>(lc->intValue() | rc->intValue());
      break;
    case XOR:
      expr = d_new<IntConstNode>(lc->intValue() ^ rc->intValue());
      break;
    case SHL:
      expr = d_new<IntConstNode>(lc->intValue() << rc->intValue());
      break;
    case SHR:
      expr = d_new<IntConstNode>((unsigned)lc->intValue() >> rc->intValue());
      break;
    case SAR:
      expr = d_new<IntConstNode>(lc->intValue() >> rc->intValue());
      break;
    }
    return expr;
  }
  sem_type = Type::int_type;
  return this;
}

///////////////////////////
// arithmetic expression //
///////////////////////////
ExprNode *ArithExprNode::semant(BCEnviron *e) {
  std::cerr << "Debug: ArithExprNode::semant starting" << std::endl;
  lhs = lhs->semant(e);
  rhs = rhs->semant(e);
  std::cerr << "Debug: ArithExprNode::semant - lhs type: "
            << lhs->sem_type->intType()
            << ", rhs type: " << rhs->sem_type->intType() << std::endl;
  if (lhs->sem_type->structType() || rhs->sem_type->structType()) {
    semex("Arithmetic operator cannot be applied to custom type objects");
  }
  if (lhs->sem_type == Type::string_type ||
      rhs->sem_type == Type::string_type) {
    // one side is a string - only + operator...
    if (op != '+')
      semex("Operator cannot be applied to strings");
    sem_type = Type::string_type;
  } else if (op == '^' || lhs->sem_type == Type::float_type ||
             rhs->sem_type == Type::float_type) {
    // It's ^, or one side is a float
    sem_type = Type::float_type;
  } else {
    // must be 2 ints
    sem_type = Type::int_type;
  }
  lhs = lhs->castTo(sem_type, e);
  rhs = rhs->castTo(sem_type, e);
  ConstNode *lc = lhs->constNode(), *rc = rhs->constNode();
  if (rc && (op == '/' || op == MOD)) {
    if ((sem_type == Type::int_type && !rc->intValue()) ||
        (sem_type == Type::float_type && !rc->floatValue())) {
      semex("Division by zero");
    }
  }
  if (lc && rc) {
    ExprNode *expr;
    if (sem_type == Type::string_type) {
      expr = d_new<StringConstNode>(lc->stringValue() + rc->stringValue());
    } else if (sem_type == Type::int_type) {
      switch (op) {
      case '+':
        expr = d_new<IntConstNode>(lc->intValue() + rc->intValue());
        break;
      case '-':
        expr = d_new<IntConstNode>(lc->intValue() - rc->intValue());
        break;
      case '*':
        expr = d_new<IntConstNode>(lc->intValue() * rc->intValue());
        break;
      case '/':
        expr = d_new<IntConstNode>(lc->intValue() / rc->intValue());
        break;
      case MOD:
        expr = d_new<IntConstNode>(lc->intValue() % rc->intValue());
        break;
      }
    } else {
      switch (op) {
      case '+':
        expr = d_new<FloatConstNode>(lc->floatValue() + rc->floatValue());
        break;
      case '-':
        expr = d_new<FloatConstNode>(lc->floatValue() - rc->floatValue());
        break;
      case '*':
        expr = d_new<FloatConstNode>(lc->floatValue() * rc->floatValue());
        break;
      case '/':
        expr = d_new<FloatConstNode>(lc->floatValue() / rc->floatValue());
        break;
      case MOD:
        expr = d_new<FloatConstNode>(fmod(lc->floatValue(), rc->floatValue()));
        break;
      case '^':
        expr = d_new<FloatConstNode>(pow(lc->floatValue(), rc->floatValue()));
        break;
      }
    }
    return expr;
  }
  return this;
}

/////////////////////////
// relation expression //
/////////////////////////
ExprNode *RelExprNode::semant(BCEnviron *e) {
  lhs = lhs->semant(e);
  rhs = rhs->semant(e);
  if (lhs->sem_type->structType() || rhs->sem_type->structType()) {
    if (op != '=' && op != NE)
      semex("Illegal operator for custom type objects");
    opType = lhs->sem_type != Type::null_type ? lhs->sem_type : rhs->sem_type;
  } else if (lhs->sem_type == Type::string_type ||
             rhs->sem_type == Type::string_type) {
    opType = Type::string_type;
  } else if (lhs->sem_type == Type::float_type ||
             rhs->sem_type == Type::float_type) {
    opType = Type::float_type;
  } else {
    opType = Type::int_type;
  }
  sem_type = Type::int_type;
  lhs = lhs->castTo(opType, e);
  rhs = rhs->castTo(opType, e);
  ConstNode *lc = lhs->constNode(), *rc = rhs->constNode();
  if (lc && rc) {
    ExprNode *expr;
    if (opType == Type::string_type) {
      switch (op) {
      case '<':
        expr = d_new<IntConstNode>(lc->stringValue() < rc->stringValue());
        break;
      case '=':
        expr = d_new<IntConstNode>(lc->stringValue() == rc->stringValue());
        break;
      case '>':
        expr = d_new<IntConstNode>(lc->stringValue() > rc->stringValue());
        break;
      case LE:
        expr = d_new<IntConstNode>(lc->stringValue() <= rc->stringValue());
        break;
      case NE:
        expr = d_new<IntConstNode>(lc->stringValue() != rc->stringValue());
        break;
      case GE:
        expr = d_new<IntConstNode>(lc->stringValue() >= rc->stringValue());
        break;
      }
    } else if (opType == Type::float_type) {
      switch (op) {
      case '<':
        expr = d_new<IntConstNode>(lc->floatValue() < rc->floatValue());
        break;
      case '=':
        expr = d_new<IntConstNode>(lc->floatValue() == rc->floatValue());
        break;
      case '>':
        expr = d_new<IntConstNode>(lc->floatValue() > rc->floatValue());
        break;
      case LE:
        expr = d_new<IntConstNode>(lc->floatValue() <= rc->floatValue());
        break;
      case NE:
        expr = d_new<IntConstNode>(lc->floatValue() != rc->floatValue());
        break;
      case GE:
        expr = d_new<IntConstNode>(lc->floatValue() >= rc->floatValue());
        break;
      }
    } else {
      switch (op) {
      case '<':
        expr = d_new<IntConstNode>(lc->intValue() < rc->intValue());
        break;
      case '=':
        expr = d_new<IntConstNode>(lc->intValue() == rc->intValue());
        break;
      case '>':
        expr = d_new<IntConstNode>(lc->intValue() > rc->intValue());
        break;
      case LE:
        expr = d_new<IntConstNode>(lc->intValue() <= rc->intValue());
        break;
      case NE:
        expr = d_new<IntConstNode>(lc->intValue() != rc->intValue());
        break;
      case GE:
        expr = d_new<IntConstNode>(lc->intValue() >= rc->intValue());
        break;
      }
    }
    return expr;
  }
  return this;
}

////////////////////
// New expression //
////////////////////
ExprNode *NewNode::semant(BCEnviron *e) {
  sem_type = e->findType(ident);
  if (!sem_type)
    semex("custom type name not found");
  if (sem_type->structType() == 0)
    semex("type is not a custom type");
  return this;
}

////////////////////
// First of class //
////////////////////
ExprNode *FirstNode::semant(BCEnviron *e) {
  sem_type = e->findType(ident);
  if (!sem_type)
    semex("custom type name name not found");
  return this;
}

///////////////////
// Last of class //
///////////////////
ExprNode *LastNode::semant(BCEnviron *e) {
  sem_type = e->findType(ident);
  if (!sem_type)
    semex("custom type name not found");
  return this;
}

////////////////////
// Next of object //
////////////////////
ExprNode *AfterNode::semant(BCEnviron *e) {
  expr = expr->semant(e);
  if (expr->sem_type == Type::null_type)
    semex("'After' cannot be used on 'Null'");
  if (expr->sem_type->structType() == 0)
    semex("'After' must be used with a custom type object");
  sem_type = expr->sem_type;
  return this;
}

////////////////////
// Prev of object //
////////////////////
ExprNode *BeforeNode::semant(BCEnviron *e) {
  expr = expr->semant(e);
  if (expr->sem_type == Type::null_type)
    semex("'Before' cannot be used with 'Null'");
  if (expr->sem_type->structType() == 0)
    semex("'Before' must be used with a custom type object");
  sem_type = expr->sem_type;
  return this;
}

/////////////////
// Null object //
/////////////////
ExprNode *NullNode::semant(BCEnviron *e) {
  sem_type = Type::null_type;
  return this;
}

/////////////////
// Object cast //
/////////////////
ExprNode *ObjectCastNode::semant(BCEnviron *e) {
  expr = expr->semant(e);
  expr = expr->castTo(Type::int_type, e);
  sem_type = e->findType(type_ident);
  if (!sem_type)
    semex("custom type name not found");
  if (!sem_type->structType())
    semex("type is not a custom type");
  return this;
}

///////////////////
// Object Handle //
///////////////////
ExprNode *ObjectHandleNode::semant(BCEnviron *e) {
  expr = expr->semant(e);
  if (!expr->sem_type->structType())
    semex("'ObjectHandle' must be used with an object");
  sem_type = Type::int_type;
  return this;
}

void ExprSeqNode::accept(Visitor *v) { v->visit(this); }
void CastNode::accept(Visitor *v) { v->visit(this); }
void CallNode::accept(Visitor *v) { v->visit(this); }
void VarExprNode::accept(Visitor *v) { v->visit(this); }
void IntConstNode::accept(Visitor *v) { v->visit(this); }
void FloatConstNode::accept(Visitor *v) { v->visit(this); }
void StringConstNode::accept(Visitor *v) { v->visit(this); }
void UniExprNode::accept(Visitor *v) { v->visit(this); }
void BinExprNode::accept(Visitor *v) { v->visit(this); }
void ArithExprNode::accept(Visitor *v) { v->visit(this); }
void RelExprNode::accept(Visitor *v) { v->visit(this); }
void NewNode::accept(Visitor *v) { v->visit(this); }
void FirstNode::accept(Visitor *v) { v->visit(this); }
void LastNode::accept(Visitor *v) { v->visit(this); }
void AfterNode::accept(Visitor *v) { v->visit(this); }
void BeforeNode::accept(Visitor *v) { v->visit(this); }
void NullNode::accept(Visitor *v) { v->visit(this); }
void ObjectCastNode::accept(Visitor *v) { v->visit(this); }
void ObjectHandleNode::accept(Visitor *v) { v->visit(this); }
