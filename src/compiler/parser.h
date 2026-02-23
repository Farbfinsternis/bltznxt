#ifndef BLITZNEXT_PARSER_H
#define BLITZNEXT_PARSER_H

#include "ast.h"
#include "lexer.h"
#include <algorithm>
#include <iostream>
#include <memory>
#include <unordered_set>
#include <vector>

class Parser {
public:
  Parser() : pos(0), errorCount(0) {}

  std::unique_ptr<Program> parse(const std::vector<Token> &toks,
                                 const std::string &fname = "") {
    tokens       = toks;
    pos          = 0;
    filename     = fname;
    errorCount   = 0;
    dimmedArrays.clear();
    auto prog = std::make_unique<Program>();

    while (!atEnd()) {
      skipNewlines();
      if (atEnd()) break;
      // Function declarations are lifted to top-level
      if (peekKw() == "FUNCTION") {
        prog->nodes.push_back(parseFunctionDecl());
      } else {
        auto s = parseStatement();
        if (s) prog->nodes.push_back(std::move(s));
      }
    }
    return prog;
  }

  bool hasErrors() const { return errorCount > 0; }

private:
  // ------------------------------------------------------------------ utils

  // Emits an IDE-parseable diagnostic (GCC format: file:line:col: error: msg)
  void error(int line, int col, const std::string &msg) {
    std::cerr << filename << ":" << line << ":" << col << ": error: "
              << msg << "\n";
    ++errorCount;
  }

  bool atEnd() const {
    return pos >= tokens.size() || tokens[pos].type == TokenType::EOF_TOKEN;
  }

  Token peek() const {
    if (pos < tokens.size()) return tokens[pos];
    return {TokenType::EOF_TOKEN, "", 0, 0};
  }

  Token advance() {
    if (pos < tokens.size()) return tokens[pos++];
    return {TokenType::EOF_TOKEN, "", 0, 0};
  }

  // Returns keyword value (already uppercase) or "" if not a keyword
  std::string peekKw() const {
    const Token &t = (pos < tokens.size()) ? tokens[pos]
                                           : Token{TokenType::EOF_TOKEN,"",0,0};
    return (t.type == TokenType::KEYWORD) ? t.value : "";
  }

  void skipNewlines() {
    while (!atEnd() && (peek().type == TokenType::NEWLINE ||
                        (peek().type == TokenType::OPERATOR &&
                         peek().value == ":")))
      advance();
  }

  // Expects a token; if mismatch, emits a structured error and advances.
  Token expect(TokenType type, const char *msg, const char *value = "") {
    Token t = peek();
    if (t.type == type && (value[0] == '\0' || t.value == value))
      return advance();
    std::string got = t.value.empty() ? "<EOF>" : ("'" + t.value + "'");
    error(t.line, t.col, std::string(msg) + " (got " + got + ")");
    if (!atEnd()) advance(); // error recovery
    return t;
  }

  // ------------------------------------------------------------------ statements

