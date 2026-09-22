#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "commands.h"
#include "emitter.h"
#include "lexer.h"
#include "parser.h"
#include "preprocessor.h"
#include "semant.h"
#include "sourcemap.h"
#include "suggest.h"
#include "token.h"

// Include last: windows.h macros (BOOL, ERROR, min/max, ...) must not
// pollute the project headers above.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace fs = std::filesystem;

// ---- Version ---------------------------------------------------------------
// Single source of truth for -h and -v output (Buglist BUG-02 / REFACTOR R12).

static constexpr const char *kVersion = "0.5.5";

// ---- Config ----------------------------------------------------------------

struct Config {
  bool        compileOnly = false;
  bool        debug       = false;
  bool        quiet       = false;
  std::string outputName;
  std::string inputPath;
};

// ---- Path helper -----------------------------------------------------------
// Search order:
//   1. CWD-relative
//   2. exe-dir-relative          (blitzcc.exe is in bin/, so bin/rel)
//   3. exe-dir/../relative       (one level up from bin/ = project root)
//   4. "../" CWD-relative        (legacy fallback)
//   5. $BLITZPATH-relative
static fs::path g_exeDir_;

static fs::path resolvePath(const std::string &rel) {
  fs::path p = fs::absolute(rel);
  if (fs::exists(p)) return p;
  if (!g_exeDir_.empty()) {
    fs::path p2 = g_exeDir_ / rel;
    if (fs::exists(p2)) return p2;
    fs::path p3 = g_exeDir_ / ".." / rel;
    if (fs::exists(p3)) return p3;
  }
  fs::path p4 = fs::absolute("../" + rel);
  if (fs::exists(p4)) return p4;
  const char *bp = std::getenv("BLITZPATH");
  if (bp) {
    fs::path p5 = fs::path(bp) / rel;
    if (fs::exists(p5)) return p5;
  }
  return p; // not found — caller checks existence
}


// -k  → one name per line
// +k  → name(sig) per line
static void listCommands(bool withSigs) {
  for (const auto &c : kCommands) {
    if (withSigs)
      std::cout << c.name << "(" << commandSignature(c) << ")\n";
    else
      std::cout << c.name << "\n";
  }
}

// ---- Semantic check: unknown calls (WEAK-03, Stufe 1 und 2) ----------------
//
// Walks the entire AST and reports any CallExpr whose name is not in
// kCommands[] and is not a user-defined Function. A name that is close to
// a known one is named in the message ("did you mean ...?"), which is the
// most common case by far: a typo in a command name. Type and arity
// checking is not here but in the Analyzer (semant.h).

static void collectCallsExpr(const ExprNode *e,
                              std::vector<const CallExpr *> &out);
static void collectCallsBlock(const std::vector<std::unique_ptr<ASTNode>> &blk,
                              std::vector<const CallExpr *> &out);

static void collectCallsExpr(const ExprNode *e,
                              std::vector<const CallExpr *> &out) {
  if (!e) return;
  if (auto *ce = dynamic_cast<const CallExpr *>(e)) {
    out.push_back(ce);
    for (auto &a : ce->args) collectCallsExpr(a.get(), out);
  } else if (auto *be = dynamic_cast<const BinaryExpr *>(e)) {
    collectCallsExpr(be->left.get(), out);
    collectCallsExpr(be->right.get(), out);
  } else if (auto *ue = dynamic_cast<const UnaryExpr *>(e)) {
    collectCallsExpr(ue->expr.get(), out);
  } else if (auto *fa = dynamic_cast<const FieldAccess *>(e)) {
    collectCallsExpr(fa->object.get(), out);
  } else if (auto *va = dynamic_cast<const VectorAccess *>(e)) {
    collectCallsExpr(va->base.get(), out);
    collectCallsExpr(va->index.get(), out);
  } else if (auto *aa = dynamic_cast<const ArrayAccess *>(e)) {
    for (auto &idx : aa->indices) collectCallsExpr(idx.get(), out);
  } else if (auto *bef = dynamic_cast<const BeforeExpr *>(e)) {
    collectCallsExpr(bef->object.get(), out);
  } else if (auto *aft = dynamic_cast<const AfterExpr *>(e)) {
    collectCallsExpr(aft->object.get(), out);
  }
  // LiteralExpr, VarExpr, NewExpr, FirstExpr, LastExpr — no sub-exprs
}

