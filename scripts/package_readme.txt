BLTZNXT @VERSION@  -  "Birth of IDE"
====================================

BlitzNext is the successor to Blitz3D: a compiler that turns Blitz3D (.bb) programs
into native Windows executables, and now an IDE to write them in.

This package is self-contained. You need Windows 10 or 11 (64-bit) and nothing else:
the compiler, the C++ toolchain it uses, SDL3 and the IDE are all inside.

Getting started
---------------
1. Unpack the ZIP anywhere (a path without special characters is best).
2. Double-click "BLTZNXT IDE.bat" (or ide\BLTZNXT IDE.exe).
3. File > Open Folder... and choose samplesriendlyfire, open friendlyfire.bb, press F5.
   The first build takes some seconds; the Output tab shows what the compiler does.

Windows may show a "SmartScreen" warning the first time: the programs are not
code-signed. Choose "More info" > "Run anyway".

The IDE
-------
It follows the original Blitz3D IDE (same menus and keys) and adds a file sidebar:

  F5  Run program            F6  Run program again      F7  Check for errors
  Shift+F5  Stop program     Ctrl+B  Show/hide the sidebar
  File > Open Folder...      Program > Create Executable...

A program has no project file: it is bound to the folder of its source. New, untitled
tabs can be run without saving. Programs run in the folder of their source file.
Not there yet: a help browser (F1) and a debugger. See KNOWN_ISSUES.md in the
repository for everything that does not yet behave like Blitz3D.

The compiler on its own
-----------------------
    bin\blitzcc.exe hello.bb

writes hello.exe next to the source. "bin\blitzcc.exe -h" lists the options.
The compiler finds its toolchain, headers and libraries relative to bin\, so the folder
can be moved or copied as a whole. The Blitz3D IDE can also use it as its compiler
(see the repository README).

What is in the package
----------------------
  bin\              blitzcc.exe and the DLLs it needs
  ide\              the IDE (an Electron application)
  tools\mingw64\    the C++ compiler blitzcc uses (a reduced MinGW-w64 / GCC build)
  src\, libs\       the runtime headers and the SDL3 libraries the programs are built with
  samples\          Friendly Fire, a small Quake III style arena shooter written in Blitz3D
  examples\         more small programs
  LICENSES\         license texts of the bundled software

Project page, source and issues: https://github.com/Farbfinsternis/bltznxt

Licenses
--------
BlitzNext itself: see the repository. Bundled: SDL3 and SDL3_ttf (zlib), stb, dr_mp3
(public domain / MIT), Electron and Chromium (MIT and others, see ide\LICENSE and
ide\LICENSES.chromium.html), Monaco Editor (MIT), and MinGW-w64 / GCC (GPL with the
GCC Runtime Library Exception; tools\mingw64\version_info.txt names the build, whose
source is available from https://winlibs.com). The GCC runtime exception lets you
distribute programs built with it under your own terms.