  std::unique_ptr<ASTNode> parseStatement() {
    skipNewlines();
    if (atEnd()) return nullptr;

    Token t = peek();

    // ---- keyword-led statements ----
    if (t.type == TokenType::KEYWORD) {
      const std::string &kw = t.value; // already uppercase

      if (kw == "LOCAL" || kw == "GLOBAL")
        return parseVarDecl();
      if (kw == "DIM")    return parseDim();
      if (kw == "CONST")  return parseConst();
      if (kw == "IF")     return parseIf();
      if (kw == "WHILE")  return parseWhile();
      if (kw == "REPEAT") return parseRepeat();
      if (kw == "FOR") {
        // Peek ahead: "For Each" → ForEach loop; otherwise → regular For loop
        size_t savedPos = pos;
        advance(); // consume FOR
        if (peekKw() == "EACH") {
          pos = savedPos; // restore — parseForEach re-consumes FOR
          return parseForEach();
        }
        pos = savedPos;
        return parseFor();
      }
      if (kw == "SELECT") return parseSelect();
      if (kw == "RETURN")  return parseReturn();
      if (kw == "DATA")    return parseData();
      if (kw == "READ")    return parseRead();
      if (kw == "RESTORE") return parseRestore();
      if (kw == "TYPE")    return parseTypeDecl();
      if (kw == "DELETE")  return parseDelete();
      if (kw == "INSERT")  return parseInsert();

      if (kw == "GOTO") {
        int ln = t.line;
        advance();
        // Accept both "Goto label" and "Goto .label"
        if (peek().type == TokenType::OPERATOR && peek().value == ".") advance();
        Token lblTok = expect(TokenType::ID, "Expected label name after Goto");
        std::string lo = lblTok.value;
        std::transform(lo.begin(), lo.end(), lo.begin(), ::tolower);
        auto s = std::make_unique<GotoStmt>(lo);
        s->line = ln;
        return s;
      }
      if (kw == "GOSUB") {
        int ln = t.line;
        advance();
        // Accept both "Gosub label" and "Gosub .label"
        if (peek().type == TokenType::OPERATOR && peek().value == ".") advance();
        Token lblTok = expect(TokenType::ID, "Expected label name after Gosub");
        std::string lo = lblTok.value;
        std::transform(lo.begin(), lo.end(), lo.begin(), ::tolower);
        auto s = std::make_unique<GosubStmt>(lo);
        s->line = ln;
        return s;
      }

      if (kw == "EXIT") {
        int ln = t.line;
        advance();
        auto s = std::make_unique<ExitStmt>();
        s->line = ln;
        return s;
      }
      if (kw == "END") {
        int ln = t.line;
        advance();
        // "End Function" / "End If" / "End Select" are block terminators,
        // handled by parseBlock callers. A bare "End" terminates the program.
        std::string nk = peekKw();
        if (nk == "FUNCTION" || nk == "IF" || nk == "SELECT" || nk == "TYPE") {
          advance(); // consume the secondary keyword
          return nullptr;
        }
        auto s = std::make_unique<EndStmt>();
        s->line = ln;
        return s;
      }

      // Unknown keyword as statement — skip to avoid infinite loop
      advance();
      return nullptr;
    }

    // ---- .labelname → LabelStmt ----
    if (t.type == TokenType::OPERATOR && t.value == ".") {
      int ln = t.line;
      advance(); // consume '.'
      if (peek().type == TokenType::ID) {
        Token lblTok = advance();
        std::string lo = lblTok.value;
        std::transform(lo.begin(), lo.end(), lo.begin(), ::tolower);
        auto s = std::make_unique<LabelStmt>(lo);
        s->line = ln;
        return s;
      }
      return nullptr;
    }

    // ---- identifier-led: assignment or command call ----
    if (t.type == TokenType::ID) {
      Token nameTok = advance();

      // labelname: → LabelStmt (must check before type-hint consumption)
      if (peek().type == TokenType::OPERATOR && peek().value == ":") {
        advance(); // consume ':'
        std::string lo = nameTok.value;
        std::transform(lo.begin(), lo.end(), lo.begin(), ::tolower);
        auto s = std::make_unique<LabelStmt>(lo);
        s->line = nameTok.line;
        return s;
      }

      // Optional type-hint suffix on the variable name (x#, s$, n%, f!)
      if (peek().type == TokenType::OPERATOR &&
          (peek().value == "#" || peek().value == "%" ||
           peek().value == "!" || peek().value == "$"))
        advance(); // consume — same variable regardless of hint

      // Field assignment: var\field = expr  (Blitz3D \ field separator)
      if (peek().type == TokenType::OPERATOR && peek().value == "\\") {
        advance(); // consume '\'
        Token fname = expect(TokenType::ID, "Expected field name after \\");
        expect(TokenType::OPERATOR, "Expected '='", "=");
        auto val = parseExpr();
        auto s = std::make_unique<FieldAssignStmt>(
            std::make_unique<VarExpr>(nameTok.value), fname.value, std::move(val));
        s->line = nameTok.line;
        return s;
      }

      // Array assignment: arr(i) = expr  or  grid(x,y) = expr
      {
        std::string lo = nameTok.value;
        std::transform(lo.begin(), lo.end(), lo.begin(), ::tolower);
        if (dimmedArrays.count(lo) &&
            peek().type == TokenType::OPERATOR && peek().value == "(") {
          advance(); // (
          auto stmt = std::make_unique<ArrayAssignStmt>(nameTok.value, nullptr);
          stmt->line = nameTok.line;
          while (true) {
            stmt->indices.push_back(parseExpr());
            if (peek().type == TokenType::OPERATOR && peek().value == ",")
              advance();
            else
              break;
          }
          expect(TokenType::OPERATOR, "Expected ')'", ")");
          expect(TokenType::OPERATOR, "Expected '='", "=");
          stmt->value = parseExpr();
          return stmt;
        }
      }

      // Assignment: x = expr
      if (peek().type == TokenType::OPERATOR && peek().value == "=") {
        advance(); // consume "="
        auto val = parseExpr();
        auto a = std::make_unique<AssignStmt>(nameTok.value, std::move(val));
        a->line = nameTok.line;
        return a;
      }

      // Otherwise: command / function call as statement
      auto call = std::make_unique<CallExpr>(nameTok.value);
      call->line = nameTok.line;

      // Parenthesised call form: Name(arg1, arg2)  or  Name()
      if (peek().type == TokenType::OPERATOR && peek().value == "(") {
        advance(); // consume (
        if (!(peek().type == TokenType::OPERATOR && peek().value == ")")) {
          while (true) {
            call->args.push_back(parseExpr());
            if (peek().type == TokenType::OPERATOR && peek().value == ",")
              advance();
            else
              break;
          }
        }
        expect(TokenType::OPERATOR, "Expected ')'", ")");
        return call;
      }

      // Blitz3D-style call without parens: Name arg1, arg2
      while (!atEnd() && peek().type != TokenType::NEWLINE &&
             peek().type != TokenType::EOF_TOKEN &&
             !(peek().type == TokenType::OPERATOR && peek().value == ":")) {
        // Break on block-terminator keywords, but allow expression-starter
        // keywords (Not, True, False, Null, New, First, Last, Before, After).
        if (peek().type == TokenType::KEYWORD) {
          const std::string &kw = peek().value;
          bool isExprStarter = (kw == "NOT"  || kw == "TRUE" || kw == "FALSE" ||
                                kw == "NULL" || kw == "NEW"  || kw == "FIRST" ||
                                kw == "LAST" || kw == "BEFORE" || kw == "AFTER");
          if (!isExprStarter) break;
        }
        call->args.push_back(parseExpr());
        if (peek().type == TokenType::OPERATOR && peek().value == ",")
          advance();
        else
          break;
      }
      return call;
    }

    // Unrecognised token — skip
    advance();
    return nullptr;
  }