static void collectCallsNode(const ASTNode *node,
                              std::vector<const CallExpr *> &out) {
  if (!node) return;
  if (auto *ce = dynamic_cast<const CallExpr *>(node)) {
    out.push_back(ce);
    for (auto &a : ce->args) collectCallsExpr(a.get(), out);
  } else if (auto *vd = dynamic_cast<const VarDecl *>(node)) {
    collectCallsExpr(vd->initValue.get(), out);
  } else if (auto *as = dynamic_cast<const AssignStmt *>(node)) {
    collectCallsExpr(as->value.get(), out);
  } else if (auto *aas = dynamic_cast<const ArrayAssignStmt *>(node)) {
    for (auto &idx : aas->indices) collectCallsExpr(idx.get(), out);
    collectCallsExpr(aas->value.get(), out);
  } else if (auto *fas = dynamic_cast<const FieldAssignStmt *>(node)) {
    collectCallsExpr(fas->object.get(), out);
    collectCallsExpr(fas->value.get(), out);
  } else if (auto *vas = dynamic_cast<const VectorAssignStmt *>(node)) {
    collectCallsExpr(vas->base.get(), out);
    collectCallsExpr(vas->index.get(), out);
    collectCallsExpr(vas->value.get(), out);
  } else if (auto *is = dynamic_cast<const IfStmt *>(node)) {
    collectCallsExpr(is->condition.get(), out);
    collectCallsBlock(is->thenBlock, out);
    collectCallsBlock(is->elseBlock, out);
  } else if (auto *ws = dynamic_cast<const WhileStmt *>(node)) {
    collectCallsExpr(ws->condition.get(), out);
    collectCallsBlock(ws->block, out);
  } else if (auto *rs = dynamic_cast<const RepeatStmt *>(node)) {
    collectCallsExpr(rs->condition.get(), out);
    collectCallsBlock(rs->block, out);
  } else if (auto *fs = dynamic_cast<const ForStmt *>(node)) {
    collectCallsExpr(fs->start.get(), out);
    collectCallsExpr(fs->end.get(), out);
    collectCallsExpr(fs->step.get(), out);
    collectCallsBlock(fs->block, out);
  } else if (auto *ss = dynamic_cast<const SelectStmt *>(node)) {
    collectCallsExpr(ss->expr.get(), out);
    for (auto &c : ss->cases) {
      for (auto &ex : c.expressions) collectCallsExpr(ex.get(), out);
      collectCallsBlock(c.block, out);
    }
    collectCallsBlock(ss->defaultBlock, out);
  } else if (auto *fes = dynamic_cast<const ForEachStmt *>(node)) {
    collectCallsBlock(fes->block, out);
  } else if (auto *fd = dynamic_cast<const FunctionDecl *>(node)) {
    collectCallsBlock(fd->body, out);
  } else if (auto *ret = dynamic_cast<const ReturnStmt *>(node)) {
    collectCallsExpr(ret->value.get(), out);
  } else if (auto *cd = dynamic_cast<const ConstDecl *>(node)) {
    collectCallsExpr(cd->value.get(), out);
  } else if (auto *ds = dynamic_cast<const DimStmt *>(node)) {
    for (auto &d : ds->dims) collectCallsExpr(d.get(), out);
  } else if (auto *del = dynamic_cast<const DeleteStmt *>(node)) {
    collectCallsExpr(del->object.get(), out);
  } else if (auto *ins = dynamic_cast<const InsertStmt *>(node)) {
    collectCallsExpr(ins->object.get(), out);
    collectCallsExpr(ins->target.get(), out);
  } else if (auto *pr = dynamic_cast<const Program *>(node)) {
    // "Local x = f()" is wrapped in a Program node so that "Local x, y" fits
    // one statement — without this branch every call in a Local/Global
    // initialiser escaped the check and only g++ complained.
    collectCallsBlock(pr->nodes, out);
  }
  // ExitStmt, EndStmt, LabelStmt, GotoStmt, GosubStmt,
  // DataStmt, ReadStmt, RestoreStmt, TypeDecl — nothing to walk
}

