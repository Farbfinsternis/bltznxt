
#ifndef CODEGEN_H
#define CODEGEN_H

#include "std.h"

// BltzNext Codegen Abstract Base Class
// Replaced legacy IR system with direct C++ generation interface.

class Codegen {
public:
  std::ostream &out;
  Codegen(std::ostream &out) : out(out) {}
  virtual ~Codegen() {}
};

#endif