  // Parses statements until one of the terminator keywords is seen (not consumed).
  std::vector<std::unique_ptr<ASTNode>>
  parseBlock(std::initializer_list<const char *> terminators) {
    std::vector<std::unique_ptr<ASTNode>> block;
    while (!atEnd()) {
      skipNewlines();
      if (atEnd()) break;

      if (peek().type == TokenType::KEYWORD) {
        const std::string &kw = peek().value;
        for (const char *term : terminators)
          if (kw == term) return block; // leave terminator for caller
      }

      // Never descend into a nested FUNCTION declaration from a block
      if (peek().type == TokenType::KEYWORD && peek().value == "FUNCTION")
        break;

      auto s = parseStatement();
      if (s) block.push_back(std::move(s));
    }
    return block;
  }

  // ------------------------------------------------------------------ IF

  std::unique_ptr<IfStmt> parseIf() {
    int ln = peek().line;
    advance(); // consume IF
    auto s = parseIfTail();
    s->line = ln;
    return s;
  }

  // Shared between IF and ELSEIF (both parse condition + body + tail).
  std::unique_ptr<IfStmt> parseIfTail() {
    auto cond = parseExpr();
    if (peekKw() == "THEN") advance(); // optional THEN

    auto stmt = std::make_unique<IfStmt>(std::move(cond));

    // Single-line form: "If cond Then stmt [Else stmt]"
    if (peek().type != TokenType::NEWLINE && !atEnd()) {
      auto s = parseStatement();
      if (s) stmt->thenBlock.push_back(std::move(s));
      if (peekKw() == "ELSE") {
        advance();
        auto e = parseStatement();
        if (e) stmt->elseBlock.push_back(std::move(e));
      }
      return stmt;
    }

    // Block form
    stmt->thenBlock = parseBlock({"ELSE", "ELSEIF", "ENDIF"});

    std::string kw = peekKw();
    if (kw == "ELSEIF") {
      advance(); // consume ELSEIF
      // Treat as "Else If ..." — recurse; nested parseIfTail will consume ENDIF
      auto nested = parseIfTail();
      stmt->elseBlock.push_back(std::move(nested));
    } else if (kw == "ELSE") {
      advance(); // consume ELSE
      stmt->elseBlock = parseBlock({"ENDIF"});
      expect(TokenType::KEYWORD, "Expected ENDIF", "ENDIF");
    } else {
      expect(TokenType::KEYWORD, "Expected ENDIF", "ENDIF");
    }

    return stmt;
  }

  // ------------------------------------------------------------------ WHILE

  std::unique_ptr<WhileStmt> parseWhile() {
    int ln = peek().line;
    advance(); // WHILE
    auto cond = parseExpr();
    auto stmt = std::make_unique<WhileStmt>(std::move(cond));
    stmt->line  = ln;
    stmt->block = parseBlock({"WEND"});
    expect(TokenType::KEYWORD, "Expected WEND", "WEND");
    return stmt;
  }

  // ------------------------------------------------------------------ REPEAT