static void collectCallsBlock(const std::vector<std::unique_ptr<ASTNode>> &blk,
                              std::vector<const CallExpr *> &out) {
  for (auto &s : blk) collectCallsNode(s.get(), out);
}

// ---- Userlibs: deklariert, aber nicht unterstuetzt -------------------------
//
// Blitz3D erweitert seinen Befehlssatz ueber 32-Bit-Windows-DLLs, die in
// userlibs/*.decls deklariert werden. BlitzNext unterstuetzt das nicht und
// wird es nicht: Begruendung und der Plan fuer einen Ersatz stehen in
// VISION.md, der Eintrag fuer Nutzer in KNOWN_ISSUES.md. Ohne diese Pruefung
// scheitert so ein Programm an einem gewoehnlichen "unknown function or
// command" - richtig, aber nichtssagend. Deshalb werden die Deklarationen
// gelesen (nur ihre Namen) und ein passender Aufruf bekommt gesagt, was
// wirklich los ist.
//
// Format laut userlibs/UserLibs.txt: eine Zeile '.lib "name.dll"' eroeffnet
// die Datei, jede weitere deklariert eine Funktion wie in Blitz, optional
// gefolgt von :"dekorierter Name". Hier zaehlt nur der Name vor der Klammer,
// ohne sein Typkuerzel.

static const std::unordered_map<std::string, std::string> &userlibDeclNames() {
  static const std::unordered_map<std::string, std::string> names = [] {
    std::unordered_map<std::string, std::string> m;
    std::error_code ec;
    fs::path dir = resolvePath("userlibs");
    if (!fs::is_directory(dir, ec)) return m;
    for (const auto &entry : fs::directory_iterator(dir, ec)) {
      if (ec) break;
      if (!entry.is_regular_file(ec)) continue;
      std::string ext = entry.path().extension().string();
      std::transform(ext.begin(), ext.end(), ext.begin(),
                     [](unsigned char c) { return (char)std::tolower(c); });
      if (ext != ".decls") continue;
      std::ifstream in(entry.path());
      std::string   line;
      while (std::getline(in, line)) {
        size_t i = line.find_first_not_of(" \t\r");
        if (i == std::string::npos) continue;
        if (line[i] == ';' || line[i] == '.') continue;   // Kommentar, .lib
        size_t j = i;
        while (j < line.size() &&
               (std::isalnum((unsigned char)line[j]) || line[j] == '_')) ++j;
        if (j == i) continue;
        std::string name = line.substr(i, j - i);
        // Ein Typkuerzel (% # $) steht zwischen Name und Klammer.
        while (j < line.size() && (line[j] == '%' || line[j] == '#' ||
                                   line[j] == '$')) ++j;
        while (j < line.size() && (line[j] == ' ' || line[j] == '\t')) ++j;
        if (j >= line.size() || line[j] != '(') continue;
        m.emplace(toUpper(name), entry.path().filename().string());
      }
    }
    return m;
  }();
  return names;
}

