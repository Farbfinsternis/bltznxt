#ifndef BLITZNEXT_LEXER_H
#define BLITZNEXT_LEXER_H

#include "sourcemap.h"
#include "token.h"
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Global utility used by lexer, parser and emitter
inline std::string toUpper(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c){ return (char)std::toupper(c); });
  return s;
}

// Blitz3D identifiers and keywords are case-insensitive; this is the canonical
// lower-case form used for lookups and for every generated C++ identifier.
inline std::string toLower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c){ return (char)std::tolower(c); });
  return s;
}

class Lexer {
public:
  // `map` turns the line counted in the preprocessed stream back into the
  // file and line the user actually wrote (WEAK-13). It must outlive the lexer.
  Lexer(const std::string &source, const SourceMap &map)
      : source(source), map(map), pos(0), line(1), col(1), lexErrors_(0) {}

  std::vector<Token> tokenize() {
    std::vector<Token> tokens;
    while (pos < source.length()) {
      char c = source[pos];

      if (std::isspace(c)) {
        if (c == '\n') {
          tokens.push_back({TokenType::NEWLINE, "\n", line, col});
          line++;
          col = 1;
        } else {
          col++;
        }
        pos++;
        continue;
      }

      if (c == ';') { // line comment
        while (pos < source.length() && source[pos] != '\n') {
          pos++;
          col++;
        }
        continue;
      }

      if (std::isalpha(c) || c == '_') {
        Token t = lexIdentifier();
        std::string upper = toUpper(t.value);
        if (isKeyword(upper)) {
          t.type  = TokenType::KEYWORD;
          t.value = upper; // keywords are always stored uppercase
        }
        tokens.push_back(t);
        continue;
      }

      if (std::isdigit(c)) {
        tokens.push_back(lexNumber());
        continue;
      }

      if (c == '"') {
        tokens.push_back(lexString());
        continue;
      }

      if (c == '$') { tokens.push_back(lexHexLiteral()); continue; }
      if (c == '%') { tokens.push_back(lexBinLiteral()); continue; }
      tokens.push_back(lexOperator());
    }
    tokens.push_back({TokenType::EOF_TOKEN, "", line, col});

    // Merge two-word "End X" forms into single compound tokens so that
    // "End If", "End Function", "End Type", "End Select" are equivalent to
    // the single-word forms "EndIf", "EndFunction", "EndType", "EndSelect".
    // Requirement: both tokens on the same source line, no newline between them.
    static const std::unordered_map<std::string,std::string> kEndMerge = {
      {"IF",       "ENDIF"},
      {"FUNCTION", "ENDFUNCTION"},
      {"TYPE",     "ENDTYPE"},
      {"SELECT",   "ENDSELECT"},
    };
    for (size_t i = 0; i + 1 < tokens.size(); ++i) {
      if (tokens[i].type  == TokenType::KEYWORD &&
          tokens[i].value == "END"              &&
          tokens[i+1].type == TokenType::KEYWORD &&
          tokens[i+1].line == tokens[i].line) {
        auto it = kEndMerge.find(tokens[i+1].value);
        if (it != kEndMerge.end()) {
          tokens[i].value = it->second;
          tokens.erase(tokens.begin() + i + 1);
          // don't advance i — recheck the new tokens[i+1]
        }
      }
    }

    return tokens;
  }

private:
  Token lexIdentifier() {
    int startCol = col;
    std::string value;
    while (pos < source.length() &&
           (std::isalnum(source[pos]) || source[pos] == '_')) {
      value += source[pos++];
      col++;
    }
    return {TokenType::ID, value, line, startCol};
  }

  Token lexNumber() {
    int startCol = col;
    std::string value;
    bool isFloat = false;
    bool hasDot  = false;
    while (pos < source.length() &&
           (std::isdigit(source[pos]) || (source[pos] == '.' && !hasDot))) {
      if (source[pos] == '.') {
        hasDot  = true;
        isFloat = true;
      }
      value += source[pos++];
      col++;
    }
    return {isFloat ? TokenType::FLOAT_LIT : TokenType::INT_LIT, value, line,
            startCol};
  }

  // Converts the digit part of a hex/bin literal into a Blitz3D integer.
  // Blitz3D integers are 32 bit and wrap, so $FFFFFFFF is -1. Anything that
  // does not fit in 32 bits (or overflows the conversion itself) is reported
  // as an error and yields 0 — std::stol used to throw here and abort the
  // whole compiler with exit code 3.
  std::string literalToInt(const std::string &digits, int base,
                           const char *kind, char sigil, int startCol) {
    unsigned long long val = 0;
    bool tooBig = false;
    try {
      val = std::stoull(digits, nullptr, base);
    } catch (const std::exception &) {
      tooBig = true;
    }
    if (tooBig || val > 0xFFFFFFFFull) {
      std::cerr << map.format(line, startCol)
                << ": error: " << kind << " literal " << sigil << digits
                << " does not fit in a 32-bit integer\n";
      ++lexErrors_;
      return "0";
    }
    return std::to_string(
        static_cast<int>(static_cast<unsigned int>(val)));
  }

