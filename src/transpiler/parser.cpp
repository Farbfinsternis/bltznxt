#include "parser.h"
#include "std.h"
#include <cstdlib>
#include <filesystem>

enum { STMTS_PROG, STMTS_BLOCK, STMTS_LINE };

static bool isTerm(int c) { return c == ':' || c == '\n'; }

Parser::Parser(Toker &t) : toker(&t), main_toker(&t) {}

ProgNode *Parser::parse(const std::string &main) {

  incfile = main;
  // Store the main file's directory for Include resolution
  try {
    mainDir = std::filesystem::path(std::filesystem::absolute(main))
                  .parent_path()
                  .string();
  } catch (...) {
    mainDir = ".";
  }

  consts = d_new<DeclSeqNode>();
  structs = d_new<DeclSeqNode>();
  funcs = d_new<DeclSeqNode>();
  datas = d_new<DeclSeqNode>();
  StmtSeqNode *stmts = 0;

  try {
    stmts = parseStmtSeq(STMTS_PROG);
    if (toker->curr() != EOF)
      exp("end-of-file");
  } catch (Ex) {
    /*delete stmts;
    delete datas;
    delete funcs;
    delete structs;
    delete consts;*/
    throw;
  }

  return d_new<ProgNode>(consts, structs, funcs, datas, stmts);
}

void Parser::ex(const std::string &s) { throw Ex(s, toker->pos(), incfile); }

void Parser::exp(const std::string &s) {
  switch (toker->curr()) {
  case NEXT:
    ex("'Next' without 'For'");
  case WEND:
    ex("'Wend' without 'While'");
  case ELSE:
  case ELSEIF:
    ex("'Else' without 'If'");
  case ENDIF:
    ex("'Endif' without 'If'");
  case ENDFUNCTION:
    ex("'End Function' without 'Function'");
  case UNTIL:
    ex("'Until' without 'Repeat'");
  case FOREVER:
    ex("'Forever' without 'Repeat'");
  case CASE:
    ex("'Case' without 'Select'");
  case ENDSELECT:
    ex("'End Select' without 'Select'");
  }
  ex("Expecting " + s);
}

std::string Parser::parseIdent() {
  if (toker->curr() != IDENT)
    exp("identifier");
  std::string t = toker->text();
  toker->next();
  return t;
}

void Parser::parseChar(int c) {
  if (toker->curr() != c)
    exp(std::string("'") + char(c) + std::string("'"));
  toker->next();
}

StmtSeqNode *Parser::parseStmtSeq(int scope) {
  a_ptr<StmtSeqNode> stmts(createNode<StmtSeqNode>(toker->pos(), incfile));
  parseStmtSeq(stmts, scope);
  return stmts.release();
}