// Returns number of errors emitted (0 = clean).
static int checkCalls(const Program *prog, const SourceMap &map) {
  // Build known-name set: all built-in commands + user-defined functions.
  // The spelled-out names are kept alongside, in this fixed order, so a
  // suggestion can be printed the way the table or the source writes it.
  std::unordered_set<std::string> known;
  std::vector<std::string>        knownNames;
  for (const auto &c : kCommands) {
    known.insert(toUpper(c.name));
    knownNames.push_back(c.name);
  }
  for (const auto &s : prog->nodes)
    if (auto *fd = dynamic_cast<const FunctionDecl *>(s.get())) {
      known.insert(toUpper(fd->name));
      knownNames.push_back(fd->name);
    }

  std::vector<const CallExpr *> calls;
  collectCallsBlock(prog->nodes, calls);

  int errors = 0;
  for (const auto *ce : calls) {
    if (known.count(toUpper(ce->name)) == 0) {
      const auto &ul = userlibDeclNames();
      auto       it = ul.find(toUpper(ce->name));
      if (it != ul.end()) {
        std::cerr << map.format(ce->line, std::max(1, ce->col))
                  << ": error: '" << ce->name << "' is declared in userlibs/"
                  << it->second
                  << " - userlibs are not supported, see KNOWN_ISSUES.md\n";
        ++errors;
        continue;
      }
      std::string hint = didYouMean(ce->name, knownNames);
      std::cerr << map.format(ce->line, std::max(1, ce->col))
                << ": error: unknown function or command '" << ce->name
                << "'" << hint << "\n";
      ++errors;
    }
  }
  return errors;
}

// ---- Semantic check: Gosub inside a function -------------------------------
//
// Gosub is a main-program construct: a bare "Return" inside a function returns
// from the function (Milestone 11), so a subroutine inside a function has no
// way to spell its own return. The emitter would produce C++ that does not
// compile — __gosub_ret__ and the dispatch switch exist only in main() — so
// this is reported here as one clear error instead of three g++ messages
// against generated code the user never wrote.

static void collectGosubsBlock(const std::vector<std::unique_ptr<ASTNode>> &blk,
                               std::vector<const GosubStmt *> &out);

// Does not descend into nested FunctionDecls — each is checked on its own.
static void collectGosubsNode(const ASTNode *node,
                              std::vector<const GosubStmt *> &out) {
  if (!node) return;
  if (auto *gs = dynamic_cast<const GosubStmt *>(node)) {
    out.push_back(gs);
  } else if (auto *is = dynamic_cast<const IfStmt *>(node)) {
    collectGosubsBlock(is->thenBlock, out);
    collectGosubsBlock(is->elseBlock, out);
  } else if (auto *ws = dynamic_cast<const WhileStmt *>(node)) {
    collectGosubsBlock(ws->block, out);
  } else if (auto *rs = dynamic_cast<const RepeatStmt *>(node)) {
    collectGosubsBlock(rs->block, out);
  } else if (auto *fs = dynamic_cast<const ForStmt *>(node)) {
    collectGosubsBlock(fs->block, out);
  } else if (auto *ss = dynamic_cast<const SelectStmt *>(node)) {
    for (auto &c : ss->cases) collectGosubsBlock(c.block, out);
    collectGosubsBlock(ss->defaultBlock, out);
  } else if (auto *fes = dynamic_cast<const ForEachStmt *>(node)) {
    collectGosubsBlock(fes->block, out);
  } else if (auto *pr = dynamic_cast<const Program *>(node)) {
    collectGosubsBlock(pr->nodes, out);
  }
}

static void collectGosubsBlock(const std::vector<std::unique_ptr<ASTNode>> &blk,
                               std::vector<const GosubStmt *> &out) {
  for (auto &s : blk) collectGosubsNode(s.get(), out);
}

// Returns number of errors emitted (0 = clean).
static int checkGosubScope(const std::vector<std::unique_ptr<ASTNode>> &nodes,
                           const SourceMap &map) {
  int errors = 0;
  for (const auto &n : nodes) {
    if (auto *pr = dynamic_cast<const Program *>(n.get())) {
      errors += checkGosubScope(pr->nodes, map); // included file
      continue;
    }
    auto *fd = dynamic_cast<const FunctionDecl *>(n.get());
    if (!fd) continue;
    std::vector<const GosubStmt *> gosubs;
    collectGosubsBlock(fd->body, gosubs);
    for (const auto *gs : gosubs) {
      std::cerr << map.format(gs->line, std::max(1, gs->col))
                << ": error: Gosub is not "
                << "allowed inside a function ('" << fd->name << "') - a bare "
                << "Return there returns from the function. Move the subroutine "
                << "into the main program or make it a function.\n";
      ++errors;
    }
  }
  return errors;
}

// ---- Transpiler ------------------------------------------------------------

