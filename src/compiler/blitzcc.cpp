#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#include "emitter.h"
#include "lexer.h"
#include "parser.h"
#include "preprocessor.h"
#include "token.h"

namespace fs = std::filesystem;

// ---- Config ----------------------------------------------------------------

struct Config {
  bool        compileOnly = false;
  bool        debug       = false;
  bool        quiet       = false;
  std::string outputName;
  std::string inputPath;
};

// ---- Path helper -----------------------------------------------------------
// Search order: CWD-relative → "../"-relative → $BLITZPATH-relative.
static fs::path resolvePath(const std::string &rel) {
  fs::path p = fs::absolute(rel);
  if (fs::exists(p)) return p;
  fs::path p2 = fs::absolute("../" + rel);
  if (fs::exists(p2)) return p2;
  const char *bp = std::getenv("BLITZPATH");
  if (bp) {
    fs::path p3 = fs::path(bp) / rel;
    if (fs::exists(p3)) return p3;
  }
  return p; // not found — caller checks existence
}

// ---- Known commands --------------------------------------------------------
// Single source of truth for -k / +k output.
// Each entry: { "Name", "param1, param2, ..." }  (empty string = no params)

struct CmdInfo { const char *name; const char *sig; };

static const CmdInfo kCommands[] = {
  // Output / Input
  { "Print",    "value"              },
  { "WaitKey",  ""                   },
  { "Input",    "prompt$=\"\""       },
  // String — conversion
  { "Str",      "value"              },
  { "Int",      "s$"                 },
  { "Float",    "s$"                 },
  { "Len",      "s$"                 },
  // String — extraction (M19)
  { "Left",     "s$, n%"            },
  { "Right",    "s$, n%"            },
  { "Mid",      "s$, pos%"          },
  { "Instr",    "s$, sub$"          },
  { "Replace",  "s$, from$, to$"    },
  // String — transformation (M20)
  { "Upper",    "s$"                 },
  { "Lower",    "s$"                 },
  { "Trim",     "s$"                 },
  { "LSet",     "s$, n%"            },
  { "RSet",     "s$, n%"            },
  { "Chr",      "n%"                 },
  { "Asc",      "s$"                 },
  { "Hex",      "n%"                 },
  { "Bin",      "n%"                 },
  { "String",   "s$, n%"            },
  // Math — trig (M17)
  { "Sin",      "deg#"               },
  { "Cos",      "deg#"               },
  { "Tan",      "deg#"               },
  { "ASin",     "x#"                 },
  { "ACos",     "x#"                 },
  { "ATan",     "x#"                 },
  { "ATan2",    "y#, x#"            },
  // Math — general (M17)
  { "Sqr",      "x#"                 },
  { "Abs",      "x#"                 },
  { "Log",      "x#"                 },
  { "Log10",    "x#"                 },
  { "Exp",      "x#"                 },
  { "Floor",    "x#"                 },
  { "Ceil",     "x#"                 },
  { "Sgn",      "x#"                 },
  { "Pi",       ""                   },
  // Math — random (M18)
  { "Rnd",      ""                   },
  { "Rand",     "max%"               },
  { "SeedRnd",  "seed%"              },
  { "RndSeed",  ""                   },
  // Time / System
  { "Delay",    "ms%"                },
  // Sound (M34)
  { "LoadSound",   "file$"          },
  { "FreeSound",   "handle%"        },
  { "PlaySound",   "handle%"        },
  { "LoopSound",   "handle%"        },
  { "StopChannel", "channel%"       },
  // 2D Graphics — colour & pixel primitives (M40)
  { "Color",       "r%, g%, b%"   },
  { "ClsColor",    "r%, g%, b%"   },
  { "ColorRed",    ""             },
  { "ColorGreen",  ""             },
  { "ColorBlue",   ""             },
  { "GetColor",    "x%, y%"       },
  { "Plot",        "x%, y%"       },
  // 2D Graphics — buffer & flip (M39)
  { "BackBuffer",  ""                                              },
  { "FrontBuffer", ""                                              },
  { "SetBuffer",   "buffer%"                                       },
  { "Cls",         ""                                              },
  { "Flip",        "vblank%=1"                                     },
  { "CopyRect",    "sx%, sy%, sw%, sh%, dx%, dy% [,src%] [,dst%]"  },
  // 2D Graphics — init (M38)
  { "Graphics",         "width%, height%, depth%=32, mode%=0" },
  { "EndGraphics",      ""                                     },
  { "GraphicsWidth",    ""                                     },
  { "GraphicsHeight",   ""                                     },
  { "GraphicsDepth",    ""                                     },
  { "GraphicsRate",     ""                                     },
  { "TotalVidMem",      ""                                     },
  { "AvailVidMem",      ""                                     },
  { "GraphicsMode",     "w%, h%, depth%, rate%"                },
  // 3D Sound (M37)
  { "Load3DSound",        "file$"                        },
  { "SoundRange",         "snd%, inner#, outer#"         },
  { "Channel3DPosition",  "ch%, x#, y#, z#"             },
  { "Channel3DVelocity",  "ch%, vx#, vy#, vz#"          },
  { "ListenerPosition",   "x#, y#, z#"                  },
  { "ListenerOrientation","fx#, fy#, fz#, ux#, uy#, uz#"},
  { "ListenerVelocity",   "vx#, vy#, vz#"               },
  { "WaitSound",          "ch%"                          },
  // Music & CD (M36)
  { "PlayMusic",    "file$"          },
  { "StopMusic",    ""               },
  { "MusicPlaying", ""               },
  { "PlayCDTrack",  "track%"         },
  // Channel control (M35)
  { "PauseChannel",   "channel%"              },
  { "ResumeChannel",  "channel%"              },
  { "ChannelPlaying", "channel%"              },
  { "ChannelVolume",  "channel%, vol#"        },
  { "ChannelPan",     "channel%, pan#"        },
  { "ChannelPitch",   "channel%, hz#"         },
  { "SoundVolume",    "handle%, vol#"         },
  { "SoundPan",       "handle%, pan#"         },
  { "SoundPitch",     "handle%, hz#"          },
  // Joystick (M33)
  { "JoyType",  "port%"             },
  { "JoyX",     "port%"             },
  { "JoyY",     "port%"             },
  { "JoyZ",     "port%"             },
  { "JoyU",     "port%"             },
  { "JoyV",     "port%"             },
  { "JoyHat",   "port%"             },
  { "JoyDown",  "port%, button%"    },
  { "JoyHit",   "port%, button%"    },
  { "WaitJoy",  "port%"             },
  { "GetJoy",   "port%"             },
  { "FlushJoy", "port%"             },
};