void Parser::parseStmtSeq(StmtSeqNode *stmts, int scope) {

  for (;;) {
    while (toker->curr() == ':' ||
           (scope != STMTS_LINE && toker->curr() == '\n'))
      toker->next();
    StmtNode *result = 0;

    int pos = toker->pos();

    switch (toker->curr()) {
    case INCLUDE: {
      if (toker->next() != STRINGCONST)
        exp("include filename");
      std::string inc = toker->text();
      toker->next();
      inc = inc.substr(1, inc.size() - 2);

      // Resolve include path relative to the main source file's directory
      // (Blitz3D resolves includes relative to the main .bb file)
      try {
        std::filesystem::path incPath(inc);
        if (incPath.is_relative()) {
          incPath = std::filesystem::path(mainDir) / incPath;
        }
        inc = std::filesystem::absolute(incPath).string();
      } catch (...) {
      }
      inc = tolower(inc);

      if (included.find(inc) != included.end())
        break;

      std::ifstream i_stream(inc.c_str());
      if (!i_stream.good())
        ex("Unable to open include file");

      Toker i_toker(i_stream);

      std::string t_inc = incfile;
      incfile = inc;
      Toker *t_toker = toker;
      toker = &i_toker;

      included.insert(incfile);

      a_ptr<StmtSeqNode> ss(parseStmtSeq(scope));
      if (toker->curr() != EOF)
        exp("end-of-file");

      result = createNode<IncludeNode>(pos, incfile, ss.release());

      toker = t_toker;
      incfile = t_inc;
    } break;
    case SELF:
    case IDENT: {
      int pos = toker->pos();
      int c = toker->curr();
      std::string ident = toker->text();
      toker->next();
      std::string tag = parseTypeTag();
      if (c == IDENT && arrayDecls.find(ident) == arrayDecls.end() &&
          toker->curr() != '=' && toker->curr() != '\\' &&
          toker->curr() != '[') {
        // must be a function
        ExprSeqNode *exprs;
        if (toker->curr() == '(') {
          // ugly lookahead for optional '()' around statement params
          int nest = 1, k;
          for (k = 1;; ++k) {
            int c = toker->lookAhead(k);
            if (isTerm(c))
              ex("Mismatched brackets");
            else if (c == '(')
              ++nest;
            else if (c == ')' && !--nest)
              break;
          }
          if (isTerm(toker->lookAhead(++k))) {
            toker->next();
            exprs = parseExprSeq();
            if (toker->curr() != ')')
              exp("')'");
            toker->next();
          } else
            exprs = parseExprSeq();
        } else
          exprs = parseExprSeq();
        CallNode *call = createNode<CallNode>(pos, ident, tag, exprs);
        result = createNode<ExprStmtNode>(pos, call);
      } else {
        // must be a var or method call
        VarNode *var = nullptr;
        if (c == SELF) {
          if (toker->curr() != '\\' && toker->curr() != '[') {
            ex("'Self' cannot be assigned to");
          }
          ExprNode *expr = createNode<SelfNode>(pos);
          if (toker->curr() == '\\') {
            toker->next();
            std::string f_ident = parseIdent();
            std::string f_tag = parseTypeTag();
            var = createNode<FieldVarNode>(pos, expr, f_ident, f_tag);
          } else {
            toker->next();
            a_ptr<ExprSeqNode> exprs(parseExprSeq());
            if (exprs->size() != 1 || toker->curr() != ']')
              exp("']'");
            toker->next();
            var = createNode<VectorVarNode>(pos, expr, exprs.release());
          }
          // Handle further trailers
          for (;;) {
            if (toker->curr() == '\\') {
              toker->next();
              std::string ident = parseIdent();
              std::string tag = parseTypeTag();
              ExprNode *expr = d_new<VarExprNode>(var);
              var = d_new<FieldVarNode>(expr, ident, tag);
            } else if (toker->curr() == '[') {
              toker->next();
              a_ptr<ExprSeqNode> exprs(parseExprSeq());
              if (exprs->size() != 1 || toker->curr() != ']')
                exp("']'");
              toker->next();
              ExprNode *expr = d_new<VarExprNode>(var);
              var = d_new<VectorVarNode>(expr, exprs.release());
            } else
              break;
          }
        } else {
          var = parseVar(ident, tag);
        }
        if (toker->curr() == '=') {
          toker->next();
          ExprNode *expr = parseExpr(false);
          result = createNode<AssNode>(pos, var, expr);
        } else if (toker->curr() == '(') {
          // Method call or array access (but arrays don't appear here usually)
          if (auto *fv = dynamic_cast<FieldVarNode *>((VarNode *)var)) {
            toker->next();
            a_ptr<ExprSeqNode> exprs(parseExprSeq());
            if (toker->curr() != ')')
              exp("')'");
            toker->next();
            ExprNode *call = d_new<MethodCallNode>(fv->expr, fv->ident, fv->tag,
                                                   exprs.release());
            fv->expr = 0; // Detach before delete fv via a_ptr
            result = createNode<ExprStmtNode>(pos, call);
          } else {
            exp("variable assignment");
          }
        } else {
          exp("variable assignment or method call");
        }
      }
    } break;
    case IF: {
      toker->next();
      result = parseIf();
      if (toker->curr() == ENDIF)
        toker->next();
    } break;
    case WHILE: {
      toker->next();
      a_ptr<ExprNode> expr(parseExpr(false));
      a_ptr<StmtSeqNode> stmts(parseStmtSeq(STMTS_BLOCK));
      int pos = toker->pos();
      if (toker->curr() != WEND)
        exp("'Wend'");
      toker->next();
      result = createNode<WhileNode>(pos, expr.release(), stmts.release(), pos);
    } break;
    case REPEAT: {
      toker->next();
      ExprNode *expr = 0;
      a_ptr<StmtSeqNode> stmts(parseStmtSeq(STMTS_BLOCK));
      int curr = toker->curr();
      int pos = toker->pos();
      if (curr != UNTIL && curr != FOREVER)
        exp("'Until' or 'Forever'");
      toker->next();
      if (curr == UNTIL)
        expr = parseExpr(false);
      result = createNode<RepeatNode>(pos, stmts.release(), expr, pos);
    } break;
    case SELECT: {
      toker->next();
      ExprNode *expr = parseExpr(false);
      a_ptr<SelectNode> selNode(createNode<SelectNode>(pos, expr));
      for (;;) {
        while (isTerm(toker->curr()))
          toker->next();
        if (toker->curr() == CASE) {
          toker->next();
          a_ptr<ExprSeqNode> exprs(parseExprSeq());
          if (!exprs->size())
            exp("expression sequence");
          a_ptr<StmtSeqNode> stmts(parseStmtSeq(STMTS_BLOCK));
          selNode->push_back(
              createNode<CaseNode>(pos, exprs.release(), stmts.release()));
          continue;
        } else if (toker->curr() == DEFAULT) {
          toker->next();
          a_ptr<StmtSeqNode> stmts(parseStmtSeq(STMTS_BLOCK));
          if (toker->curr() != ENDSELECT)
            exp("'End Select'");
          selNode->defStmts = stmts.release();
          break;
        } else if (toker->curr() == ENDSELECT) {
          break;
        }
        exp("'Case', 'Default' or 'End Select'");
      }
      toker->next();
      result = selNode.release();
    } break;
    case FOR: {
      a_ptr<VarNode> var;
      a_ptr<StmtSeqNode> stmts;
      toker->next();
      var = parseVar();
      if (toker->curr() != '=')
        exp("variable assignment");
      if (toker->next() == EACH) {
        toker->next();
        std::string ident = parseIdent();
        stmts = parseStmtSeq(STMTS_BLOCK);
        int pos = toker->pos();
        if (toker->curr() != NEXT)
          exp("'Next'");
        toker->next();
        result = createNode<ForEachNode>(pos, var.release(), ident,
                                         stmts.release(), pos);
      } else {
        a_ptr<ExprNode> from, to, step;
        from = parseExpr(false);
        if (toker->curr() != TO)
          exp("'TO'");
        toker->next();
        to = parseExpr(false);
        // step...
        if (toker->curr() == STEP) {
          toker->next();
          step = parseExpr(false);
        } else
          step = createNode<IntConstNode>(pos, 1);
        stmts = parseStmtSeq(STMTS_BLOCK);
        int pos = toker->pos();
        if (toker->curr() != NEXT)
          exp("'Next'");
        toker->next();
        result = createNode<ForNode>(pos, var.release(), from.release(),
                                     to.release(), step.release(),
                                     stmts.release(), pos);
      }
    } break;
    case EXIT: {
      toker->next();
      result = createNode<ExitNode>(pos);
    } break;
    case GOTO: {
      toker->next();
      std::string t = parseIdent();
      result = createNode<GotoNode>(pos, t);
    } break;
    case GOSUB: {
      toker->next();
      std::string t = parseIdent();
      result = createNode<GosubNode>(pos, t);
    } break;
    case RETURN: {
      toker->next();
      result = createNode<ReturnNode>(pos, parseExpr(true));
    } break;
    case BBDELETE: {
      if (toker->next() == EACH) {
        toker->next();
        std::string t = parseIdent();
        result = createNode<DeleteEachNode>(pos, t);
      } else {
        ExprNode *expr = parseExpr(false);
        result = createNode<DeleteNode>(pos, expr);
      }
    } break;
    case INSERT: {
      toker->next();
      a_ptr<ExprNode> expr1(parseExpr(false));
      if (toker->curr() != BEFORE && toker->curr() != AFTER)
        exp("'Before' or 'After'");
      bool before = toker->curr() == BEFORE;
      toker->next();
      a_ptr<ExprNode> expr2(parseExpr(false));
      result =
          createNode<InsertNode>(pos, expr1.release(), expr2.release(), before);
    } break;
    case READ:
      do {
        toker->next();
        VarNode *var = parseVar();
        StmtNode *stmt = createNode<ReadNode>(pos, var);
        stmts->push_back(stmt);
      } while (toker->curr() == ',');
      break;
    case RESTORE:
      if (toker->next() == IDENT) {
        result = createNode<RestoreNode>(pos, toker->text());
        toker->next();
      } else
        result = createNode<RestoreNode>(pos, "");
      break;
    case DATA:
      if (scope != STMTS_PROG)
        ex("'Data' can only appear in main program");
      do {
        toker->next();
        ExprNode *expr = parseExpr(false);
        datas->push_back(createNode<DataDeclNode>(pos, expr));
      } while (toker->curr() == ',');
      break;
    case TYPE:
      if (scope != STMTS_PROG)
        ex("'Type' can only appear in main program");
      toker->next();
      structs->push_back(parseStructDecl());
      break;
    case BBCONST:
      if (scope != STMTS_PROG)
        ex("'Const' can only appear in main program");
      do {
        toker->next();
        consts->push_back(parseVarDecl(DECL_GLOBAL, true));
      } while (toker->curr() == ',');
      break;
    case FUNCTION:
      if (scope != STMTS_PROG)
        ex("'Function' can only appear in main program");
      toker->next();
      funcs->push_back(parseFuncDecl());
      break;
    case DIM:
      do {
        toker->next();
        StmtNode *stmt = parseArrayDecl();
        stmt->pos = pos;
        pos = toker->pos();
        stmts->push_back(stmt);
      } while (toker->curr() == ',');
      break;
    case LOCAL:
      do {
        toker->next();
        DeclNode *d = parseVarDecl(DECL_LOCAL, false);
        StmtNode *stmt = createNode<DeclStmtNode>(pos, d);
        pos = toker->pos();
        stmts->push_back(stmt);
      } while (toker->curr() == ',');
      break;
    case GLOBAL:
      if (scope != STMTS_PROG)
        ex("'Global' can only appear in main program");
      do {
        toker->next();
        DeclNode *d = parseVarDecl(DECL_GLOBAL, false);
        StmtNode *stmt = createNode<DeclStmtNode>(pos, d);
        pos = toker->pos();
        stmts->push_back(stmt);
      } while (toker->curr() == ',');
      break;
    case '.': {
      toker->next();
      std::string t = parseIdent();
      result = createNode<LabelNode>(pos, t, datas->size());
    } break;
    default:
      return;
    }

    if (result) {
      setPos(result, pos);
      stmts->push_back(result);
    }
  }
}

