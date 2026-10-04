#!/usr/bin/env python
"""Baut das Windows-Paket von BLTZNXT: Compiler, IDE, Toolchain, Bibliotheken.

    py scripts/package_release.py            baut dist/bltznxt-v<Version>-win64/ und die ZIP
    py scripts/package_release.py --no-zip   nur den Ordner
    py scripts/package_release.py --verify   danach das Paket pruefen (uebersetzt und startet Programme)
    py scripts/package_release.py --no-build ide/dist nicht neu bauen (sonst: npm run build)

Voraussetzungen: bin/blitzcc.exe ist gebaut (build_windows.bat oder die Zeile im
README), ide/ hat node_modules (npm install), die MinGW-Toolchain liegt unter
tools/mingw64.

Die Version steht in ide/package.json; blitzcc muss dieselbe melden (-v).

Aufbau des Pakets:

    bltznxt-v0.6.5-win64/
      BLTZNXT IDE.bat         startet die IDE
      README.txt  LICENSES/
      bin/                    blitzcc.exe und seine DLLs, SDL3
      ide/                    die IDE (Electron): BLTZNXT IDE.exe, resources/app
      src/compiler/           die Laufzeit-Header, die das erzeugte C++ einbindet
      src/thirdparty/         stb, dr_mp3, ...
      libs/                   SDL3, SDL3_ttf
      tools/mingw64/          g++ - auf das Noetige verkleinert
      samples/friendlyfire/     das Beispielspiel (Quelltexte und Daten)
      examples/               weitere Beispiele

blitzcc findet Toolchain, Header und Bibliotheken relativ zu seinem eigenen Ort
(bin/..), die IDE findet blitzcc unter ../bin neben sich - deshalb laeuft das
Paket aus jedem Ordner.
"""

import argparse
import fnmatch
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TOOLS = ROOT / "tools" / "mingw64"
GCC_VER = "14.2.0"
TRIPLE = "x86_64-w64-mingw32"


def log(msg):
    print(msg, flush=True)


def version():
    return json.loads((ROOT / "ide" / "package.json").read_text(encoding="utf-8"))["version"]


def copy_tree(src, dst, ignore=None):
    if not src.exists():
        raise SystemExit(f"fehlt: {src}")
    shutil.copytree(src, dst, ignore=ignore, dirs_exist_ok=True)


def copy_file(src, dst):
    if not src.exists():
        raise SystemExit(f"fehlt: {src}")
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, dst)


def ignore_patterns(*patterns):
    def ignore(_dir, names):
        return [n for n in names if any(fnmatch.fnmatch(n.lower(), p.lower()) for p in patterns)]
    return ignore


# ---------------------------------------------------------------------------
# Bausteine
# ---------------------------------------------------------------------------

def add_compiler(out):
    bin_out = out / "bin"
    for name in ["blitzcc.exe", "libgcc_s_seh-1.dll", "libstdc++-6.dll", "libwinpthread-1.dll",
                 "SDL3.dll", "SDL3_ttf.dll"]:
        copy_file(ROOT / "bin" / name, bin_out / name)
    # Laufzeit-Header (das erzeugte C++ bindet sie ein) und Drittbibliotheken
    for header in (ROOT / "src" / "compiler").glob("*.h"):
        copy_file(header, out / "src" / "compiler" / header.name)
    copy_tree(ROOT / "src" / "thirdparty", out / "src" / "thirdparty")


def add_libs(out):
    ignore = ignore_patterns("*.md", "cmake", "pkgconfig", "*.pc")
    for lib in ("sd3", "sdl3_ttf"):
        base = ROOT / "libs" / lib
        target = out / "libs" / lib
        copy_file(base / "LICENSE.txt", target / "LICENSE.txt")
        copy_tree(base / TRIPLE, target / TRIPLE, ignore=ignore)