  // $FF, $1A2B etc. — Blitz3D hex literals.
  // Falls back to OPERATOR "$" if not followed by a hex digit (e.g. string type-hint a$).
  Token lexHexLiteral() {
    int startCol = col;
    pos++; col++; // skip '$'
    std::string digits;
    while (pos < source.length() && std::isxdigit((unsigned char)source[pos])) {
      digits += source[pos++];
      col++;
    }
    if (digits.empty())
      return {TokenType::OPERATOR, "$", line, startCol};
    return {TokenType::INT_LIT,
            literalToInt(digits, 16, "hex", '$', startCol), line, startCol};
  }

  // %1010 etc. — Blitz3D binary literals.
  // Falls back to OPERATOR "%" if not followed by 0/1 (e.g. integer type-hint a%, Mod operator).
  Token lexBinLiteral() {
    int startCol = col;
    pos++; col++; // skip '%'
    std::string digits;
    while (pos < source.length() && (source[pos] == '0' || source[pos] == '1')) {
      digits += source[pos++];
      col++;
    }
    if (digits.empty())
      return {TokenType::OPERATOR, "%", line, startCol};
    return {TokenType::INT_LIT,
            literalToInt(digits, 2, "binary", '%', startCol), line, startCol};
  }

  Token lexString() {
    int startCol = col;
    std::string value;
    pos++; col++; // skip opening "
    while (pos < source.length() && source[pos] != '"' && source[pos] != '\n') {
      value += source[pos++];
      col++;
    }
    if (pos < source.length() && source[pos] == '"') {
      pos++; col++; // skip closing "
    } else {
      std::cerr << map.format(line, startCol)
                << ": error: unclosed string literal\n";
      ++lexErrors_;
    }
    return {TokenType::STRING_LIT, value, line, startCol};
  }

  Token lexOperator() {
    int startCol = col;
    std::string value;
    char c = source[pos++];
    value += c;
    col++;

    if (pos < source.length()) {
      char next = source[pos];
      if ((c == '<' && (next == '=' || next == '>')) ||
          (c == '>' && next == '=') ||
          (c == ':' && next == '=')) {
        value += source[pos++];
        col++;
      }
    }

    return {TokenType::OPERATOR, value, line, startCol};
  }

  bool isKeyword(const std::string &val) {
    static const std::unordered_set<std::string> keywords = {
        // Control flow
        "IF",       "THEN",     "ELSE",     "ELSEIF",   "ENDIF",
        "SELECT",      "ENDSELECT",                         // EndSelect alias
        "CASE",        "DEFAULT",  "END",
        "REPEAT",   "UNTIL",    "FOREVER",
        "WHILE",    "WEND",
        "FOR",      "TO",       "STEP",     "NEXT",
        "EXIT",     "GOTO",     "GOSUB",    "RETURN",
        // Functions & scope
        "FUNCTION", "ENDFUNCTION",                        // EndFunction alias
        "CONST",    "GLOBAL",   "LOCAL",    "DIM",
        // Types
        "TYPE",     "ENDTYPE",                            // EndType alias
        "FIELD",    "NEW",      "DELETE",
        "EACH",     "FIRST",    "LAST",     "BEFORE",   "AFTER",
        "INSERT",
        // Data
        "DATA",     "READ",     "RESTORE",  "INCLUDE",
        // Literals. Pi is a reserved word in Blitz3D, not an identifier:
        // toker.cpp registers it next to True/False (alphaTokes["Pi"]=PI),
        // so it can never be declared or assigned to.
        "TRUE",     "FALSE",    "NULL",     "PI",
        // Operators (these MUST be keywords so the parser sees KEYWORD type)
        "AND",      "OR",       "XOR",      "NOT",
        "MOD",      "SHL",      "SHR",      "SAR",
        // Unary operators and casts. Reserved words in Blitz3D, not calls:
        // parseUniExpr builds UniExprNode for Abs/Sgn and CastNode for
        // Int/Float/Str, each over a following unary expression (BUG-36).
        "ABS",      "SGN",
        "INT",      "FLOAT",    "STR"
    };
    return keywords.count(val) > 0;
  }

  std::string      source;
  const SourceMap &map;
  size_t pos;
  int line, col;
  int lexErrors_;

public:
  bool hasErrors() const { return lexErrors_ > 0; }
};

#endif // BLITZNEXT_LEXER_H