std::string Parser::parseTypeTag() {
  switch (toker->curr()) {
  case '%':
    toker->next();
    return "%";
  case '#':
    toker->next();
    return "#";
  case '$':
    toker->next();
    return "$";
  case '.':
    toker->next();
    return parseIdent();
  }
  return "";
}

VarNode *Parser::parseVar() {
  int pos = toker->pos();
  std::string ident = parseIdent();
  std::string tag = parseTypeTag();
  return setPos(parseVar(ident, tag), pos);
}

VarNode *Parser::parseVar(const std::string &ident, const std::string &tag) {
  a_ptr<VarNode> var;
  if (toker->curr() == '(') {
    toker->next();
    a_ptr<ExprSeqNode> exprs(parseExprSeq());
    if (toker->curr() != ')')
      exp("')'");
    toker->next();
    var = d_new<ArrayVarNode>(ident, tag, exprs.release());
  } else
    var = d_new<IdentVarNode>(ident, tag);

  for (;;) {
    if (toker->curr() == '\\') {
      toker->next();
      std::string ident = parseIdent();
      std::string tag = parseTypeTag();
      ExprNode *expr = d_new<VarExprNode>(var.release());
      var = d_new<FieldVarNode>(expr, ident, tag);
    } else if (toker->curr() == '[') {
      toker->next();
      a_ptr<ExprSeqNode> exprs(parseExprSeq());
      if (exprs->exprs.size() != 1 || toker->curr() != ']')
        exp("']'");
      toker->next();
      ExprNode *expr = d_new<VarExprNode>(var.release());
      var = d_new<VectorVarNode>(expr, exprs.release());
    } else {
      break;
    }
  }
  return var.release();
}