def add_toolchain(out):
    """g++ ohne alles, was BLTZNXT nicht braucht (clang, gdb, python, cmake, ...)."""
    t = out / "tools" / "mingw64"
    # Treiber, Assembler, Linker
    for name in ["g++.exe", "gcc.exe", "as.exe", "ld.exe"]:
        copy_file(TOOLS / "bin" / name, t / "bin" / name)
    # Bibliotheken der Programme im Treiberordner (winpthread, iconv, ...)
    for dll in (TOOLS / "bin").glob("*.dll"):
        if dll.name.lower() in ("libwinpthread-1.dll", "libgcc_s_seh-1.dll", "libstdc++-6.dll",
                                "libiconv-2.dll", "libintl-8.dll", "libzstd.dll", "zlib1.dll"):
            copy_file(dll, t / "bin" / dll.name)
    # Uebersetzer, Linkerhelfer und deren DLLs
    lx = f"libexec/gcc/{TRIPLE}/{GCC_VER}"
    for name in ["cc1plus.exe", "collect2.exe", "lto-wrapper.exe", "liblto_plugin.dll"]:
        copy_file(TOOLS / lx / name, t / lx / name)
    for dll in (TOOLS / lx).glob("*.dll"):
        copy_file(dll, t / lx / dll.name)
    # die "mingw"-Unterordner: Assembler/Linker, Header, Import-Bibliotheken
    for name in ["as.exe", "ld.exe"]:
        copy_file(TOOLS / TRIPLE / "bin" / name, t / TRIPLE / "bin" / name)
    for dll in (TOOLS / TRIPLE / "bin").glob("*.dll"):
        copy_file(dll, t / TRIPLE / "bin" / dll.name)
    copy_tree(TOOLS / TRIPLE / "include", t / TRIPLE / "include")
    copy_tree(TOOLS / TRIPLE / "lib", t / TRIPLE / "lib")
    # libgcc.a und Verwandte liegen in <triple>/<gcc-version> ...
    copy_tree(TOOLS / TRIPLE / GCC_VER, t / TRIPLE / GCC_VER)
    # ... die C++-Laufzeit im obersten lib/ (dort liegt auch viel LLVM, das nicht mit soll)
    for pattern in ("libstdc++*.a", "libsupc++.a", "libgcc_s.a", "libatomic*", "libgomp.*",
                    "libquadmath*", "libssp*"):
        for lib in (TOOLS / "lib").glob(pattern):
            copy_file(lib, t / "lib" / lib.name)
    # GCC-eigene Dateien (crt, libgcc, Intrinsics) und die C++-Standardbibliothek
    copy_tree(TOOLS / "lib" / "gcc" / TRIPLE / GCC_VER, t / "lib" / "gcc" / TRIPLE / GCC_VER)
    copy_tree(TOOLS / "include" / "c++", t / "include" / "c++")
    # Lizenzen der Toolchain
    for name in ["version_info.txt"]:
        if (TOOLS / name).exists():
            copy_file(TOOLS / name, t / name)


def add_ide(out):
    """Electron mit der IDE unter resources/app; nur Englisch und Deutsch."""
    electron = ROOT / "ide" / "node_modules" / "electron" / "dist"
    ide = out / "ide"
    copy_tree(electron, ide, ignore=ignore_patterns("*.pdb"))
    # Sprachdateien: die IDE ist englisch, Deutsch kommt als Zweitsprache
    locales = ide / "locales"
    for pak in locales.glob("*.pak"):
        if pak.stem not in ("en-US", "de"):
            pak.unlink()
    default_app = ide / "resources" / "default_app.asar"
    if default_app.exists():
        default_app.unlink()
    (ide / "electron.exe").rename(ide / "BLTZNXT IDE.exe")

    app = ide / "resources" / "app"
    if not (ROOT / "ide" / "dist" / "index.html").exists():
        raise SystemExit("ide/dist fehlt - erst 'npm run build' in ide/")
    copy_tree(ROOT / "ide" / "dist", app / "dist")
    copy_tree(ROOT / "ide" / "electron", app / "electron")
    copy_file(ROOT / "ide" / "electron-main.js", app / "electron-main.js")
    pkg = json.loads((ROOT / "ide" / "package.json").read_text(encoding="utf-8"))
    (app / "package.json").write_text(json.dumps({
        "name": pkg["name"],
        "version": pkg["version"],
        "main": pkg["main"],
        "description": "BLTZNXT IDE",
    }, indent=2) + "\n", encoding="utf-8")
    # Monaco steckt im Bundle; seine Lizenz gehoert dazu
    monaco = ROOT / "ide" / "node_modules" / "monaco-editor" / "LICENSE"
    if monaco.exists():
        copy_file(monaco, out / "LICENSES" / "monaco-editor-LICENSE.txt")


