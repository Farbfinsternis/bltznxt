#include <fstream>
#include <iostream>
#include <string>

#include "cpp_generator.h"
#include "ex.h"
#include "memory.h"
#include "parser.h"
#include "prognode.h"
#include "toker.h"

int main(int argc, char *argv[]) {
  std::cerr << "BltzNxt Transpiler v0.6.0" << std::endl;

  if (argc < 2) {
    std::cerr << "Usage: bbc_cpp <input.bb>" << std::endl;
    std::cerr << "Options:" << std::endl;
    std::cerr << "  --keywords    List all registered Blitz3D keywords"
              << std::endl;
    return 1;
  }

  std::string arg1 = argv[1];
  if (arg1 == "--keywords") {
#define CMD(name, ret, params) std::cout << #name << std::endl;
#define CMATH CMD
#define CSTR CMD
#include "../commands.def"
#undef CMD
#undef CMATH
#undef CSTR
    return 0;
  }

  std::string inFile = arg1;
  std::cerr << "Compiling: " << inFile << std::endl;

  std::ifstream in(inFile);
  if (!in) {
    std::cerr << "Error: Could not open file " << inFile << std::endl;
    return 1;
  }

  try {
    std::cerr << "Debug: Initializing Toker..." << std::endl;
    Toker toker(in);
    std::cerr << "Debug: Initializing Parser..." << std::endl;
    Parser parser(toker);
    std::cerr << "Debug: Starting Parsing..." << std::endl;
    ProgNode *prog = parser.parse(inFile);
    std::cerr << "Parser completed." << std::endl;

    // Run Semantic Analysis
    BCEnviron *globalEnv = d_new<BCEnviron>("", Type::int_type, 0, nullptr);

    // Register built-in functions from commands.def (Single Source of Truth)
    auto makeDefault = [](Type *t, const char *vStr) -> ConstType * {
      if (t == Type::int_type)
        return d_new<ConstType>(std::atoi(vStr));
      if (t == Type::float_type)
        return d_new<ConstType>((float)std::atof(vStr));
      if (t == Type::string_type) {
        std::string s = vStr;
        if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
          s = s.substr(1, s.size() - 2);
        return d_new<ConstType>(s);
      }
      return nullptr;
    };

#define T_INT Type::int_type
#define T_FLOAT Type::float_type
#define T_STRING Type::string_type
#define T_VOID Type::void_type
#define P(n, t) d->insertDecl(#n, t, DECL_PARAM);
#define OP(n, t, v) d->insertDecl(#n, t, DECL_PARAM, makeDefault(t, #v));
#define CMD(name, ret, params)                                                 \
  {                                                                            \
    DeclSeq *d = d_new<DeclSeq>();                                             \
    params globalEnv->funcDecls->insertDecl(                                   \
        #name, d_new<FuncType>(ret, d, false, false), DECL_FUNC);              \
  }
#define CMATH CMD
#define CSTR CMD
#include "../commands.def"
#undef CMD
#undef CMATH
#undef CSTR
#undef P
#undef OP
#undef T_INT
#undef T_FLOAT
#undef T_STRING
#undef T_VOID

    std::cerr << "Running Semantic Analysis..." << std::endl;
    prog->semant(globalEnv);
    std::cerr << "Semantic Analysis completed." << std::endl;

    // Generate C++ code to stdout
    CppGenerator gen(std::cout);
    std::cerr << "Generating Code..." << std::endl;
    gen.generate(prog);
    std::cerr << "Code Generation completed." << std::endl;

    // delete prog; // Handled by MemoryManager
  } catch (const Ex &e) {
    if (e.pos >= 0) {
      std::cerr << e.file << ":" << (e.line() + 1) << ":" << (e.column() + 1)
                << ": error: " << e.ex << std::endl;
    } else {
      std::cerr << "Error: " << e.ex << std::endl;
    }
    return 1;
  } catch (...) {
    std::cerr << "Unknown error occurred." << std::endl;
    return 1;
  }

  // MemoryManager::cleanup(); // Using cerr for logging now, which is fine.
  // Actually, run_tests.py redirects stdout to file.
  // MemoryManager writes to cerr.
  // main.cpp writes to stdout. i need to remove stdout writes.

  MemoryManager::cleanup();
  return 0;
}