DeclNode *Parser::parseVarDecl(int kind, bool constant) {
  int pos = toker->pos();
  std::string ident = parseIdent();
  std::string tag = parseTypeTag();
  DeclNode *d;
  if (toker->curr() == '[') {
    if (constant)
      ex("Blitz arrays may not be constant");
    toker->next();
    a_ptr<ExprSeqNode> exprs(parseExprSeq());
    if (exprs->size() != 1 || toker->curr() != ']')
      exp("']'");
    toker->next();
    d = d_new<VectorDeclNode>(ident, tag, exprs.release(), kind);
  } else {
    ExprNode *expr = 0;
    if (toker->curr() == '=') {
      toker->next();
      expr = parseExpr(false);
    } else if (constant)
      ex("Constants must be initialized");
    d = d_new<VarDeclNode>(ident, tag, kind, constant, expr);
  }
  d->pos = pos;
  d->file = incfile;
  return d;
}

DimNode *Parser::parseArrayDecl() {
  int pos = toker->pos();
  std::string ident = parseIdent();
  std::string tag = parseTypeTag();
  if (toker->curr() != '(')
    exp("'('");
  toker->next();
  a_ptr<ExprSeqNode> exprs(parseExprSeq());
  if (toker->curr() != ')')
    exp("')'");
  if (!exprs->size())
    ex("can't have a 0 dimensional array");
  toker->next();
  DimNode *d = d_new<DimNode>(ident, tag, exprs.release());
  arrayDecls[ident] = d;
  d->pos = pos;
  return d;
}