class Transpiler {
public:
  bool compile(const std::string &cppPath, const std::string &outputPath,
               bool debug) const {
    fs::path gppPath = resolvePath("tools/mingw64/bin/g++.exe");
    if (!fs::exists(gppPath)) {
      std::cerr << "[ERROR] g++ not found at " << gppPath << "\n";
      return false;
    }

    fs::path includeDir  = resolvePath("src/compiler");
    fs::path sdlBase     = resolvePath("libs/sd3/x86_64-w64-mingw32");
    fs::path sdlInc      = sdlBase / "include";
    fs::path libDir      = sdlBase / "lib";
    fs::path sdlImport   = libDir / "libSDL3.dll.a";

    // SDL3_ttf (optional — present after build_windows.bat has run)
    fs::path ttfBase     = resolvePath("libs/sdl3_ttf/x86_64-w64-mingw32");
    fs::path ttfInc      = ttfBase / "include";
    fs::path ttfImport   = ttfBase / "lib" / "libSDL3_ttf.dll.a";
    const bool haveTtf   = fs::exists(ttfImport);

    std::string gpp      = gppPath.make_preferred().string();
    std::string finalCpp = fs::absolute(cppPath).make_preferred().string();
    std::string finalOut =
        fs::absolute(outputPath + ".exe").make_preferred().string();
    std::string incDir    = includeDir.make_preferred().string();
    std::string sdlIncDir = sdlInc.make_preferred().string();

    std::string cmd = "\"" + gpp + "\" -std=c++17 -static"
                      " -I\"" + incDir + "\""
                      " -I\"" + sdlIncDir + "\"";

    if (haveTtf)
      cmd += " -I\"" + ttfInc.make_preferred().string() + "\""
             " -DBB_HAS_SDL3_TTF";

    cmd += " \"" + finalCpp + "\""
           " -o \"" + finalOut + "\"";

    // Link SDL3 (required for graphics/audio/input)
    if (fs::exists(sdlImport))
      cmd += " \"" + sdlImport.make_preferred().string() + "\"";

    // Link SDL3_ttf when available
    if (haveTtf)
      cmd += " \"" + ttfImport.make_preferred().string() + "\"";

    // Link winmm for timeBeginPeriod/timeEndPeriod (high-res timer on Windows)
    cmd += " -lwinmm";

    // Winsock fuer die TCP-Befehle (bb_socket.h)
    cmd += " -lws2_32";

    // Link opengl32 for OpenGL (3D programs via bb_gl_ctx.h)
    cmd += " -lopengl32";

    if (debug) cmd += " -g";

    // Launch g++ directly via CreateProcessW — no shell, no injection risk.
    std::wstring wcmd(cmd.begin(), cmd.end());
    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};
    if (!CreateProcessW(nullptr, wcmd.data(),
                        nullptr, nullptr, FALSE, 0, nullptr, nullptr,
                        &si, &pi)) {
      std::cerr << "[ERROR] CreateProcessW failed (code " << GetLastError() << ")\n";
      return false;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode = 1;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    if (exitCode != 0) return false;

    // Copy SDL3.dll next to the executable (non-fatal if absent)
    fs::path sdlDll = resolvePath("libs/sd3/x86_64-w64-mingw32/bin/SDL3.dll");
    if (fs::exists(sdlDll)) {
      try {
        fs::copy_file(sdlDll,
                      fs::path(outputPath).parent_path() / "SDL3.dll",
                      fs::copy_options::overwrite_existing);
      } catch (...) {}
    }

    // Copy SDL3_ttf.dll next to the executable (non-fatal if absent)
    fs::path ttfDll = ttfBase / "bin" / "SDL3_ttf.dll";
    if (fs::exists(ttfDll)) {
      try {
        fs::copy_file(ttfDll,
                      fs::path(outputPath).parent_path() / "SDL3_ttf.dll",
                      fs::copy_options::overwrite_existing);
      } catch (...) {}
    }

    return true;
  }