  std::unique_ptr<RepeatStmt> parseRepeat() {
    int ln = peek().line;
    advance(); // REPEAT
    auto stmt  = std::make_unique<RepeatStmt>();
    stmt->line = ln;
    stmt->block = parseBlock({"UNTIL", "FOREVER"});

    std::string kw = peekKw();
    if (kw == "UNTIL") {
      advance();
      stmt->condition = parseExpr();
    } else if (kw == "FOREVER") {
      advance();
      stmt->condition = nullptr;
    } else {
      Token t = peek();
      error(t.line, t.col, "expected UNTIL or FOREVER");
    }
    return stmt;
  }

  // ------------------------------------------------------------------ FOR

  std::unique_ptr<ForStmt> parseFor() {
    int ln = peek().line;
    advance(); // FOR
    Token nameTok = expect(TokenType::ID, "Expected loop variable name");

    // Optional type hint on loop variable
    if (peek().type == TokenType::OPERATOR &&
        (peek().value == "#" || peek().value == "%" ||
         peek().value == "!" || peek().value == "$"))
      advance();

    expect(TokenType::OPERATOR, "Expected '='", "=");
    auto start = parseExpr();
    expect(TokenType::KEYWORD, "Expected TO", "TO");
    auto end  = parseExpr();

    std::unique_ptr<ExprNode> step;
    if (peekKw() == "STEP") {
      advance();
      step = parseExpr();
    }

    auto stmt  = std::make_unique<ForStmt>(nameTok.value, std::move(start),
                                           std::move(end), std::move(step));
    stmt->line  = ln;
    stmt->block = parseBlock({"NEXT"});
    expect(TokenType::KEYWORD, "Expected NEXT", "NEXT");
    if (peek().type == TokenType::ID) advance(); // optional "Next i"
    return stmt;
  }

  // ------------------------------------------------------------------ SELECT

  std::unique_ptr<SelectStmt> parseSelect() {
    int ln = peek().line;
    advance(); // SELECT
    auto stmt  = std::make_unique<SelectStmt>(parseExpr());
    stmt->line = ln;

    while (!atEnd()) {
      skipNewlines();
      std::string kw = peekKw();

      if (kw == "CASE") {
        advance();
        SelectStmt::Case c;
        while (true) {
          c.expressions.push_back(parseExpr());
          if (peek().type == TokenType::OPERATOR && peek().value == ",")
            advance();
          else
            break;
        }
        c.block = parseBlock({"CASE", "DEFAULT", "END", "ENDSELECT"});
        stmt->cases.push_back(std::move(c));

      } else if (kw == "DEFAULT") {
        advance();
        stmt->defaultBlock = parseBlock({"CASE", "END", "ENDSELECT"});

      } else if (kw == "END") {
        advance();
        if (peekKw() == "SELECT") advance();
        break;
      } else if (kw == "ENDSELECT") {
        advance();
        break;
      } else {
        advance(); // skip unexpected
      }
    }
    return stmt;
  }

  // ------------------------------------------------------------------ VAR DECL

  std::unique_ptr<ASTNode> parseVarDecl() {
    Token scopeTok = advance(); // LOCAL / GLOBAL
    VarDecl::Scope scope = VarDecl::LOCAL;
    if (scopeTok.value == "GLOBAL") scope = VarDecl::GLOBAL;

    // Multiple declarations on one line: "Local x = 1, y = 2"
    // We wrap them in a Program node (used here as a transparent block container)
    auto list = std::make_unique<Program>();

    while (true) {
      Token nameTok = expect(TokenType::ID, "Expected variable name");
      std::string typeHint;
      if (peek().type == TokenType::OPERATOR &&
          (peek().value == "#" || peek().value == "%" ||
           peek().value == "!" || peek().value == "$")) {
        typeHint = advance().value;
      } else if (peek().type == TokenType::OPERATOR && peek().value == ".") {
        // Object type annotation: v.Vec → typeHint = ".Vec"
        advance(); // consume '.'
        if (peek().type == TokenType::ID)
          typeHint = "." + advance().value;
      }

      std::unique_ptr<ExprNode> init;
      if (peek().type == TokenType::OPERATOR && peek().value == "=") {
        advance(); // =
        init = parseExpr();
      }

      auto vd  = std::make_unique<VarDecl>(scope, nameTok.value, typeHint,
                                            std::move(init));
      vd->line = nameTok.line;
      list->nodes.push_back(std::move(vd));

      if (peek().type == TokenType::OPERATOR && peek().value == ",")
        advance();
      else
        break;
    }
    return list;
  }

  // ------------------------------------------------------------------ DIM