DeclNode *Parser::parseFuncDecl() {
  int pos = toker->pos();
  std::string ident = parseIdent();
  std::string tag = parseTypeTag();
  if (toker->curr() != '(')
    exp("'('");
  a_ptr<DeclSeqNode> params(createNode<DeclSeqNode>(toker->pos()));
  if (toker->next() != ')') {
    for (;;) {
      params->push_back(parseVarDecl(DECL_PARAM, false));
      if (toker->curr() != ',')
        break;
      toker->next();
    }
    if (toker->curr() != ')')
      exp("')'");
  }
  toker->next();
  a_ptr<StmtSeqNode> stmts(parseStmtSeq(STMTS_BLOCK));
  if (toker->curr() != ENDFUNCTION)
    exp("'End Function'");
  StmtNode *ret = createNode<ReturnNode>(toker->pos(), nullptr);
  stmts->push_back(ret);
  toker->next();
  return createNode<FuncDeclNode>(pos, ident, tag, params.release(),
                                  stmts.release());
}

DeclNode *Parser::parseMethodDecl() {
  int pos = toker->pos();
  std::string ident = parseIdent();
  std::string tag = parseTypeTag();
  if (toker->curr() != '(')
    exp("'('");
  a_ptr<DeclSeqNode> params(createNode<DeclSeqNode>(toker->pos()));
  if (toker->next() != ')') {
    for (;;) {
      params->push_back(parseVarDecl(DECL_PARAM, false));
      if (toker->curr() != ',')
        break;
      toker->next();
    }
    if (toker->curr() != ')')
      exp("')'");
  }
  toker->next();
  a_ptr<StmtSeqNode> stmts(parseStmtSeq(STMTS_BLOCK));
  if (toker->curr() != ENDMETHOD)
    exp("'End Method'");
  StmtNode *ret = createNode<ReturnNode>(toker->pos(), nullptr);
  stmts->push_back(ret);
  toker->next();
  return createNode<MethodDeclNode>(pos, ident, tag, params.release(),
                                    stmts.release());
}