  // Returns: 0 = success, 1 = parse error, 2 = compile error
  int transpile(const Config &cfg) const {
    std::string output = cfg.outputName;
    if (output.empty()) {
      fs::path p = cfg.inputPath;
      output = (p.parent_path() / p.stem()).string();
    }

    if (!cfg.quiet)
      std::cout << "Building: " << cfg.inputPath << " -> " << output
                << ".exe\n";

    // Preprocess
    std::vector<std::string> included;
    Preprocessor preproc;
    SourceMap    srcMap;
    srcMap.setMainFile(cfg.inputPath);
    std::string src = preproc.process(cfg.inputPath, included, srcMap);
    if (preproc.hasErrors()) return 1;
    if (src.empty()) {
      std::cerr << cfg.inputPath << ":0:0: error: could not read file\n";
      return 1;
    }

    // Lex
    Lexer lexer(src, srcMap);
    auto  tokens = lexer.tokenize();
    if (lexer.hasErrors()) return 1;

    // Parse — the map turns stream lines back into file:line for diagnostics
    Parser parser;
    auto   ast = parser.parse(tokens, srcMap);
    if (parser.hasErrors()) return 1;

    // Semantic check: unknown function/command names (WEAK-03 Stufe 1)
    Analyzer analyzer;
    int semanticErrors = checkCalls(ast.get(), srcMap) +
                         checkGosubScope(ast->nodes, srcMap) +
                         analyzer.analyze(ast.get(), srcMap);
    if (semanticErrors > 0) return 1;

    // Emit C++17
    Emitter emitter;
    emitter.emit(ast.get(), output);

    // Transpile-only mode: skip compilation, keep the .cpp
    if (cfg.compileOnly) {
      if (!cfg.quiet)
        std::cout << "Success: " << output << ".cpp written.\n";
      return 0;
    }

    // Compile with MinGW
    if (compile(output + ".cpp", output, cfg.debug)) {
      if (!cfg.quiet)
        std::cout << "Success: " << output << ".exe created.\n";
      if (!cfg.debug) {
        try { fs::remove(output + ".cpp"); } catch (...) {}
      }
      return 0;
    } else {
      std::cerr << cfg.inputPath << ":0:0: error: compilation failed\n";
      return 2;
    }
  }
};

// ---- CLI -------------------------------------------------------------------

static void showHelp() {
  std::cout
      << "BlitzNext Compiler (blitzcc) v" << kVersion << "\n"
      << "Usage: blitzcc [options] <file.bb>\n\n"
      << "  -h          Show this help\n"
      << "  -q          Quiet mode\n"
      << "  +q          Very quiet mode\n"
      << "  -c          Transpile only (no compile step)\n"
      << "  -d          Compile with debug info (-g)\n"
      << "  -release    Release build (no debug info, alias for default)\n"
      << "  -v          Show version\n"
      << "  -o <name>   Output executable name (without .exe)\n"
      << "  -k          List all known built-in command names\n"
      << "  +k          List all known built-in commands with signatures\n"
      << "\nEnvironment:\n"
      << "  BLITZPATH   Installation root fallback for toolchain lookup\n";
}

int main(int argc, char **argv) {
  if (argc > 0)
    g_exeDir_ = fs::absolute(argv[0]).parent_path();

  Config cfg;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if      (arg == "-h")                { showHelp(); return 0; }
    else if (arg == "-k")                { listCommands(false); return 0; }
    else if (arg == "+k")                { listCommands(true);  return 0; }
    else if (arg == "-q")                  cfg.quiet       = true;
    else if (arg == "+q")                  cfg.quiet       = true;
    else if (arg == "-c")                  cfg.compileOnly = true;
    else if (arg == "-d")                  cfg.debug       = true;
    else if (arg == "-release")            cfg.debug       = false;
    else if (arg == "-v") { std::cout << "BlitzNext v" << kVersion << "\n"; return 0; }
    else if (arg == "-o" && i + 1 < argc)  cfg.outputName  = argv[++i];
    else if (arg[0] != '-' && arg[0] != '+') cfg.inputPath = arg;
  }

  if (cfg.inputPath.empty()) {
    showHelp();
    return 1;
  }

  Transpiler t;
  return t.transpile(cfg);
}