def add_samples(out):
    lt = ROOT / "samples" / "friendlyfire"
    copy_tree(lt, out / "samples" / "friendlyfire",
              ignore=ignore_patterns("*.exe", "*.dll", "kenney", "roh", "astra", "__pycache__", "*.pyc"))
    if (ROOT / "examples").exists():
        copy_tree(ROOT / "examples", out / "examples",
                  ignore=ignore_patterns("*.exe", "*.dll", "*.cpp", "__pycache__"))


def add_docs(out, ver):
    (out / "LICENSES").mkdir(parents=True, exist_ok=True)
    copy_file(ROOT / "libs" / "sd3" / "LICENSE.txt", out / "LICENSES" / "SDL3-LICENSE.txt")
    copy_file(ROOT / "libs" / "sdl3_ttf" / "LICENSE.txt", out / "LICENSES" / "SDL3_ttf-LICENSE.txt")
    for src, name in [(ROOT / "ide" / "node_modules" / "electron" / "dist" / "LICENSE", "Electron-LICENSE.txt")]:
        if src.exists():
            copy_file(src, out / "LICENSES" / name)
    gcc_docs = TOOLS / "share" / "doc" / "gcc"
    for lic in ("COPYING3", "COPYING.RUNTIME"):
        for hit in list(TOOLS.rglob(lic))[:1]:
            copy_file(hit, out / "LICENSES" / f"GCC-{lic}.txt")
    template = (ROOT / "scripts" / "package_readme.txt").read_text(encoding="utf-8")
    (out / "README.txt").write_text(template.replace("@VERSION@", ver), encoding="utf-8", newline="\r\n")
    (out / "BLTZNXT IDE.bat").write_text(
        '@echo off\r\nstart "" "%~dp0ide\\BLTZNXT IDE.exe" %*\r\n', encoding="ascii")


# ---------------------------------------------------------------------------
# Pruefen
# ---------------------------------------------------------------------------

def run(cmd, cwd, env, timeout=300):
    return subprocess.run(cmd, cwd=cwd, env=env, capture_output=True, text=True, timeout=timeout)


