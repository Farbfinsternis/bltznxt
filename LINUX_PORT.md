# Plan: BLTZNXT unter Linux

Stand 2026-11-19. Anlass: ein Tester betreibt BLTZNXT unter Linux mit Wine; die IDE (Electron 34)
startet dort nicht. Ziel: BLTZNXT nativ unter Linux bauen und benutzen, alte Programme laufen
wie unter Windows (siehe Kompatibilitätsziel). Arbeitsplan, kein Vertrag — wo ein Punkt falsch
ist, wird er geändert und hier nachgezogen.

Einordnung: eigenes kleines Update **nach** dem Network-Update, nicht in die laufende
Reihenfolge Network → Debugger → IDE hineingeschoben.

---

## Bestandsaufnahme (19.11.)

| Teil | Zustand |
|---|---|
| `build_linux.sh` | seit v0.2.7 nicht gepflegt; lädt SDL3 nur als Quelltext und baut es nicht; SDL3_ttf fehlt |
| `src/compiler/blitzcc.cpp` | fest auf Windows: `<windows.h>` ohne Schalter, `CreateProcessW`, `GetModuleFileNameW`, `std::wstring`-Befehlszeile, MinGW-Pfade, `-lws2_32 -lwinhttp -lopengl32 -mwindows`, `.exe`, Kopie von `SDL3.dll` |
| Laufzeit `bb_*.h` | SDL3 + OpenGL, Windows-Stellen hinter `_WIN32`; Netz/Socket/Datei/System haben meist `#else`. Ohne Linux-Zweig: Gamma (GDI), Schriftliste (BUG-218), Konsole, Scancodes, SEH, Filme (DirectShow), `winhttp` |
| IDE | startet exe direkt (`build-runner.js`, `toolchain.js`), beendet mit `taskkill` |
| Tests / Messlatte | nur Windows |

## Pakete

### L0 Testumgebung: WSL2 auf D: — **fertig 19.11.**
Vom Nutzer nach der Anleitung unten eingerichtet (SVM Mode im BIOS war aus). Stand: WSL 3.0.5,
Ubuntu 24.04.5 in `D:\WSL\Ubuntu`, Benutzer `administrator`, GCC 13.3, CMake 3.28,
OpenGL 4.6 über D3D12 auf der RTX 5070 Ti (`GALLIUM_DRIVER=d3d12`). Repo geklont nach
`~/bltznxt` (Stand b1ee388). `sudo` braucht das Passwort des Nutzers — Paketinstallationen
macht der Nutzer, oder Claude nennt den Befehl.

### L1 Build-Script — **fertig 19.11.** (in WSL geprüft)
- `build_linux.sh` neu: GCC und CMake vom System; SDL3 und SDL3_ttf vom System, wenn
  pkg-config die Version findet, sonst Quelltext laden (SHA-256 geprüft) und nach
  `libs/linux/{sdl3,sdl3_ttf}` bauen (shared, Release). Zweiter Lauf verwendet alles wieder;
  eine ältere eigene Version wird erkannt und neu gebaut.
- Abhängigkeiten über `pkg-config`/PATH geprüft, Installationsbefehl für apt/dnf/pacman/zypper
  (nur apt ausprobiert). Schalter `--check`, `--setup-only`, `--clean-deps`, `--help`.
- **SDL3 3.2.10 statt 3.2.0:** SDL3_ttf 3.2.2 verlangt aus dem Quelltext SDL3 ≥ 3.2.6 (das
  Windows-Fertigpaket verbirgt das). Später prüfen, ob Windows auf dieselbe 3.2.x zieht.
- `.gitattributes`: `*.sh` mit LF; `build_linux.sh` in git ausführbar.
- Voller Lauf scheitert erwartungsgemäß an `blitzcc.cpp:28` (`windows.h`) → L2.
- Benötigte Ubuntu-Pakete (24.04): siehe Ausgabe von `./build_linux.sh --check`.

### L2 Plattformschicht in blitzcc — **fertig 19.11.**
- Neu `src/compiler/host.h` (nur von `blitzcc.cpp` eingebunden, bringt `windows.h` mit):
  ANSI-Pfadhelfer (unter Linux durchgereicht), `host::run` (Windows `CreateProcessW` mit
  korrektem Quoting nach CommandLineToArgvW; Linux `fork`/`execvp`), `host::exeDir`
  (`GetModuleFileNameW` ↔ `/proc/self/exe`), `host::processId`, `host::findInPath`,
  `host::kExeSuffix` (".exe" ↔ "").
- Befehlszeile als Argumentliste (`host::Args`) statt `wstring`.
- Linux-Linkzeile: g++ aus dem PATH; SDL3/SDL3_ttf aus `libs/linux/` oder vom System;
  `-lSDL3_ttf -lSDL3 -lGL -lpthread -ldl -static-libstdc++ -static-libgcc`. Die eigene SDL
  wird **wie die DLLs unter Windows neben das Programm kopiert** (`libSDL3.so.0`,
  `libSDL3_ttf.so.0`), gefunden über `-Wl,-rpath,$ORIGIN` — der Programmordner läuft so auch
  auf einem anderen Rechner. Kein `-static`, kein `-mwindows`, kein `-Wa,-mbig-obj`.
- Windows geprüft (gezielt): Umlaut-Test der Suite, Quelle im Umlaut-Ordner, GUI-Subsystem bei
  Graphics, IDE-Modus mit Argumenten (Leerzeichen, Backslash am Ende — gleiche Ausgabe wie der
  alte blitzcc), 6 Negativtests mit Diagnose, Fehler im IDE-Format. Volle Suite nicht gelaufen.
