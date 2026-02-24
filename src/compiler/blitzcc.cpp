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
  { "Rgb",         "r%, g%, b%"   },
  // 2D Graphics — shapes (M41)
  { "Line",        "x1%, y1%, x2%, y2%"              },
  { "Rect",        "x%, y%, w%, h% [,solid%]"         },
  { "Oval",        "x%, y%, w%, h% [,solid%]"         },
  { "Poly",        "x0%, y0%, x1%, y1%, x2%, y2%"    },
  // 2D Graphics — text (M42)
  { "Write",       "val"                                          },
  { "Locate",      "x%, y%"                                      },
  { "Text",        "x%, y%, s$ [,centerX%] [,centerY%]"          },
  // 2D Graphics — fonts (M43)
  { "LoadFont",    "name$, height% [,bold%] [,italic%] [,underline%]" },
  { "SetFont",     "handle%"                                      },
  { "FreeFont",    "handle%"                                      },
  { "FontWidth",   ""                                             },
  { "FontHeight",  ""                                             },
  { "StringWidth", "s$"                                          },
  { "StringHeight","s$"                                          },
  // 2D Graphics — images (M44)
  { "LoadImage",       "file$"                                                    },
  { "CreateImage",     "w%, h% [,frames%=1]"                                      },
  { "FreeImage",       "handle%"                                                   },
  { "ImageWidth",      "handle% [,frame%=0]"                                      },
  { "ImageHeight",     "handle% [,frame%=0]"                                      },
  { "DrawImage",       "handle%, x%, y% [,frame%=0]"                              },
  { "DrawImageRect",   "handle%, x%, y%, sx%, sy%, sw%, sh% [,frame%=0]"          },
  { "DrawBlock",       "handle%, x%, y% [,frame%=0]"                              },
  { "DrawBlockRect",   "handle%, x%, y%, sx%, sy%, sw%, sh% [,frame%=0]"          },
  // 2D Graphics — image manipulation (M45)
  { "HandleImage",     "handle%, x%, y% [,frame%=0]"                              },
  { "MidHandle",       "handle% [,frame%=0]"                                      },
  { "AutoMidHandle",   "on%"                                                       },
  { "ImageXHandle",    "handle% [,frame%=0]"                                      },
  { "ImageYHandle",    "handle% [,frame%=0]"                                      },
  { "ScaleImage",      "handle%, sx#, sy# [,frame%=0]"                            },
  { "RotateImage",     "handle%, deg# [,frame%=0]"                                },
  { "MaskImage",       "handle%, r%, g%, b% [,frame%=0]"                          },
  { "TileImage",       "handle%, x%, y% [,frame%=0]"                              },
  { "TileBlock",       "handle%, x%, y% [,frame%=0]"                              },
  { "DrawImageEllipse","handle%, x%, y%, rx%, ry% [,frame%=0]"                    },
  { "SaveImage",       "handle%, file$ [,frame%=0]"                               },
  { "ImagesOverlap",   "h1%, x1%, y1%, h2%, x2%, y2%"                             },
  { "ImageRectOverlap","handle%, x%, y%, rx%, ry%, rw%, rh%"                      },
  { "ImagesColl",      "h1%, x1%, y1%, h2%, x2%, y2%"                             },
  { "ImageXColl",      "h1%, x1%, y1%, h2%, x2%, y2%"                             },
  { "ImageYColl",      "h1%, x1%, y1%, h2%, x2%, y2%"                             },
  // 2D Graphics — animated images & new image API (M46b)
  { "LoadAnimImage",   "file$, fw%, fh%, first%, count%"                           },
  { "GrabImage",       "handle%, x%, y% [,frame%=0]"                              },
  { "CopyImage",       "handle%"                                                   },
  { "FlipImage",       "handle% [,frame%=0]"                                      },
  { "MirrorImage",     "handle% [,frame%=0]"                                      },
  { "ImagesCollide",   "h1%, x1%, y1%, f1%, h2%, x2%, y2%, f2%"                  },
  { "ImageRectCollide","handle%, x%, y%, frame%, rx%, ry%, rw%, rh%"              },
  // 2D Graphics — pixel buffer access (M46)
  { "ImageBuffer",      "handle% [,frame%=0]"                                      },
  { "LockBuffer",       "buf%"                                             },
  { "UnlockBuffer",     "buf%"                                             },
  { "ReadPixel",        "x%, y%, buf%"                                     },
  { "WritePixel",       "x%, y%, color%, buf%"                             },
  { "ReadPixelFast",    "x%, y%, buf%"                                     },
  { "WritePixelFast",   "x%, y%, color%, buf%"                             },
  { "CopyPixel",        "sx%, sy%, sbuf%, dx%, dy%, dbuf%"                 },
  { "CopyPixelFast",    "sx%, sy%, sbuf%, dx%, dy%, dbuf%"                 },
  { "LoadBuffer",       "buf%, file$"                                      },
  { "SaveBuffer",       "buf%, file$"                                      },
  { "BufferWidth",      "buf%"                                             },
  { "BufferHeight",     "buf%"                                             },
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
