"""BLTZCRFT - Release-Paket zum Herunterladen und direkt Spielen.

Uebersetzt bltzcrft.bb mit bin/blitzcc.exe (Release, -O2) und legt an:

  dist/bltzcrft-v<VERSION>-win64/bltzcrft/
      bltzcrft.exe, SDL3.dll, SDL3_ttf.dll
      daten/logo.png
      README.txt   (aus werkzeug/paket_liesmich.txt)
      LIZENZEN.txt
  dist/bltzcrft-v<VERSION>-win64.zip  (+ .sha256)

Mit --verify wird das ZIP in einen leeren Ordner entpackt und dort der
Rundflug gestartet (oeffnet 20 s ein Fenster); geprueft wird, dass er
rundflug.txt schreibt - also dass exe, DLLs und Daten zusammenpassen.

  py samples/bltzcrft/werkzeug/paket.py [--version 0.1.0] [--verify]
"""

import argparse, hashlib, shutil, subprocess, sys, tempfile, zipfile
from pathlib import Path

HIER = Path(__file__).resolve().parent
SPIEL = HIER.parent
ROOT = SPIEL.parent.parent


def pruefen(bedingung, text):
    print(("  ok    " if bedingung else "  FEHLT ") + text)
    if not bedingung:
        sys.exit(1)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--version", default="0.1.0")
    ap.add_argument("--verify", action="store_true")
    a = ap.parse_args()

    name = f"bltzcrft-v{a.version}-win64"
    dist = ROOT / "dist"
    aus = dist / name
    ziel = aus / "bltzcrft"
    if aus.exists():
        shutil.rmtree(aus)
    (ziel / "daten").mkdir(parents=True)

    print("Uebersetzen ...")
    with tempfile.TemporaryDirectory() as tmp:
        exe = Path(tmp) / "bltzcrft"
        r = subprocess.run([str(ROOT / "bin" / "blitzcc.exe"), "-q", "-o", str(exe), "bltzcrft.bb"],
                           cwd=SPIEL, capture_output=True, text=True)
        if r.returncode != 0:
            print(r.stdout, r.stderr)
            sys.exit("Uebersetzen fehlgeschlagen")
        shutil.copy2(Path(tmp) / "bltzcrft.exe", ziel)
        for dll in ("SDL3.dll", "SDL3_ttf.dll"):
            quelle = Path(tmp) / dll
            if not quelle.exists():
                quelle = ROOT / "bin" / dll
            shutil.copy2(quelle, ziel)

    shutil.copy2(SPIEL / "daten" / "logo.png", ziel / "daten")
    shutil.copy2(SPIEL / "LIZENZEN.txt", ziel)
    text = (HIER / "paket_liesmich.txt").read_text(encoding="utf-8").replace("{VERSION}", a.version)
    # Notepad und Co.: Windows-Zeilenenden
    (ziel / "README.txt").write_bytes(text.replace("\r\n", "\n").replace("\n", "\r\n").encode("utf-8"))

    zipname = dist / f"{name}.zip"
    with zipfile.ZipFile(zipname, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for f in sorted(ziel.rglob("*")):
            if f.is_file():
                z.write(f, f.relative_to(aus).as_posix())
    summe = hashlib.sha256(zipname.read_bytes()).hexdigest()
    (dist / f"{name}.zip.sha256").write_text(f"{summe}  {zipname.name}\n", encoding="ascii")

    print(f"\n{zipname}  ({zipname.stat().st_size / 1e6:.1f} MB)\nSHA-256 {summe}")
    with zipfile.ZipFile(zipname) as z:
        for i in z.infolist():
            print(f"  {i.file_size:>10}  {i.filename}")

    if a.verify:
        print("\nPruefen (entpacken, Rundflug 20 s) ...")
        with tempfile.TemporaryDirectory() as tmp:
            with zipfile.ZipFile(zipname) as z:
                z.extractall(tmp)
            spiel = Path(tmp) / "bltzcrft"
            for f in ("bltzcrft.exe", "SDL3.dll", "SDL3_ttf.dll", "daten/logo.png", "README.txt", "LIZENZEN.txt"):
                pruefen((spiel / f).exists(), f)
            r = subprocess.run([str(spiel / "bltzcrft.exe"), "2026", "-rundflug"], cwd=spiel,
                               capture_output=True, text=True, timeout=180)
            erg = spiel / "rundflug.txt"
            pruefen(r.returncode == 0, f"Spiel endet sauber (Code {r.returncode})")
            pruefen(erg.exists(), "rundflug.txt geschrieben")
            zeile = erg.read_text(encoding="latin-1").strip()
            pruefen(zeile.startswith("Rundflug:"), zeile)


if __name__ == "__main__":
    main()
