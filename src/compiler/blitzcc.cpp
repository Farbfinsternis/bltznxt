#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#include "emitter.h"
#include "lexer.h"
#include "parser.h"
#include "preprocessor.h"
#include "token.h"

// Include last: windows.h macros (BOOL, ERROR, min/max, ...) must not
// pollute the project headers above.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

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
  { "Min",      "a, b"               },
  { "Max",      "a, b"               },
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
  { "Origin",      "x%, y%"                                        },
  { "Viewport",    "x%, y%, w%, h%"                                },
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
  // Keyboard (M31)
  { "KeyDown",      "key%"           },
  { "KeyHit",       "key%"           },
  { "GetKey",       ""               },
  { "FlushKeys",    ""               },
  // Mouse (M32)
  { "MouseX",       ""               },
  { "MouseY",       ""               },
  { "MouseZ",       ""               },
  { "MouseXSpeed",  ""               },
  { "MouseYSpeed",  ""               },
  { "MouseZSpeed",  ""               },
  { "MouseDown",    "button%"        },
  { "MouseHit",     "button%"        },
  { "WaitMouse",    ""               },
  { "GetMouse",     ""               },
  { "FlushMouse",   ""               },
  { "MoveMouse",    "x%, y%"         },
  // Events
  { "PollEvents",   ""               },
  // System (M28-M30)
  { "AppTitle",     "title$"         },
  { "MilliSecs",    ""               },
  { "CurrentDate",  ""               },
  { "CurrentTime",  ""               },
  { "RuntimeError", "msg$"           },
  { "Notify",       "msg$"           },
  { "Confirm",      "msg$"           },
  { "Proceed",      "msg$"           },
  { "CommandLine",  ""               },
  { "ExecFile",     "path$"          },
  { "GetEnv",       "name$"          },
  { "SetEnv",       "name$, val$"    },
  { "SystemProperty","prop$"         },
  { "ShowPointer",  ""               },
  { "HidePointer",  ""               },
  { "CreateTimer",  "hz%"            },
  { "WaitTimer",    "handle%"        },
  { "FreeTimer",    "handle%"        },
  { "CallDLL",      "dll$, func$, bank%, retbank%" },
  // File I/O (M25-M27)
  { "OpenFile",     "path$"          },
  { "ReadFile",     "path$"          },
  { "WriteFile",    "path$"          },
  { "CloseFile",    "handle%"        },
  { "FilePos",      "handle%"        },
  { "SeekFile",     "handle%, pos%"  },
  { "Eof",          "handle%"        },
  { "ReadAvail",    "handle%"        },
  { "WriteByte",    "handle%, val%"  },
  { "WriteShort",   "handle%, val%"  },
  { "WriteInt",     "handle%, val%"  },
  { "WriteFloat",   "handle%, val#"  },
  { "WriteString",  "handle%, s$"    },
  { "WriteLine",    "handle%, s$"    },
  { "ReadByte",     "handle%"        },
  { "ReadShort",    "handle%"        },
  { "ReadInt",      "handle%"        },
  { "ReadFloat",    "handle%"        },
  { "ReadString",   "handle%"        },
  { "ReadLine",     "handle%"        },
  { "ReadDir",      "path$"          },
  { "NextFile",     "handle%"        },
  { "CloseDir",     "handle%"        },
  { "CurrentDir",   ""               },
  { "ChangeDir",    "path$"          },
  { "CreateDir",    "path$"          },
  { "DeleteDir",    "path$"          },
  { "FileType",     "path$"          },
  { "FileSize",     "path$"          },
  { "CopyFile",     "src$, dst$"     },
  { "DeleteFile",   "path$"          },
  // Bank (M24)
  { "CreateBank",   "size%"          },
  { "FreeBank",     "handle%"        },
  { "BankSize",     "handle%"        },
  { "ResizeBank",   "handle%, size%" },
  { "CopyBank",     "src%, srcoff%, dst%, dstoff%, len%" },
  { "PeekByte",     "handle%, offset%"  },
  { "PeekShort",    "handle%, offset%"  },
  { "PeekInt",      "handle%, offset%"  },
  { "PeekFloat",    "handle%, offset%"  },
  { "PokeByte",     "handle%, offset%, val%"  },
  { "PokeShort",    "handle%, offset%, val%"  },
  { "PokeInt",      "handle%, offset%, val%"  },
  { "PokeFloat",    "handle%, offset%, val#"  },
  { "ReadBytes",    "bank%, file%, offset%, count%" },
  { "WriteBytes",   "bank%, file%, offset%, count%" },
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

// ---- Semantic check: unknown calls (WEAK-03, Stufe 1) ----------------------
//
// Walks the entire AST and reports any CallExpr whose name is not in
// kCommands[] and is not a user-defined Function.  Stufe 1 only — no
// "did you mean?" suggestions and no type / arity checking (Stufe 2/3).

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
  }
  // ExitStmt, EndStmt, LabelStmt, GotoStmt, GosubStmt,
  // DataStmt, ReadStmt, RestoreStmt, TypeDecl — nothing to walk
}

static void collectCallsBlock(const std::vector<std::unique_ptr<ASTNode>> &blk,
                              std::vector<const CallExpr *> &out) {
  for (auto &s : blk) collectCallsNode(s.get(), out);
}

// Returns number of errors emitted (0 = clean).
static int checkCalls(const Program *prog, const std::string &filename) {
  // Build known-name set: all built-in commands + user-defined functions
  std::unordered_set<std::string> known;
  for (const auto &c : kCommands)
    known.insert(toUpper(c.name));
  for (const auto &s : prog->nodes)
    if (auto *fd = dynamic_cast<const FunctionDecl *>(s.get()))
      known.insert(toUpper(fd->name));

  std::vector<const CallExpr *> calls;
  collectCallsBlock(prog->nodes, calls);

  int errors = 0;
  for (const auto *ce : calls) {
    if (known.count(toUpper(ce->name)) == 0) {
      std::cerr << filename << ":" << ce->line
                << ": error: unknown function or command '" << ce->name << "'\n";
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
    std::string src = preproc.process(cfg.inputPath, included);
    if (src.empty()) {
      std::cerr << cfg.inputPath << ":0:0: error: could not read file\n";
      return 1;
    }

    // Lex
    Lexer lexer(src, cfg.inputPath);
    auto  tokens = lexer.tokenize();
    if (lexer.hasErrors()) return 1;

    // Parse — pass filename for IDE-parseable error messages
    Parser parser;
    auto   ast = parser.parse(tokens, cfg.inputPath);
    if (parser.hasErrors()) return 1;

    // Semantic check: unknown function/command names (WEAK-03 Stufe 1)
    if (checkCalls(ast.get(), cfg.inputPath) > 0) return 1;

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
      << "BlitzNext Compiler (blitzcc) v0.4.0\n"
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
    else if (arg == "-v") { std::cout << "BlitzNext v0.4.0\n"; return 0; }
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