  std::unique_ptr<ASTNode> parseDim() {
    advance(); // DIM
    auto list = std::make_unique<Program>();

    while (true) {
      Token nameTok = expect(TokenType::ID, "Expected array name");
      std::string typeHint;
      if (peek().type == TokenType::OPERATOR &&
          (peek().value == "#" || peek().value == "%" ||
           peek().value == "!" || peek().value == "$"))
        typeHint = advance().value;

      expect(TokenType::OPERATOR, "Expected '('", "(");
      auto ds   = std::make_unique<DimStmt>(nameTok.value, typeHint);
      ds->line  = nameTok.line;
      while (true) {
        ds->dims.push_back(parseExpr());
        if (peek().type == TokenType::OPERATOR && peek().value == ",")
          advance();
        else
          break;
      }
      expect(TokenType::OPERATOR, "Expected ')'", ")");

      // Register name (lowercase) so parsePrimary / parseStatement can
      // distinguish array access from function calls.
      std::string lo = nameTok.value;
      std::transform(lo.begin(), lo.end(), lo.begin(), ::tolower);
      dimmedArrays.insert(lo);

      list->nodes.push_back(std::move(ds));

      if (peek().type == TokenType::OPERATOR && peek().value == ",")
        advance();
      else
        break;
    }
    return list;
  }

  // ------------------------------------------------------------------ CONST

  std::unique_ptr<ASTNode> parseConst() {
    advance(); // CONST
    // Multiple on one line: "Const a% = 1, b# = 3.14"
    auto list = std::make_unique<Program>();

    while (true) {
      Token nameTok = expect(TokenType::ID, "Expected constant name");
      std::string typeHint;
      if (peek().type == TokenType::OPERATOR &&
          (peek().value == "#" || peek().value == "%" ||
           peek().value == "!" || peek().value == "$"))
        typeHint = advance().value;

      expect(TokenType::OPERATOR, "Expected '='", "=");
      auto val = parseExpr();

      auto cd  = std::make_unique<ConstDecl>(nameTok.value, typeHint,
                                              std::move(val));
      cd->line = nameTok.line;
      list->nodes.push_back(std::move(cd));

      if (peek().type == TokenType::OPERATOR && peek().value == ",")
        advance();
      else
        break;
    }
    return list;
  }

  // ------------------------------------------------------------------ FUNCTION

  std::unique_ptr<FunctionDecl> parseFunctionDecl() {
    advance(); // FUNCTION
    Token nameTok = expect(TokenType::ID, "Expected function name");
    auto func  = std::make_unique<FunctionDecl>(nameTok.value);
    func->line = nameTok.line;

    // Consume optional return-type hint on the function name (e.g. Double%)
    if (peek().type == TokenType::OPERATOR &&
        (peek().value == "#" || peek().value == "%" ||
         peek().value == "!" || peek().value == "$"))
      advance();

    if (peek().type == TokenType::OPERATOR && peek().value == "(") {
      advance(); // (
      while (!atEnd() &&
             !(peek().type == TokenType::OPERATOR && peek().value == ")")) {
        Token p = expect(TokenType::ID, "Expected parameter name");
        std::string hint;
        if (peek().type == TokenType::OPERATOR &&
            (peek().value == "#" || peek().value == "%" ||
             peek().value == "!" || peek().value == "$"))
          hint = advance().value;
        func->params.emplace_back(p.value, hint);
        if (peek().type == TokenType::OPERATOR && peek().value == ",")
          advance();
      }
      expect(TokenType::OPERATOR, "Expected ')'", ")");
    }

    func->body = parseBlock({"END", "ENDFUNCTION"});

    std::string kw = peekKw();
    if (kw == "END") {
      advance();
      if (peekKw() == "FUNCTION") advance();
    } else if (kw == "ENDFUNCTION") {
      advance();
    }
    return func;
  }

  // ------------------------------------------------------------------ DELETE

  std::unique_ptr<DeleteStmt> parseDelete() {
    int ln = peek().line;
    advance(); // DELETE
    auto obj = parseExpr();
    auto s   = std::make_unique<DeleteStmt>(std::move(obj));
    s->line  = ln;
    return s;
  }

  // ------------------------------------------------------------------ INSERT

  std::unique_ptr<InsertStmt> parseInsert() {
    int ln = peek().line;
    advance(); // INSERT
    auto obj = parseExpr();
    InsertStmt::Mode mode = InsertStmt::BEFORE;
    std::string kw = peekKw();
    if (kw == "BEFORE") { advance(); mode = InsertStmt::BEFORE; }
    else if (kw == "AFTER") { advance(); mode = InsertStmt::AFTER; }
    else {
      Token t = peek();
      error(t.line, t.col, "expected BEFORE or AFTER after Insert");
    }
    auto tgt = parseExpr();
    auto s   = std::make_unique<InsertStmt>(std::move(obj), mode, std::move(tgt));
    s->line  = ln;
    return s;
  }

