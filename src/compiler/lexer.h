#ifndef BLITZNEXT_LEXER_H
#define BLITZNEXT_LEXER_H

#include "token.h"
#include <algorithm>
#include <cctype>
#include <string>
#include <unordered_set>
#include <vector>

// Global utility used by lexer, parser and emitter
inline std::string toUpper(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c){ return (char)std::toupper(c); });
  return s;
}

class Lexer {
public:
  Lexer(const std::string &source, const std::string &filename = "")
      : source(source), filename(filename), pos(0), line(1), col(1), lexErrors_(0) {}

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
    long val = std::stol(digits, nullptr, 16);
    return {TokenType::INT_LIT, std::to_string(val), line, startCol};
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
    long val = std::stol(digits, nullptr, 2);
    return {TokenType::INT_LIT, std::to_string(val), line, startCol};
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
      std::cerr << filename << ":" << line << ":" << startCol
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
        // Literals
        "TRUE",     "FALSE",    "NULL",
        // Operators (these MUST be keywords so the parser sees KEYWORD type)
        "AND",      "OR",       "XOR",      "NOT",
        "MOD",      "SHL",      "SHR",      "SAR"
    };
    return keywords.count(val) > 0;
  }

  std::string source;
  std::string filename;
  size_t pos;
  int line, col;
  int lexErrors_;

public:
  bool hasErrors() const { return lexErrors_ > 0; }
};

#endif // BLITZNEXT_LEXER_H