// -k  → one name per line
// +k  → name(sig) per line
static void listCommands(bool withSigs) {
  for (const auto &c : kCommands) {
    if (withSigs)
      std::cout << c.name << "(" << c.sig << ")\n";
    else
      std::cout << c.name << "\n";
  }
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

    fs::path includeDir = resolvePath("src/compiler");
    fs::path sdlBase    = resolvePath("libs/sd3/x86_64-w64-mingw32");
    fs::path sdlInc     = sdlBase / "include";
    fs::path libDir     = sdlBase / "lib";
    fs::path sdlImport  = libDir / "libSDL3.dll.a";

    std::string gpp      = gppPath.make_preferred().string();
    std::string finalCpp = fs::absolute(cppPath).make_preferred().string();
    std::string finalOut =
        fs::absolute(outputPath + ".exe").make_preferred().string();
    std::string incDir    = includeDir.make_preferred().string();
    std::string sdlIncDir = sdlInc.make_preferred().string();

    std::string cmd = "\"" + gpp + "\" -std=c++17 -static"
                      " -I\"" + incDir + "\""
                      " -I\"" + sdlIncDir + "\""
                      " \"" + finalCpp + "\""
                      " -o \"" + finalOut + "\"";

    // Only link SDL3 when the import library is present
    if (fs::exists(sdlImport))
      cmd += " \"" + sdlImport.make_preferred().string() + "\"";

    if (debug) cmd += " -g";

    // Windows cmd.exe /c requires an extra pair of outer quotes when the
    // first token inside is itself quoted.
    int res = std::system(("\"" + cmd + "\"").c_str());
    if (res != 0) return false;

    // Copy SDL3.dll next to the executable (non-fatal if absent)
    fs::path sdlDll = resolvePath("libs/sd3/x86_64-w64-mingw32/bin/SDL3.dll");
    if (fs::exists(sdlDll)) {
      try {
        fs::copy_file(sdlDll,
                      fs::path(outputPath).parent_path() / "SDL3.dll",
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
    std::string src = preproc.process(cfg.inputPath, included);
    if (src.empty()) {
      std::cerr << cfg.inputPath << ":0:0: error: could not read file\n";
      return 1;
    }

    // Lex
    Lexer lexer(src);
    auto  tokens = lexer.tokenize();

    // Parse — pass filename for IDE-parseable error messages
    Parser parser;
    auto   ast = parser.parse(tokens, cfg.inputPath);
    if (parser.hasErrors()) return 1;

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
      << "BlitzNext Compiler (blitzcc) v0.1.4\n"
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
    else if (arg == "-v") { std::cout << "BlitzNext v0.2.7\n"; return 0; }
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