  // ------------------------------------------------------------------ FOR EACH

  std::unique_ptr<ForEachStmt> parseForEach() {
    int ln = peek().line;
    advance(); // FOR
    advance(); // EACH
    Token nameTok = expect(TokenType::ID, "Expected variable name after Each");
    std::string typeName;
    if (peek().type == TokenType::OPERATOR && peek().value == ".") {
      advance(); // consume '.'
      Token tn = expect(TokenType::ID, "Expected type name after '.'");
      typeName = tn.value;
    }
    auto s   = std::make_unique<ForEachStmt>(nameTok.value, typeName);
    s->line  = ln;
    s->block = parseBlock({"NEXT"});
    expect(TokenType::KEYWORD, "Expected NEXT", "NEXT");
    if (peek().type == TokenType::ID) advance(); // optional "Next p"
    return s;
  }

  // ------------------------------------------------------------------ TYPE

  std::unique_ptr<TypeDecl> parseTypeDecl() {
    int ln = peek().line;
    advance(); // TYPE
    Token nameTok = expect(TokenType::ID, "Expected type name after Type");
    auto td = std::make_unique<TypeDecl>(nameTok.value);
    td->line = ln;

    skipNewlines();

    while (!atEnd()) {
      std::string kw = peekKw();

      if (kw == "END") {
        advance(); // END
        if (peekKw() == "TYPE") advance(); // TYPE
        break;
      }
      if (kw == "ENDTYPE") {
        advance();
        break;
      }
      if (kw == "FIELD") {
        advance(); // FIELD
        // Parse comma-separated field declarations: name[hint], name[hint], ...
        while (true) {
          Token fieldTok = expect(TokenType::ID, "Expected field name after Field");
          std::string hint;
          if (peek().type == TokenType::OPERATOR &&
              (peek().value == "%" || peek().value == "#" ||
               peek().value == "!" || peek().value == "$"))
            hint = advance().value;
          TypeDecl::Field f;
          f.name     = fieldTok.value;
          f.typeHint = hint;
          td->fields.push_back(std::move(f));
          if (peek().type == TokenType::OPERATOR && peek().value == ",")
            advance();
          else
            break;
        }
      } else {
        // Unknown token in type body — skip to avoid infinite loop
        advance();
      }
      skipNewlines();
    }
    return td;
  }

  // ------------------------------------------------------------------ DATA

  std::unique_ptr<DataStmt> parseData() {
    int ln = peek().line;
    advance(); // DATA
    auto ds  = std::make_unique<DataStmt>();
    ds->line = ln;
    while (true) {
      Token t = peek();
      // Handle optional sign for negative numeric literals
      std::string sign;
      if (t.type == TokenType::OPERATOR &&
          (t.value == "-" || t.value == "+")) {
        sign = t.value;
        advance();
        t = peek();
      }
      if (t.type == TokenType::INT_LIT || t.type == TokenType::FLOAT_LIT ||
          t.type == TokenType::STRING_LIT) {
        Token lit = advance();
        if (!sign.empty()) lit.value = sign + lit.value;
        ds->values.push_back(lit);
      } else {
        break; // no more data items
      }
      if (peek().type == TokenType::OPERATOR && peek().value == ",")
        advance();
      else
        break;
    }
    return ds;
  }

  // ------------------------------------------------------------------ READ

  std::unique_ptr<ReadStmt> parseRead() {
    int ln = peek().line;
    advance(); // READ
    Token nameTok = expect(TokenType::ID, "Expected variable name after Read");
    std::string typeHint;
    if (peek().type == TokenType::OPERATOR &&
        (peek().value == "#" || peek().value == "%" ||
         peek().value == "!" || peek().value == "$"))
      typeHint = advance().value;
    auto s  = std::make_unique<ReadStmt>(nameTok.value, typeHint);
    s->line = ln;
    return s;
  }

  // ------------------------------------------------------------------ RESTORE

  std::unique_ptr<RestoreStmt> parseRestore() {
    int ln = peek().line;
    advance(); // RESTORE
    std::string label;
    // Optional dot-label or plain label after Restore
    if (peek().type == TokenType::OPERATOR && peek().value == ".") {
      advance(); // consume '.'
      if (peek().type == TokenType::ID) {
        label = advance().value;
        std::transform(label.begin(), label.end(), label.begin(), ::tolower);
      }
    } else if (peek().type == TokenType::ID) {
      label = advance().value;
      std::transform(label.begin(), label.end(), label.begin(), ::tolower);
    }
    auto s  = std::make_unique<RestoreStmt>(label);
    s->line = ln;
    return s;
  }

