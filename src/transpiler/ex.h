
#ifndef EX_H
#define EX_H
#include <string>

struct Ex {
  std::string ex; // what happened
  int pos;        // source offset (encoded line/column)
  std::string file;
  Ex(const std::string &ex) : ex(ex), pos(-1) {}
  Ex(const std::string &ex, int pos, const std::string &t)
      : ex(ex), pos(pos), file(t) {}

  int line() const { return (pos >> 16); }
  int column() const { return (pos & 0xffff); }
};

#endif