DeclNode *Parser::parseStructDecl() {
  int pos = toker->pos();
  std::string ident = parseIdent();
  while (toker->curr() == '\n')
    toker->next();
  a_ptr<DeclSeqNode> fields(createNode<DeclSeqNode>(toker->pos()));
  for (;;) {
    if (toker->curr() == FIELD) {
      do {
        toker->next();
        fields->push_back(parseVarDecl(DECL_FIELD, false));
      } while (toker->curr() == ',');
    } else if (toker->curr() == METHOD) {
      toker->next();
      fields->push_back(parseMethodDecl());
    } else {
      break;
    }
    while (toker->curr() == '\n')
      toker->next();
  }
  if (toker->curr() != ENDTYPE)
    exp("'Field', 'Method' or 'End Type'");
  toker->next();
  return createNode<StructDeclNode>(pos, ident, fields.release());
}

IfNode *Parser::parseIf() {
  int pos = toker->pos();
  a_ptr<ExprNode> expr;
  a_ptr<StmtSeqNode> stmts, elseOpt;

  expr = parseExpr(false);
  if (toker->curr() == THEN)
    toker->next();

  bool blkif = isTerm(toker->curr());
  stmts = parseStmtSeq(blkif ? STMTS_BLOCK : STMTS_LINE);

  if (toker->curr() == ELSEIF) {
    int pos = toker->pos();
    toker->next();
    IfNode *ifnode = setPos(parseIf(), pos);
    elseOpt = createNode<StmtSeqNode>(pos, incfile);
    elseOpt->push_back(ifnode);
  } else if (toker->curr() == ELSE) {
    toker->next();
    elseOpt = parseStmtSeq(blkif ? STMTS_BLOCK : STMTS_LINE);
  }
  if (blkif) {
    if (toker->curr() != ENDIF)
      exp("'EndIf'");
  } else if (toker->curr() != '\n')
    exp("end-of-line");

  return createNode<IfNode>(pos, expr.release(), stmts.release(),
                            elseOpt.release());
}

ExprSeqNode *Parser::parseExprSeq() {
  a_ptr<ExprSeqNode> exprs(d_new<ExprSeqNode>());
  bool opt = true;
  while (ExprNode *e = parseExpr(opt)) {
    exprs->push_back(e);
    if (toker->curr() != ',')
      break;
    toker->next();
    opt = false;
  }
  return exprs.release();
}

ExprNode *Parser::parseExpr(bool opt) {
  int pos = toker->pos();
  if (toker->curr() == NOT) {
    toker->next();
    ExprNode *expr = parseExpr1(false);
    return createNode<RelExprNode>(pos, '=', expr,
                                   createNode<IntConstNode>(pos, 0));
  }
  return parseExpr1(opt);
}

ExprNode *Parser::parseExpr1(bool opt) {

  a_ptr<ExprNode> lhs(parseExpr2(opt));
  if (!lhs)
    return 0;
  for (;;) {
    int c = toker->curr();
    if (c != AND && c != OR && c != XOR)
      return lhs.release();
    int pos = toker->pos();
    toker->next();
    ExprNode *rhs = parseExpr2(false);
    int lpos = lhs->pos;
    lhs = createNode<BinExprNode>(lpos, c, lhs.release(), rhs);
  }
}

ExprNode *Parser::parseExpr2(bool opt) {

  a_ptr<ExprNode> lhs(parseExpr3(opt));
  if (!lhs)
    return 0;
  for (;;) {
    int c = toker->curr();
    if (c != '<' && c != '>' && c != '=' && c != LE && c != GE && c != NE)
      return lhs.release();
    int pos = toker->pos();
    toker->next();
    ExprNode *rhs = parseExpr3(false);
    int lpos = lhs->pos;
    lhs = createNode<RelExprNode>(lpos, c, lhs.release(), rhs);
  }
}

ExprNode *Parser::parseExpr3(bool opt) {

  a_ptr<ExprNode> lhs(parseExpr4(opt));
  if (!lhs)
    return 0;
  for (;;) {
    int c = toker->curr();
    if (c != '+' && c != '-')
      return lhs.release();
    int pos = toker->pos();
    toker->next();
    ExprNode *rhs = parseExpr4(false);
    int lpos = lhs->pos;
    lhs = createNode<ArithExprNode>(lpos, c, lhs.release(), rhs);
  }
}