  // ------------------------------------------------------------------ RETURN

  std::unique_ptr<ReturnStmt> parseReturn() {
    int ln = peek().line;
    advance(); // RETURN
    std::unique_ptr<ExprNode> val;
    if (!atEnd() && peek().type != TokenType::NEWLINE &&
        peek().type != TokenType::EOF_TOKEN &&
        !(peek().type == TokenType::OPERATOR && peek().value == ":"))
      val = parseExpr();
    auto r  = std::make_unique<ReturnStmt>(std::move(val));
    r->line = ln;
    return r;
  }

  // ------------------------------------------------------------------ expressions

  std::unique_ptr<ExprNode> parseExpr()           { return parseLogical(); }

  std::unique_ptr<ExprNode> parseLogical() {
    auto left = parseNot();
    while (peek().type == TokenType::KEYWORD) {
      const std::string &op = peek().value;
      if (op == "AND" || op == "OR" || op == "XOR") {
        int ln = peek().line;
        advance();
        auto right = parseNot();
        auto be    = std::make_unique<BinaryExpr>(op, std::move(left),
                                                   std::move(right));
        be->line = ln;
        left = std::move(be);
      } else break;
    }
    return left;
  }

  std::unique_ptr<ExprNode> parseNot() {
    if (peek().type == TokenType::KEYWORD && peek().value == "NOT") {
      int ln = peek().line;
      advance();
      auto ue  = std::make_unique<UnaryExpr>("NOT", parseComparison());
      ue->line = ln;
      return ue;
    }
    return parseComparison();
  }

  std::unique_ptr<ExprNode> parseComparison() {
    auto left = parseAdditive();
    while (peek().type == TokenType::OPERATOR) {
      const std::string &op = peek().value;
      if (op == "=" || op == "<>" || op == "<" || op == ">" ||
          op == "<=" || op == ">=") {
        int ln = peek().line;
        advance();
        auto right = parseAdditive();
        auto be    = std::make_unique<BinaryExpr>(op, std::move(left),
                                                   std::move(right));
        be->line = ln;
        left = std::move(be);
      } else break;
    }
    return left;
  }

  std::unique_ptr<ExprNode> parseAdditive() {
    auto left = parseMultiplicative();
    while (peek().type == TokenType::OPERATOR) {
      const std::string &op = peek().value;
      if (op == "+" || op == "-") {
        int ln = peek().line;
        advance();
        auto right = parseMultiplicative();
        auto be    = std::make_unique<BinaryExpr>(op, std::move(left),
                                                   std::move(right));
        be->line = ln;
        left = std::move(be);
      } else break;
    }
    return left;
  }

  std::unique_ptr<ExprNode> parseMultiplicative() {
    auto left = parseUnary();
    while (true) {
      Token t = peek();
      bool isOpMul = (t.type == TokenType::OPERATOR &&
                      (t.value == "*" || t.value == "/"));
      bool isKwMul = (t.type == TokenType::KEYWORD &&
                      (t.value == "MOD" || t.value == "SHL" ||
                       t.value == "SHR" || t.value == "SAR"));
      if (!isOpMul && !isKwMul) break;
      int ln = t.line;
      advance();
      auto right = parseUnary();
      auto be    = std::make_unique<BinaryExpr>(t.value, std::move(left),
                                                 std::move(right));
      be->line = ln;
      left = std::move(be);
    }
    return left;
  }

  std::unique_ptr<ExprNode> parseUnary() {
    if (peek().type == TokenType::OPERATOR) {
      const std::string &op = peek().value;
      if (op == "+" || op == "-" || op == "~") {
        int ln = peek().line;
        advance();
        auto ue  = std::make_unique<UnaryExpr>(op, parsePower());
        ue->line = ln;
        return ue;
      }
    }
    return parsePower();
  }

  std::unique_ptr<ExprNode> parsePower() {
    auto left = parsePostfix(); // parsePostfix handles \ field access
    while (peek().type == TokenType::OPERATOR && peek().value == "^") {
      int ln = peek().line;
      advance();
      auto right = parsePostfix();
      auto be    = std::make_unique<BinaryExpr>("^", std::move(left),
                                                 std::move(right));
      be->line = ln;
      left = std::move(be);
    }
    return left;
  }

  // Handles postfix field access: obj\field (chained: a\b\c)
  std::unique_ptr<ExprNode> parsePostfix() {
    auto left = parsePrimary();
    while (peek().type == TokenType::OPERATOR && peek().value == "\\") {
      int ln = peek().line;
      advance(); // consume '\'
      Token fname = expect(TokenType::ID, "Expected field name after \\");
      auto fa  = std::make_unique<FieldAccess>(std::move(left), fname.value);
      fa->line = ln;
      left = std::move(fa);
    }
    return left;
  }