def verify(out):
    """Das Paket allein, ohne das Repository: uebersetzen und starten."""
    blitzcc = out / "bin" / "blitzcc.exe"
    # Sauberes Umfeld: nur Windows im PATH, kein BLITZPATH, ein fremder Arbeitsordner
    env = {k: v for k, v in os.environ.items() if k.upper() not in ("BLITZPATH", "BLITZIDE")}
    env["PATH"] = os.path.join(os.environ.get("SystemRoot", r"C:\Windows"), "System32")
    work = Path(tempfile.mkdtemp(prefix="bltznxt-verify-"))
    failures = 0

    def check(label, ok, detail=""):
        nonlocal failures
        log(f"{'PASS' if ok else 'FAIL'}  {label}{'  -> ' + detail if detail else ''}")
        if not ok:
            failures += 1

    r = run([str(blitzcc), "-v"], work, env)
    check("blitzcc -v meldet die Version", version() in r.stdout, r.stdout.strip())

    (work / "text.bb").write_text('Print "Hallo aus dem Paket"\r\nEnd\r\n', encoding="ascii")
    r = run([str(blitzcc), "-q", "text.bb"], work, env)
    exe = work / "text.exe"
    check("Textprogramm uebersetzt", r.returncode == 0 and exe.exists(), (r.stdout + r.stderr).strip()[:400])
    if exe.exists():
        r = run([str(exe)], work, env, timeout=30)
        check("Textprogramm laeuft", "Hallo aus dem Paket" in r.stdout, r.stdout.strip())

    (work / "grafik.bb").write_text(
        'Graphics3D 320, 240, 32, 2\r\nc = CreateCamera()\r\ncube = CreateCube()\r\n'
        'MoveEntity cube, 0, 0, 5\r\nRenderWorld\r\nFlip\r\nEnd\r\n', encoding="ascii")
    r = run([str(blitzcc), "-q", "grafik.bb"], work, env)
    check("3D-Programm uebersetzt und bindet SDL3", r.returncode == 0 and (work / "grafik.exe").exists(),
          (r.stdout + r.stderr).strip()[:400])
    check("SDL3.dll liegt neben dem Programm", (work / "SDL3.dll").exists())

    # Das Beispielspiel: Quelle -> Code (ohne g++), dann mit g++ im IDE-Protokoll
    lt = out / "samples" / "friendlyfire" / "friendlyfire.bb"
    if lt.exists():
        env_ide = dict(env, blitzide="1")
        r = run([str(blitzcc), "-q", "-c", str(lt)], lt.parent, env_ide)
        check("Friendly Fire: Pruefung im IDE-Protokoll (Include-Kette)", r.returncode == 0 and "Generating C++..." in r.stdout,
              r.stdout.replace("\r\n", " | ").strip()[:300])
        r = run([str(blitzcc), "-q", "-o", str(work / "friendlyfire.exe"), str(lt)], lt.parent, env_ide, timeout=600)
        check("Friendly Fire wird zu einem Programm", r.returncode == 0 and (work / "friendlyfire.exe").exists(),
              r.stdout.replace("\r\n", " | ").strip()[-300:])

    shutil.rmtree(work, ignore_errors=True)
    return failures


# ---------------------------------------------------------------------------

def zip_dir(folder, target):
    if target.exists():
        target.unlink()
    with zipfile.ZipFile(target, "w", zipfile.ZIP_DEFLATED, compresslevel=6) as z:
        for path in sorted(folder.rglob("*")):
            if path.is_file():
                z.write(path, Path(folder.name) / path.relative_to(folder))


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest()


def dir_size(folder):
    return sum(p.stat().st_size for p in folder.rglob("*") if p.is_file())


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--no-zip", action="store_true")
    ap.add_argument("--no-build", action="store_true", help="ide/dist nicht neu bauen")
    ap.add_argument("--verify", action="store_true")
    args = ap.parse_args()

    ver = version()
    reported = subprocess.run([str(ROOT / "bin" / "blitzcc.exe"), "-v"], capture_output=True, text=True).stdout
    if ver not in reported:
        raise SystemExit(f"bin/blitzcc.exe meldet '{reported.strip()}', erwartet {ver} - neu bauen")

    out = ROOT / "dist" / f"bltznxt-v{ver}-win64"
    if out.exists():
        shutil.rmtree(out)
    out.mkdir(parents=True)

    # Die Version steckt im gebauten Renderer ("About"): immer frisch bauen
    if not args.no_build:
        log("... npm run build (ide)")
        r = subprocess.run("npm run build", cwd=ROOT / "ide", shell=True, capture_output=True, text=True)
        if r.returncode != 0:
            raise SystemExit(f"npm run build ist fehlgeschlagen:\n{r.stdout}\n{r.stderr}")

    for step in (add_compiler, add_libs, add_toolchain, add_ide, add_samples):
        log(f"... {step.__name__}")
        step(out)
    add_docs(out, ver)
    log(f"Ordner: {out}  ({dir_size(out) / 1e6:.0f} MB)")

    failures = verify(out) if args.verify else 0

    if not args.no_zip:
        target = ROOT / "dist" / f"bltznxt-v{ver}-win64.zip"
        log(f"... ZIP {target.name}")
        zip_dir(out, target)
        digest = sha256(target)
        (ROOT / "dist" / f"{target.name}.sha256").write_text(f"{digest} *{target.name}\n", encoding="ascii")
        log(f"ZIP: {target}  ({target.stat().st_size / 1e6:.0f} MB)\nSHA-256: {digest}")

    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