- Linux geprüft (WSL): blitzcc baut über `build_linux.sh`, `-q`-Umgebungsprüfung still;
  Textprogramm, 2D (Rect, Text, ReadPixel) und 3D (Würfel, RenderWorld) laufen.
- **Nicht geprüft unter Linux:** IDE-Modus (`blitzide`), Programmargumente.

### L3 Laufzeit übersetzbar — **übersetzt schon (19.11.)**
- Erster Lauf: einziger Fehler war `<sys/ioctl.h>` in `bb_socket.h` (behoben). Die übrigen
  Punkte unten betreffen also **Verhalten**, nicht das Übersetzen — einzeln prüfen.
- Gamma über SDL oder weglassen; Schriften über fontconfig mit Ersatztabelle (Arial → Liberation
  Sans usw.); Scancodes über SDL; Absturzmeldung über Signal-Handler; Liste-Server per eigenem
  HTTP statt `winhttp`.
- **Filme zunächst nicht** — in KNOWN_ISSUES.

### L4 Pfade wie unter Windows
- Alte Programme schreiben `"Media\Bild.PNG"`. Beim Öffnen: `\` → `/`, Suche ohne Rücksicht auf
  Groß-/Kleinschreibung, Laufwerksbuchstaben behandeln. Gilt für alle Lade- und Dateibefehle.
- Wichtigster Punkt für das Kompatibilitätsziel.

### L5 IDE
- Name von `blitzcc` und Programmen je Plattform, Beenden per Signal statt `taskkill`.
- Electron nativ für Linux (`npm start` in WSL testen).

### L6 Prüfen
- Testsuite und Beispiel-Messlatte in WSL laufen lassen, Abweichungen in Buglist.md.
- Gegencheck durch den Tester auf echtem Linux.

---

## Anleitung: Linux (WSL2) auf Laufwerk D: einrichten

Für Windows 11. Dauer etwa 20–30 Minuten, ein Neustart.

### 1. WSL einschalten (PowerShell **als Administrator**)
```powershell
wsl --install --no-distribution
```
Danach **neu starten**. Meldet Windows, die Virtualisierung sei aus: im BIOS/UEFI „Intel VT-x“
bzw. „AMD-V/SVM“ einschalten.

**Gigabyte-Board mit AMD-CPU (hier: B550 GAMING X V2, Ryzen 7 5800X3D):** Die Einstellung heißt
**„SVM Mode“** und ist ab Werk **aus**. Im BIOS (Entf beim Start) mit **F2** in den
„Advanced Mode“ wechseln, dann **Tweaker → Advanced CPU Settings → SVM Mode → Enabled**,
mit F10 speichern. Das Wort „Virtualisierung“ kommt dort nirgends vor.
Prüfen in Windows: Task-Manager → Leistung → CPU → „Virtualisierung: Aktiviert“.
Typischer Fehler, solange SVM aus ist: `HCS_E_HYPERV_NOT_INSTALLED`.

### 2. Ubuntu direkt auf D: installieren (normale PowerShell)
```powershell
wsl --update
```
```powershell
wsl --install -d Ubuntu-24.04 --location D:\WSL\Ubuntu
```
Beim ersten Start Benutzername und Passwort für Linux festlegen (frei wählbar, nicht das
Windows-Passwort nötig).

Kennt die WSL-Version `--location` noch nicht: normal installieren und dann verschieben:
```powershell
wsl --install -d Ubuntu-24.04
```
```powershell
wsl --manage Ubuntu-24.04 --move D:\WSL\Ubuntu
```
Notlösung, falls beides nicht geht: `wsl --export Ubuntu-24.04 D:\WSL\ubuntu.tar`, dann
`wsl --unregister Ubuntu-24.04`, dann `wsl --import Ubuntu-24.04 D:\WSL\Ubuntu D:\WSL\ubuntu.tar`.

### 3. Prüfen
```powershell
wsl -l -v
```
Ubuntu-24.04 muss mit **VERSION 2** erscheinen.

### 4. Grundausstattung (in Ubuntu)
```bash
sudo apt update && sudo apt upgrade -y
sudo apt install -y build-essential cmake git pkg-config mesa-utils
glxinfo -B          # zeigt die Grafikkarte; Fenster laufen über WSLg ohne weiteres Zutun
```
Steht dort `llvmpipe`, rechnet die CPU. Die Grafikkarte holt man so dazu (dauerhaft):
```bash
echo 'export GALLIUM_DRIVER=d3d12' >> ~/.profile
echo 'export GALLIUM_DRIVER=d3d12' >> ~/.bashrc
```
Danach zeigt `glxinfo -B` „D3D12 (…Grafikkarte…)“.

### 5. Wichtig: im Linux-Dateisystem arbeiten
Das Repo ist zwar unter `/mnt/f/dev/projects/bltznxt` sichtbar, dort ist das Dateisystem aber
**ohne Groß-/Kleinschreibung** (wie Windows) und langsam. Für echte Linux-Tests klonen:
```bash
cd ~
git clone /mnt/f/dev/projects/bltznxt bltznxt
```

### 6. Optional: Wine, um den Tester nachzustellen
```bash
sudo dpkg --add-architecture i386 && sudo apt update
sudo apt install -y wine64
wine --version
```

### 7. Optional: Node für die IDE nativ
```bash
sudo apt install -y nodejs npm
```

### Aufräumen / Entfernen
```powershell
wsl --unregister Ubuntu-24.04
```
löscht die Linux-Installation samt Daten auf D: endgültig.