  std::unique_ptr<ExprNode> parsePrimary() {
    Token t = peek();

    // TRUE / FALSE / NULL / NEW
    if (t.type == TokenType::KEYWORD) {
      if (t.value == "TRUE") {
        advance();
        auto le  = std::make_unique<LiteralExpr>(
            Token{TokenType::INT_LIT, "1", t.line, t.col});
        le->line = t.line;
        return le;
      }
      if (t.value == "FALSE" || t.value == "NULL") {
        advance();
        auto le  = std::make_unique<LiteralExpr>(
            Token{TokenType::INT_LIT, "0", t.line, t.col});
        le->line = t.line;
        return le;
      }
      if (t.value == "NEW") {
        advance(); // consume NEW
        Token tn = expect(TokenType::ID, "Expected type name after New");
        auto ne  = std::make_unique<NewExpr>(tn.value);
        ne->line = t.line;
        return ne;
      }
      if (t.value == "FIRST") {
        advance();
        Token tn = expect(TokenType::ID, "Expected type name after First");
        auto fe  = std::make_unique<FirstExpr>(tn.value);
        fe->line = t.line;
        return fe;
      }
      if (t.value == "LAST") {
        advance();
        Token tn = expect(TokenType::ID, "Expected type name after Last");
        auto le2 = std::make_unique<LastExpr>(tn.value);
        le2->line = t.line;
        return le2;
      }
      if (t.value == "BEFORE") {
        advance();
        auto obj = parseExpr();
        auto be2 = std::make_unique<BeforeExpr>(std::move(obj));
        be2->line = t.line;
        return be2;
      }
      if (t.value == "AFTER") {
        advance();
        auto obj = parseExpr();
        auto ae  = std::make_unique<AfterExpr>(std::move(obj));
        ae->line = t.line;
        return ae;
      }
    }

    advance(); // consume token

    if (t.type == TokenType::INT_LIT || t.type == TokenType::FLOAT_LIT ||
        t.type == TokenType::STRING_LIT) {
      auto le  = std::make_unique<LiteralExpr>(t);
      le->line = t.line;
      return le;
    }

    if (t.type == TokenType::ID) {
      // Consume optional type-hint suffix
      if (peek().type == TokenType::OPERATOR &&
          (peek().value == "#" || peek().value == "%" ||
           peek().value == "!" || peek().value == "$"))
        advance();

      // Array access or function/command call: name(args)
      if (peek().type == TokenType::OPERATOR && peek().value == "(") {
        std::string lo = t.value;
        std::transform(lo.begin(), lo.end(), lo.begin(), ::tolower);

        advance(); // (

        if (dimmedArrays.count(lo)) {
          // Array access: arr(i)  or  grid(x, y)
          auto acc  = std::make_unique<ArrayAccess>(t.value);
          acc->line = t.line;
          while (true) {
            acc->indices.push_back(parseExpr());
            if (peek().type == TokenType::OPERATOR && peek().value == ",")
              advance();
            else
              break;
          }
          expect(TokenType::OPERATOR, "Expected ')'", ")");
          return acc;
        } else {
          // Function / built-in call
          auto call  = std::make_unique<CallExpr>(t.value);
          call->line = t.line;
          if (!(peek().type == TokenType::OPERATOR && peek().value == ")")) {
            while (true) {
              call->args.push_back(parseExpr());
              if (peek().type == TokenType::OPERATOR && peek().value == ",")
                advance();
              else
                break;
            }
          }
          expect(TokenType::OPERATOR, "Expected ')'", ")");
          return call;
        }
      }

      auto ve  = std::make_unique<VarExpr>(t.value);
      ve->line = t.line;
      return ve;
    }

    // Parenthesised expression
    if (t.type == TokenType::OPERATOR && t.value == "(") {
      auto expr = parseExpr();
      expect(TokenType::OPERATOR, "Expected ')'", ")");
      return expr;
    }

    // Unexpected token — emit an error and return a safe dummy value
    error(t.line, t.col, "unexpected token '" + t.value + "'");
    auto le  = std::make_unique<LiteralExpr>(
        Token{TokenType::INT_LIT, "0", t.line, t.col});
    le->line = t.line;
    return le;
  }

  // ------------------------------------------------------------------ state
  std::vector<Token>              tokens;
  size_t                          pos;
  std::string                     filename;
  int                             errorCount;
  std::unordered_set<std::string> dimmedArrays; // lowercase names of Dim'd arrays
};

#endif // BLITZNEXT_PARSER_H
