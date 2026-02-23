#ifndef BLITZNEXT_TOKEN_H
#define BLITZNEXT_TOKEN_H

#include <string>

enum class TokenType {
  NONE,
  ID,
  STRING_LIT,
  INT_LIT,
  FLOAT_LIT,
  KEYWORD,
  OPERATOR,
  NEWLINE,
  EOF_TOKEN,
  UNKNOWN
};

struct Token {
  TokenType type;
  std::string value;
  int line;
  int col;
};

#endif // BLITZNEXT_TOKEN_H