ExprNode *Parser::parseExpr4(bool opt) {
  a_ptr<ExprNode> lhs(parseExpr5(opt));
  if (!lhs)
    return 0;
  for (;;) {
    int c = toker->curr();
    if (c != SHL && c != SHR && c != SAR)
      return lhs.release();
    int pos = toker->pos();
    toker->next();
    ExprNode *rhs = parseExpr5(false);
    int lpos = lhs->pos;
    lhs = createNode<BinExprNode>(lpos, c, lhs.release(), rhs);
  }
}

ExprNode *Parser::parseExpr5(bool opt) {

  a_ptr<ExprNode> lhs(parseExpr6(opt));
  if (!lhs)
    return 0;
  for (;;) {
    int c = toker->curr();
    if (c != '*' && c != '/' && c != MOD)
      return lhs.release();
    int pos = toker->pos();
    toker->next();
    ExprNode *rhs = parseExpr6(false);
    int lpos = lhs->pos;
    lhs = createNode<ArithExprNode>(lpos, c, lhs.release(), rhs);
  }
}

ExprNode *Parser::parseExpr6(bool opt) {

  a_ptr<ExprNode> lhs(parseUniExpr(opt));
  if (!lhs)
    return 0;
  for (;;) {
    int c = toker->curr();
    if (c != '^')
      return lhs.release();
    int pos = toker->pos();
    toker->next();
    ExprNode *rhs = parseUniExpr(false);
    int lpos = lhs->pos;
    lhs = createNode<ArithExprNode>(lpos, c, lhs.release(), rhs);
  }
}

ExprNode *Parser::parseUniExpr(bool opt) {

  ExprNode *result = 0;
  std::string t;

  int c = toker->curr();
  int pos = toker->pos();
  switch (c) {
  case BBINT:
    if (toker->next() == '%')
      toker->next();
    result = setPos(parseUniExpr(false), pos);
    result = createNode<CastNode>(pos, result, Type::int_type);
    break;
  case BBFLOAT:
    if (toker->next() == '#')
      toker->next();
    result = setPos(parseUniExpr(false), pos);
    result = createNode<CastNode>(pos, result, Type::float_type);
    break;
  case BBSTR:
    if (toker->next() == '$')
      toker->next();
    result = setPos(parseUniExpr(false), pos);
    result = createNode<CastNode>(pos, result, Type::string_type);
    break;
  case OBJECT:
    if (toker->next() == '.')
      toker->next();
    t = parseIdent();
    result = setPos(parseUniExpr(false), pos);
    result = createNode<ObjectCastNode>(pos, result, t);
    break;
  case BBHANDLE:
    toker->next();
    result = setPos(parseUniExpr(false), pos);
    result = createNode<ObjectHandleNode>(pos, result);
    break;
  case BEFORE:
    toker->next();
    result = setPos(parseUniExpr(false), pos);
    result = createNode<BeforeNode>(pos, result);
    break;
  case AFTER:
    toker->next();
    result = setPos(parseUniExpr(false), pos);
    result = createNode<AfterNode>(pos, result);
    break;
  case '+':
  case '-':
  case '~':
  case ABS:
  case SGN:
    toker->next();
    result = setPos(parseUniExpr(false), pos);
    if (c == '~') {
      result = createNode<BinExprNode>(pos, XOR, result,
                                       createNode<IntConstNode>(pos, -1));
    } else {
      result = createNode<UniExprNode>(pos, c, result);
    }
    break;
  default:
    result = parsePrimary(opt);
  }
  return result;
}

