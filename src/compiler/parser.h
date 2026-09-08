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
  static constexpr int kMaxErrors = 20;

  Parser() : pos(0), errorCount(0), tooManyErrors_(false) {}

  std::unique_ptr<Program> parse(const std::vector<Token> &toks,
                                 const SourceMap &map) {
    tokens         = toks;
    pos            = 0;
    map_           = &map;
    errorCount     = 0;
    tooManyErrors_ = false;
    dimmedArrays.clear();
    preScanDims(toks); // forward-reference fix: collect all Dim names first
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
  // ------------------------------------------------------------------ pre-scan

  // Pre-scan pass: walk all tokens and register every Dim'd array name so that
  // forward references work correctly (e.g. an array declared in an Include
  // that appears after the first use, or simply used before its Dim).
  void preScanDims(const std::vector<Token> &toks) {
    for (size_t i = 0; i < toks.size(); ++i) {
      if (toks[i].type != TokenType::KEYWORD || toks[i].value != "DIM")
        continue;
      ++i; // skip DIM
      // Scan one or more "name[hint]( dims )" on this Dim line
      while (i < toks.size() &&
             toks[i].type != TokenType::NEWLINE &&
             toks[i].type != TokenType::EOF_TOKEN) {
        if (toks[i].type != TokenType::ID) break;
        // Register array name (lowercase)
        std::string lo = toks[i].value;
        std::transform(lo.begin(), lo.end(), lo.begin(), ::tolower);
        dimmedArrays.insert(lo);
        ++i;
        // Skip optional type hint (#, %, !, $)
        if (i < toks.size() && toks[i].type == TokenType::OPERATOR &&
            (toks[i].value == "#" || toks[i].value == "%" ||
             toks[i].value == "$"))
          ++i;
        // Skip balanced parentheses: ( dims... )
        if (i < toks.size() && toks[i].type == TokenType::OPERATOR &&
            toks[i].value == "(") {
          int depth = 1;
          ++i;
          while (i < toks.size() && depth > 0) {
            if (toks[i].type == TokenType::OPERATOR && toks[i].value == "(") ++depth;
            else if (toks[i].type == TokenType::OPERATOR && toks[i].value == ")") --depth;
            ++i;
          }
        }
        // Comma between multiple arrays on same Dim line — continue
        if (i < toks.size() && toks[i].type == TokenType::OPERATOR &&
            toks[i].value == ",")
          ++i;
        else
          break;
      }
    }
  }

  // ------------------------------------------------------------------ utils

  // Emits an IDE-parseable diagnostic (GCC format: file:line:col: error: msg)
  void error(int line, int col, const std::string &msg) {
    std::cerr << map_->format(line, col) << ": error: "
              << msg << "\n";
    if (++errorCount == kMaxErrors) {
      std::cerr << map_->mainFile() << ": fatal: too many errors (" << kMaxErrors
                << "), aborting parse.\n";
      tooManyErrors_ = true;
    }
  }

  bool atEnd() const {
    return tooManyErrors_ || pos >= tokens.size() ||
           tokens[pos].type == TokenType::EOF_TOKEN;
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

  // True if the token can continue an expression to its left (binary operator
  // or field separator). Used to tell a parenthesised argument list apart from
  // a parenthesised sub-expression:  Print(a, b)  vs  Print (a + b) * 3
  static bool continuesExpr(const Token &t) {
    if (t.type == TokenType::OPERATOR)
      return t.value == "+"  || t.value == "-"  || t.value == "*" ||
             t.value == "/"  || t.value == "^"  || t.value == "\\" ||
             t.value == "="  || t.value == "<>" || t.value == "<"  ||
             t.value == ">"  || t.value == "<=" || t.value == ">=" ||
             t.value == ",";
    if (t.type == TokenType::KEYWORD)
      return t.value == "AND" || t.value == "OR"  || t.value == "XOR" ||
             t.value == "MOD" || t.value == "SHL" || t.value == "SHR" ||
             t.value == "SAR";
    return false;
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
      if (kw == "FOR")    return parseFor();
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
        int ln = t.line, cl = t.col;
        advance();
        // Accept both "Gosub label" and "Gosub .label"
        if (peek().type == TokenType::OPERATOR && peek().value == ".") advance();
        Token lblTok = expect(TokenType::ID, "Expected label name after Gosub");
        std::string lo = lblTok.value;
        std::transform(lo.begin(), lo.end(), lo.begin(), ::tolower);
        auto s = std::make_unique<GosubStmt>(lo);
        s->line = ln;
        s->col  = cl;
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

      // Optional type-hint suffix on the variable name (x#, s$, n%, f!) and,
      // since BUG-47, the object tag "p.T". The reference reads every variable
      // through parseVar()/parseTypeTag(), so the object tag is legal wherever
      // a scalar tag is: measured against Blitz3D 11.8, "p.T = New T" declares
      // p, "p.T\v = 1" is accepted with the tag in front of the field, and a
      // second, differing tag on the same name is "Variable type mismatch".
      // The tag only supplies the type of a variable that does not exist yet -
      // it never opens a new scope: inside a function "p.T = New T" writes to
      // an existing global p (measured: the store goes to the global slot) and
      // only creates a local when no such global exists.
      std::string assignHint;
      if (peek().type == TokenType::OPERATOR &&
          (peek().value == "#" || peek().value == "%" ||
           peek().value == "$")) {
        assignHint = advance().value; // consume and remember for auto-decl
      } else if (peek().type == TokenType::OPERATOR && peek().value == ".") {
        // Consume-and-expect, the same shape parseFor() uses for its counter
        // tag. A "." that starts a number is a single FLOAT_LIT token since
        // BUG-46, so anything still spelled "." here is a type tag or an error.
        advance(); // consume '.'
        assignHint = "." + expect(TokenType::ID,
                                  "Expected type name after '.'").value;
      }

      // Field assignment: var\field = expr  (Blitz3D \ field separator)
      if (peek().type == TokenType::OPERATOR && peek().value == "\\") {
        advance(); // consume '\'
        Token fname = expect(TokenType::ID, "Expected field name after \\");
        parseOptionalTypeTag(); // "p\f#" - read and dropped (BUG-31)
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
          stmt->typeHint = assignHint; // "a$(0)" - checked against the element
          stmt->line = nameTok.line;
          stmt->col  = nameTok.col;
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
        auto a = std::make_unique<AssignStmt>(nameTok.value, assignHint, std::move(val));
        a->line = nameTok.line;
        return a;
      }

      // Otherwise: command / function call as statement
      auto call = std::make_unique<CallExpr>(nameTok.value);
      call->line = nameTok.line;
      call->col  = nameTok.col;

      // Parenthesised call form: Name(arg1, arg2)  or  Name()
      //
      // The parentheses are only an argument list if the statement ends with
      // the closing ')'. In  Print (1 + 2) * 3  or  Print (First Node)\val
      // the group is merely the start of the first argument, so the tentative
      // parse is rolled back and the un-parenthesised form below re-parses the
      // whole rest of the line as one expression.
      if (peek().type == TokenType::OPERATOR && peek().value == "(") {
        const size_t savedPos    = pos;
        const int    savedErrors = errorCount;
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

        // A single group followed by an operator was a sub-expression, not an
        // argument list — rewind and let the loop below parse it in full.
        if (call->args.size() == 1 && errorCount == savedErrors &&
            continuesExpr(peek())) {
          pos = savedPos;
          call->args.clear();
        } else {
          return call;
        }
      }

      // Blitz3D-style call without parens: Name arg1, arg2
      while (!atEnd() && peek().type != TokenType::NEWLINE &&
             peek().type != TokenType::EOF_TOKEN &&
             !(peek().type == TokenType::OPERATOR && peek().value == ":")) {
        // Break on block-terminator keywords, but allow expression-starter
        // keywords (Not, True, False, Null, New, First, Last, Before, After, Pi,
        // Abs, Sgn, Int, Float, Str).
        if (peek().type == TokenType::KEYWORD) {
          const std::string &kw = peek().value;
          bool isExprStarter = (kw == "NOT"  || kw == "TRUE" || kw == "FALSE" ||
                                kw == "NULL" || kw == "NEW"  || kw == "FIRST" ||
                                kw == "LAST" || kw == "BEFORE" || kw == "AFTER" ||
                                kw == "PI"   || kw == "ABS"  || kw == "SGN" ||
                                kw == "INT"  || kw == "FLOAT" || kw == "STR");
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

    // Unrecognised token. Reporting it is the point: skipping silently meant
    // that "Local f! = 3.14" produced "int var_f = 0;" and the rest of the line
    // simply vanished - no diagnostic, no code, nothing to notice.
    error(t.line, t.col, "unexpected token " + describeToken(t));
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

  // The body of a single-line If runs to the end of the physical line, colons
  // included - the colon separates statements here, it does not end the body.
  // Measured in the reference's own assembler: for
  // "If a=1 Print "x" : Print "y"" a single conditional jump spans both prints,
  // and in "If a=1 Print "x" Else Print "y" : Print "z"" the Else branch holds
  // both of its statements. Reading only one statement, as this used to, left
  // the rest of the line running unconditionally - a silent wrong result.
  std::vector<std::unique_ptr<ASTNode>> parseSingleLineBody() {
    std::vector<std::unique_ptr<ASTNode>> body;
    while (!atEnd() && peek().type != TokenType::NEWLINE) {
      if (peek().type == TokenType::OPERATOR && peek().value == ":") {
        advance(); // separator, not a terminator
        continue;
      }
      // Every block terminator is left for the caller. ELSE/ELSEIF belong to
      // this If; the rest cannot open a statement, and leaving them makes the
      // end-of-line check below reject "If a=1 EndIf" the way the reference
      // does. Swallowing them here would silently accept the line.
      // "END" is deliberately absent: a bare End is the program-end statement
      // and a legal single-line body ("If a=1 End" is accepted by the original,
      // with _fend inside the conditional). The two-word block closers reach us
      // already merged as ENDIF/ENDFUNCTION/... by the lexer.
      std::string kw = peekKw();
      if (kw == "ELSE" || kw == "ELSEIF" || kw == "ENDIF" ||
          kw == "ENDSELECT" || kw == "ENDFUNCTION" || kw == "ENDTYPE" ||
          kw == "CASE" || kw == "DEFAULT" || kw == "WEND" || kw == "UNTIL" ||
          kw == "FOREVER" || kw == "NEXT")
        break;
      size_t before = pos;
      auto s = parseStatement();
      if (s) body.push_back(std::move(s));
      if (pos == before) break; // parseStatement made no progress - do not spin
    }
    return body;
  }

  // Shared between IF and ELSEIF (both parse condition + body + tail).
  std::unique_ptr<IfStmt> parseIfTail() {
    auto cond = parseExpr();
    if (peekKw() == "THEN") advance(); // optional, and never decides the form

    auto stmt = std::make_unique<IfStmt>(std::move(cond));

    // "Then" is optional (BUG-48). Measured against Blitz3D 11.8: the block
    // forms are accepted without it, and so is "If a=1 Print "x"". What decides
    // the form is the token right after the condition and the optional Then -
    // a newline or a colon starts the block form, anything else the single-line
    // form. That is exactly why "If a=1 : Print "x"" demands an EndIf while
    // "If a=1 Print "x"" does not; both measured.
    //   If x = 0 Print "zero"                 → single-line, no EndIf
    //   If x = 0 Then Print "zero" Else …     → single-line
    //   If x = 0 : Print "zero" : End If      → block form (colon like newline)
    bool isColon = (peek().type == TokenType::OPERATOR && peek().value == ":");
    if (peek().type != TokenType::NEWLINE && !isColon && !atEnd()) {
      stmt->thenBlock = parseSingleLineBody();
      // On ElseIf the reference recurses into parseIf() and returns straight
      // away, so the nested If decides its own form and no EndIf is expected
      // here. "If a=1 Print "x" ElseIf a=2 Print "y"" is accepted (measured).
      if (peekKw() == "ELSEIF") {
        advance();
        stmt->elseBlock.push_back(parseIfTail());
        return stmt;
      }
      if (peekKw() == "ELSE") {
        advance();
        stmt->elseBlock = parseSingleLineBody();
      }
      // The reference closes the single-line form with an end-of-line check, so
      // a trailing "EndIf" on the same line is an error there: both
      // "If a=1 EndIf" and "If a=1 Print "x" EndIf" are rejected (measured).
      // Without this the leftover token would be skipped and the program would
      // be accepted.
      if (!atEnd() && peek().type != TokenType::NEWLINE)
        error(peek().line, peek().col,
              "Expected end of line after a single-line If (got '" +
                  peek().value + "')");
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

  // Consumes a type tag if one is there and returns it ("" otherwise).
  // Blitz3D's parseVar() reads a tag after *every* name, including a field
  // name; FieldVarNode::semant then ignores it and takes the type from the
  // field declaration (BUG-31). So the tag is accepted and dropped here too -
  // even a contradicting one, exactly as in the reference.
  std::string parseOptionalTypeTag() {
    if (peek().type == TokenType::OPERATOR &&
        (peek().value == "#" || peek().value == "%" ||
         peek().value == "$"))
      return advance().value;
    return "";
  }

  std::unique_ptr<StmtNode> parseFor() {
    int ln = peek().line;
    advance(); // FOR

    // "For Each p.Punkt" was this project's own spelling; Blitz3D has no such
    // form (BUG-38). Read it to the end anyway, so a program written in the
    // old spelling gets exactly one message per loop instead of a cascade
    // from the '=' that never comes.
    if (peekKw() == "EACH") {
      Token e = peek();
      error(e.line, e.col,
            "'For Each <var>' is not Blitz3D syntax; write "
            "'For <var> = Each <Type>'");
      return parseForEachOldForm(ln);
    }

    Token nameTok = expect(TokenType::ID, "Expected loop variable name");

    // Optional type hint on the loop variable. It is kept, not dropped: the
    // loop variable is an ordinary Blitz3D variable, so its tag decides the
    // type the emitter declares it with (BUG-19).
    std::string hint;
    // parseVar() in the reference reads its tag with parseTypeTag(), which
    // also accepts ".TypeName" - that is where the tag in
    // "For p.Punkt = Each Punkt" comes from (BUG-38).
    std::string objTag;
    Token objTagTok;
    if (peek().type == TokenType::OPERATOR &&
        (peek().value == "#" || peek().value == "%" ||
         peek().value == "$")) {
      hint = peek().value;
      advance();
    } else if (peek().type == TokenType::OPERATOR && peek().value == ".") {
      advance(); // consume '.'
      objTagTok = expect(TokenType::ID, "Expected type name after '.'");
      objTag    = objTagTok.value;
    }

    // In the reference the counter is read with parseVar(), the same function
    // every other variable reference goes through, so an array element and a
    // type field are legal counters too (BUG-30). Both forms are recognised
    // exactly the way the assignment statement recognises them, including the
    // Dim'd-name test that tells "arr(i)" from a call.
    std::unique_ptr<ExprNode> target;
    if (peek().type == TokenType::OPERATOR && peek().value == "\\") {
      advance(); // consume the field separator
      Token fname = expect(TokenType::ID, "Expected field name after \\");
      parseOptionalTypeTag(); // "For p\f# = ..." - read, dropped (BUG-31)
      target = std::make_unique<FieldAccess>(
          std::make_unique<VarExpr>(nameTok.value), fname.value);
    } else if (peek().type == TokenType::OPERATOR && peek().value == "(") {
      if (!dimmedArrays.count(toLower(nameTok.value))) {
        // Without a Dim this reads as a call, and the two errors that follow
        // ("Expected TO", "unexpected token '='") point at the wrong thing.
        // Say what is actually wrong instead.
        error(nameTok.line, nameTok.col,
              "loop variable '" + nameTok.value +
                  "' is used like an array, but no Dim declares it");
      }
      advance(); // consume '('
      auto arr = std::make_unique<ArrayAccess>(nameTok.value);
      arr->typeHint = hint;
      while (true) {
        arr->indices.push_back(parseExpr());
        if (peek().type == TokenType::OPERATOR && peek().value == ",")
          advance();
        else
          break;
      }
      expect(TokenType::OPERATOR, "Expected ')'", ")");
      target = std::move(arr);
    }
    if (target) {
      // Without this every diagnostic about the counter reports line 0.
      target->line = nameTok.line;
      target->col  = nameTok.col;
    }

    expect(TokenType::OPERATOR, "Expected '='", "=");

    // The reference reads the '=' first and only then looks for EACH
    // (parser.cpp, case FOR): "For <var> = Each <Type>".
    if (peekKw() == "EACH") {
      advance(); // EACH
      Token tn = expect(TokenType::ID, "Expected type name after Each");
      if (target)
        error(nameTok.line, nameTok.col,
              "an array element or a field cannot be the index variable of "
              "'For ... = Each'");
      else if (!hint.empty())
        error(nameTok.line, nameTok.col,
              "index variable '" + nameTok.value + hint +
                  "' is a number, but 'Each " + tn.value +
                  "' walks a list of objects");
      else if (!objTag.empty() && toLower(objTag) != toLower(tn.value))
        // ForEachNode::semant compares the two and says "Type mismatch"; the
        // tag and the iterated type have to name the same Type.
        error(objTagTok.line, objTagTok.col,
              "index variable is tagged '." + objTag + "', but the loop walks '" +
                  tn.value + "'");
      auto each  = std::make_unique<ForEachStmt>(nameTok.value, tn.value);
      each->typeTag = objTag;
      each->line = ln;
      each->col  = nameTok.col;
      each->block = parseBlock({"NEXT"});
      expect(TokenType::KEYWORD, "Expected NEXT", "NEXT");
      return each;
    }

    if (!objTag.empty())
      error(objTagTok.line, objTagTok.col,
            "'." + objTag + "' makes the counter an object; a counting For "
            "needs a number");

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
    stmt->typeHint = hint;
    stmt->target   = std::move(target);
    stmt->line  = ln;
    stmt->block = parseBlock({"NEXT"});
    expect(TokenType::KEYWORD, "Expected NEXT", "NEXT");
    return stmt;
  }

  // ------------------------------------------------------------------ SELECT

  std::unique_ptr<SelectStmt> parseSelect() {
    int ln = peek().line;
    int co = peek().col;
    advance(); // SELECT
    auto stmt  = std::make_unique<SelectStmt>(parseExpr());
    stmt->line = ln;
    stmt->col  = co;

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
           peek().value == "$")) {
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
      vd->col  = nameTok.col;
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
           peek().value == "$"))
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
           peek().value == "$"))
        typeHint = advance().value;

      expect(TokenType::OPERATOR, "Expected '='", "=");
      auto val = parseExpr();

      auto cd  = std::make_unique<ConstDecl>(nameTok.value, typeHint,
                                              std::move(val));
      cd->line = nameTok.line;
      cd->col  = nameTok.col;
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

    // Return-type hint on the function name (e.g. Double%, Name$, Make.Vec).
    // It decides the C++ return type of the emitted function, so it must be
    // kept, not just consumed.
    if (peek().type == TokenType::OPERATOR &&
        (peek().value == "#" || peek().value == "%" ||
         peek().value == "$")) {
      func->returnHint = advance().value;
    } else if (peek().type == TokenType::OPERATOR && peek().value == ".") {
      advance(); // consume '.'
      if (peek().type == TokenType::ID)
        func->returnHint = "." + advance().value;
    }

    if (peek().type == TokenType::OPERATOR && peek().value == "(") {
      advance(); // (
      while (!atEnd() &&
             !(peek().type == TokenType::OPERATOR && peek().value == ")")) {
        Token p = expect(TokenType::ID, "Expected parameter name");
        std::string hint;
        if (peek().type == TokenType::OPERATOR &&
            (peek().value == "#" || peek().value == "%" ||
             peek().value == "$")) {
          hint = advance().value;
        } else if (peek().type == TokenType::OPERATOR && peek().value == ".") {
          // Objektparameter "F(p.T)" (BUG-56) - dieselbe Schreibweise, die der
          // Rueckgabetyp oben schon liest. Am Original gemessen: der Parameter
          // ist ein gewoehnlicher Wertparameter und wird - anders als eine
          // lokale oder globale Objektvariable - **nicht** referenzgezaehlt.
          // "p = New T" im Rumpf schreibt dort direkt in den Stack-Slot, ohne
          // _bbObjStore/_bbObjRelease; der Aufrufer sieht die Zuweisung nicht.
          // Ein roher Zeiger als C++-Parameter bildet das genau ab.
          advance(); // consume '.'
          if (peek().type != TokenType::ID) {
            // Nicht ueber expect() melden: das wuerde das folgende ')'
            // mitkonsumieren, die Parameterschleife liefe bis zum Dateiende
            // und haengte acht Folgefehler an eine einzige Ursache.
            error(peek().line, peek().col,
                  "Expected type name after '.' (got '" + peek().value + "')");
            break;
          }
          hint = "." + advance().value;
        }
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
    int ln = peek().line, cl = peek().col;
    advance(); // DELETE

    // "Delete Each <Typ>" loescht alle Objekte eines Typs (BUG-51). Die
    // Referenz verlangt hier einen Typnamen, keinen Ausdruck - gemessen an
    // Blitz3D 11.8 lehnt sie sowohl einen unbekannten Namen als auch eine
    // Objektvariable mit "Specified name is not a NewType name" ab.
    if (peekKw() == "EACH") {
      advance(); // EACH
      Token tn = expect(TokenType::ID, "Expected type name after 'Delete Each'");
      auto s  = std::make_unique<DeleteStmt>(tn.value);
      s->line = ln;
      s->col  = cl;
      return s;
    }

    auto obj = parseExpr();
    auto s   = std::make_unique<DeleteStmt>(std::move(obj));
    s->line  = ln;
    s->col   = cl;
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

  // The rejected "For Each var.Type" spelling, read only so that the loop and
  // everything after it still parse. parseFor() has already reported it and
  // the program will not be emitted; this just keeps the error count at one
  // per loop. FOR is consumed, EACH is not.
  std::unique_ptr<StmtNode> parseForEachOldForm(int ln) {
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
    s->col   = nameTok.col;
    s->block = parseBlock({"NEXT"});
    expect(TokenType::KEYWORD, "Expected NEXT", "NEXT");
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
               peek().value == "$"))
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
         peek().value == "$"))
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

  // Not binds loosest of all, as in Blitz3D: "Not a And b" is "Not (a And b)",
  // not "(Not a) And b". Blitz3D parses NOT only here, at the top of an
  // expression; parseNot() below additionally accepts it in operand position
  // ("a And Not b"), where Blitz3D would want parentheses — accepting more
  // than the reference is harmless, misreading it is not.
  std::unique_ptr<ExprNode> parseExpr() {
    if (peek().type == TokenType::KEYWORD && peek().value == "NOT") {
      int ln = peek().line;
      advance();
      auto ue  = std::make_unique<UnaryExpr>("NOT", parseLogical());
      ue->line = ln;
      return ue;
    }
    return parseLogical();
  }

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
    auto left = parseShift();
    while (peek().type == TokenType::OPERATOR) {
      const std::string &op = peek().value;
      if (op == "+" || op == "-") {
        int ln = peek().line;
        advance();
        auto right = parseShift();
        auto be    = std::make_unique<BinaryExpr>(op, std::move(left),
                                                   std::move(right));
        be->line = ln;
        left = std::move(be);
      } else break;
    }
    return left;
  }

  // Shl / Shr / Sar sit on their own level between + - and * / Mod, as in
  // Blitz3D: "1 Shl 2 * 3" is "1 Shl (2 * 3)", not "(1 Shl 2) * 3".
  std::unique_ptr<ExprNode> parseShift() {
    auto left = parseMultiplicative();
    while (peek().type == TokenType::KEYWORD) {
      const std::string &op = peek().value;
      if (op != "SHL" && op != "SHR" && op != "SAR") break;
      int ln = peek().line;
      advance();
      auto right = parseMultiplicative();
      auto be    = std::make_unique<BinaryExpr>(op, std::move(left),
                                                 std::move(right));
      be->line = ln;
      left = std::move(be);
    }
    return left;
  }

  std::unique_ptr<ExprNode> parseMultiplicative() {
    auto left = parsePower();
    while (true) {
      Token t = peek();
      bool isOpMul = (t.type == TokenType::OPERATOR &&
                      (t.value == "*" || t.value == "/"));
      bool isKwMul = (t.type == TokenType::KEYWORD && t.value == "MOD");
      if (!isOpMul && !isKwMul) break;
      int ln = t.line;
      advance();
      auto right = parsePower();
      auto be    = std::make_unique<BinaryExpr>(t.value, std::move(left),
                                                 std::move(right));
      be->line = ln;
      left = std::move(be);
    }
    return left;
  }

  // A token as it should appear inside a diagnostic. A newline or the end of
  // the stream has no printable text, and putting it in raw would break the
  // one-line file:line:col contract that the IDE parses.
  std::string describeToken(const Token &t) const {
    if (t.type == TokenType::NEWLINE)   return "end of line";
    if (t.type == TokenType::EOF_TOKEN) return "end of file";
    return "'" + t.value + "'";
  }

  std::unique_ptr<ExprNode> parseUnary() {
    // Abs, Sgn, Int, Float and Str are reserved words, not calls. The
    // reference handles them here, in parseUniExpr: Abs/Sgn become a
    // UniExprNode, Int/Float/Str a CastNode, and each takes the following
    // *unary* expression as its operand - "Abs -3" needs no parentheses.
    // A type tag right after the cast word is read and dropped, the way
    // the reference does it (if( toker->next()=='%' ) toker->next();).
    // The operand keeps going through the ordinary builtin path, so the
    // emitted code and the arity check stay what they were for Abs(x).
    if (peek().type == TokenType::KEYWORD) {
      const std::string &kw = peek().value;
      const char *canon = kw == "ABS"   ? "Abs"
                        : kw == "SGN"   ? "Sgn"
                        : kw == "INT"   ? "Int"
                        : kw == "FLOAT" ? "Float"
                        : kw == "STR"   ? "Str" : nullptr;
      if (canon) {
        Token t = advance();
        const char *tag = kw == "INT" ? "%" : kw == "FLOAT" ? "#"
                        : kw == "STR" ? "$" : nullptr;
        if (tag && peek().type == TokenType::OPERATOR && peek().value == tag)
          advance();
        auto call  = std::make_unique<CallExpr>(canon);
        call->line = t.line;
        call->col  = t.col;
        call->args.push_back(parseUnary());
        return call;
      }
    }
    if (peek().type == TokenType::OPERATOR) {
      const std::string &op = peek().value;
      if (op == "+" || op == "-" || op == "~") {
        int ln = peek().line;
        advance();
        auto ue  = std::make_unique<UnaryExpr>(op, parseUnary());
        ue->line = ln;
        return ue;
      }
    }
    return parsePostfix();
  }

  // The power level sits *above* the unary one, as parseExpr6 does in the
  // reference: it takes both of its operands from parseUniExpr, so a sign
  // belongs to the base and "-2 ^ 2" is (-2) ^ 2 = 4, not -(2 ^ 2) (BUG-37).
  std::unique_ptr<ExprNode> parsePower() {
    auto left = parseUnary(); // parseUnary ends in parsePostfix (\ field access)
    while (peek().type == TokenType::OPERATOR && peek().value == "^") {
      int ln = peek().line;
      advance();
      auto right = parseUnary();
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
      parseOptionalTypeTag(); // "p\f#" on the reading side too (BUG-31).
                              // It used to fall through to the statement
                              // parser, which silently dropped it.
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
      if (t.value == "PI") {
        // parsePrimary() in the reference answers the PI token with a
        // plain constant: FloatConstNode( 3.14159...f ). Same digits, and the
        // f keeps it a float: a Blitz3D float is 32 bits wide, and nothing but
        // the emitter ever reads this token's text.
        advance();
        auto le  = std::make_unique<LiteralExpr>(
            Token{TokenType::FLOAT_LIT,
                  "3.1415926535897932384626433832795f", t.line, t.col});
        le->line = t.line;
        le->col  = t.col;
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
        fe->col  = t.col;
        return fe;
      }
      if (t.value == "LAST") {
        advance();
        Token tn = expect(TokenType::ID, "Expected type name after Last");
        auto le2 = std::make_unique<LastExpr>(tn.value);
        le2->line = t.line;
        le2->col  = t.col;
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
      // Type-hint suffix at a use site: it does not change which variable is
      // meant (the name alone does that, as in Blitz3D), but a contradicting
      // tag is an error — so keep it for the semantic pass.
      std::string useHint;
      if (peek().type == TokenType::OPERATOR &&
          (peek().value == "#" || peek().value == "%" ||
           peek().value == "$"))
        useHint = advance().value;

      // Array access or function/command call: name(args)
      if (peek().type == TokenType::OPERATOR && peek().value == "(") {
        std::string lo = t.value;
        std::transform(lo.begin(), lo.end(), lo.begin(), ::tolower);

        advance(); // (

        if (dimmedArrays.count(lo)) {
          // Array access: arr(i)  or  grid(x, y)
          auto acc  = std::make_unique<ArrayAccess>(t.value);
          acc->typeHint = useHint;
          acc->line = t.line;
          acc->col  = t.col;
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
          call->col  = t.col;
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

      auto ve      = std::make_unique<VarExpr>(t.value);
      ve->typeHint = useHint;
      ve->line     = t.line;
      ve->col      = t.col;
      return ve;
    }

    // Parenthesised expression
    if (t.type == TokenType::OPERATOR && t.value == "(") {
      auto expr = parseExpr();
      expect(TokenType::OPERATOR, "Expected ')'", ")");
      return expr;
    }

    // Unexpected token — emit an error and return a safe dummy value
    error(t.line, t.col, "unexpected token " + describeToken(t));
    auto le  = std::make_unique<LiteralExpr>(
        Token{TokenType::INT_LIT, "0", t.line, t.col});
    le->line = t.line;
    return le;
  }

  // ------------------------------------------------------------------ state
  std::vector<Token>              tokens;
  size_t                          pos;
  const SourceMap                *map_ = nullptr;
  int                             errorCount;
  bool                            tooManyErrors_;
  std::unordered_set<std::string> dimmedArrays; // lowercase names of Dim'd arrays
};

#endif // BLITZNEXT_PARSER_H
