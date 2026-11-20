BLTZNXT @VERSION@  -  "The Linux Update"  (Linux x64)
=====================================================

BlitzNext is the successor to Blitz3D: a compiler that turns Blitz3D (.bb) programs
into native programs, and an IDE to write them in. This is the Linux package.

What you need
-------------
- Linux x64 with a desktop (X11 or Wayland) and glibc 2.39 or newer:
  Ubuntu 24.04, Debian 13, Fedora 40 or newer, current Arch, ...
  (Older systems: build BLTZNXT yourself, see "Building" in the repository README.)
- A C++ compiler. blitzcc translates your program to C++ and builds it with g++:
    Ubuntu/Debian:  sudo apt install build-essential
    Fedora:         sudo dnf install gcc-c++
    Arch:           sudo pacman -S base-devel
- Recommended: the Liberation fonts, so that Arial, Times New Roman and Courier New
  have the same measurements as on Windows (fonts-liberation / liberation-fonts /
  ttf-liberation).

Everything else - the compiler, SDL3 and the IDE - is inside.

Getting started
---------------
1. Unpack the archive anywhere:   tar xzf bltznxt-v@VERSION@-linux-x64.tar.gz
2. Start the IDE:                 bltznxt-v@VERSION@-linux-x64/bltznxt-ide
3. File > Open Folder... and choose samples/friendlyfire, open friendlyfire.bb, press F5.
   The first build takes some seconds; the Output tab shows what the compiler does.

Menu entry and desktop icon
---------------------------
    bltznxt-v@VERSION@-linux-x64/install-desktop.sh

adds "BLTZNXT IDE" to the application menu (category Development) and puts an icon on
the desktop; from then on the IDE starts with a click. "--no-desktop" leaves the desktop
alone, "--uninstall" removes everything again. The entry points to the unpacked folder:
after moving it, run the script again.

Windows 11 with WSL: run the same script inside your distribution. It asks for the sudo
password, because the Windows Start menu only lists Linux programs installed for all
users. A few seconds later "BLTZNXT IDE (Ubuntu-24.04)" (or the name of your
distribution) is in the Windows Start menu - pin it to the taskbar or Start like any
Windows program.

The IDE
-------
It follows the original Blitz3D IDE (same menus and keys) and adds a file sidebar:

  F5  Run program            F6  Run program again      F7  Check for errors
  Shift+F5  Stop program     Ctrl+B  Show/hide the sidebar
  File > Open Folder...      Program > Create Executable...

Programs run in the folder of their source file. A text program that reads input with
Input$ does not get a terminal when started from the IDE on Linux - run it from a
terminal instead (see below).

The compiler on its own
-----------------------
    bin/blitzcc hello.bb

writes the program "hello" next to the source, together with libSDL3.so.0 (and
libSDL3_ttf.so.0), which it finds there - the folder can be copied to another
computer as a whole. "bin/blitzcc -h" lists the options. As in Blitz3D,
"Graphics w,h" without a mode opens full screen; built with "bin/blitzcc -d" it opens
a window. Set BLTZNXT_WINDOWED=1 to keep every program in a window.

Old programs written for Windows work unchanged: a path like "Media\Pic.PNG" finds
media/pic.png, and strings stay Windows-1252 as in Blitz3D.

Differences to Windows
----------------------
No gamma, no MIDI music and no movies (OpenMovie) on Linux; FontWidth is measured
differently. KNOWN_ISSUES.md in the repository lists everything that does not yet
behave like Blitz3D.

What is in the package
----------------------
  bltznxt-ide       starts the IDE
  install-desktop.sh  menu entry and desktop icon for the IDE
  bin/             blitzcc, the compiler
  ide/              the IDE (an Electron application)
  src/, libs/       the runtime headers and the SDL3 libraries the programs are built with
  samples/          Friendly Fire, a small Quake III style arena shooter written in Blitz3D
  examples/         more small programs
  LICENSES/         license texts of the bundled software

Project page, source and issues: https://github.com/Farbfinsternis/bltznxt

Licenses
--------
BlitzNext itself: see the repository. Bundled: SDL3 and SDL3_ttf (zlib), stb, dr_mp3
(public domain / MIT), Electron and Chromium (MIT and others, see ide/LICENSE and
ide/LICENSES.chromium.html), Monaco Editor (MIT).