ExprNode *Parser::parsePrimary(bool opt) {
  int pos = toker->pos();
  a_ptr<ExprNode> expr;
  std::string t, ident, tag;
  ExprNode *result = 0;
  int n, k;

  switch (toker->curr()) {
  case '(':
    toker->next();
    expr = parseExpr(false);
    if (toker->curr() != ')')
      exp("')'");
    toker->next();
    result = expr.release();
    break;
  case BBNEW:
    toker->next();
    t = parseIdent();
    result = d_new<NewNode>(t);
    break;
  case FIRST:
    toker->next();
    t = parseIdent();
    result = d_new<FirstNode>(t);
    break;
  case LAST:
    toker->next();
    t = parseIdent();
    result = d_new<LastNode>(t);
    break;
  case BBNULL:
    result = d_new<NullNode>();
    toker->next();
    break;
  case INTCONST:
    result = d_new<IntConstNode>(atoi(toker->text().c_str()));
    toker->next();
    break;
  case FLOATCONST:
    result = d_new<FloatConstNode>(atof(toker->text().c_str()));
    toker->next();
    break;
  case STRINGCONST:
    t = toker->text();
    result = d_new<StringConstNode>(t.substr(1, t.size() - 2));
    toker->next();
    break;
  case BINCONST:
    n = 0;
    t = toker->text();
    for (k = 1; k < (int)t.size(); ++k)
      n = (n << 1) | (t[k] == '1');
    result = d_new<IntConstNode>(n);
    toker->next();
    break;
  case HEXCONST:
    n = 0;
    t = toker->text();
    for (k = 1; k < (int)t.size(); ++k)
      n = (n << 4) | (isdigit(t[k]) ? t[k] & 0xf : (t[k] & 7) + 9);
    result = d_new<IntConstNode>(n);
    toker->next();
    break;
  case PI:
    result = d_new<FloatConstNode>(3.1415926535897932384626433832795f);
    toker->next();
    break;
  case BBTRUE:
    result = d_new<IntConstNode>(1);
    toker->next();
    break;
  case BBFALSE:
    result = d_new<IntConstNode>(0);
    toker->next();
    break;
  case SELF:
  case IDENT: {
    int c = toker->curr();
    std::string ident, tag;
    if (c == SELF) {
      toker->next();
      result = d_new<SelfNode>();
    } else {
      ident = toker->text();
      toker->next();
      tag = parseTypeTag();
      if (toker->curr() == '(' && arrayDecls.find(ident) == arrayDecls.end()) {
        toker->next();
        a_ptr<ExprSeqNode> exprs(parseExprSeq());
        if (toker->curr() != ')')
          exp("')'");
        toker->next();
        result = d_new<CallNode>(ident, tag, exprs.release());
      } else {
        VarNode *var = parseVar(ident, tag);
        result = d_new<VarExprNode>(var);
      }
    }
    // Handle trailers for Self or result of Call/Var
    for (;;) {
      if (toker->curr() == '\\') {
        toker->next();
        std::string ident = parseIdent();
        std::string tag = parseTypeTag();
        VarNode *v = d_new<FieldVarNode>(result, ident, tag);
        result = d_new<VarExprNode>(v);
      } else if (toker->curr() == '[') {
        toker->next();
        a_ptr<ExprSeqNode> exprs(parseExprSeq());
        if (exprs->exprs.size() != 1 || toker->curr() != ']')
          exp("']'");
        toker->next();
        VarNode *v = d_new<VectorVarNode>(result, exprs.release());
        result = d_new<VarExprNode>(v);
      } else if (toker->curr() == '(') {
        // If the current result is a FieldVarNode, it might be a method call
        if (VarExprNode *ve = dynamic_cast<VarExprNode *>(result)) {
          if (FieldVarNode *fv = dynamic_cast<FieldVarNode *>(ve->var)) {
            toker->next();
            a_ptr<ExprSeqNode> exprs(parseExprSeq());
            if (toker->curr() != ')')
              exp("')'");
            toker->next();
            result = d_new<MethodCallNode>(fv->expr, fv->ident, fv->tag,
                                           exprs.release());
            fv->expr = 0; // Detach
            // fv and ve will be cleaned up by MemoryManager eventually or we
            // should delete them? For now, this is safer.
            continue;
          }
        }
        break;
      } else
        break;
    }
    break;
  }
  default:
    if (!opt)
      exp("expression");
  }
  if (result)
    setPos(result, pos);
  return result;
}
