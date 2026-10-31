# Blox'n'Balls – Protokoll: wie das Remaster BLTZNXT beeinflusst hat

Blox'n'Balls ist das Remaster eines alten Blitz3D-Spiels (Original unter
`F:\dev\Blitz3D\Games\bnb`), neu geschrieben als BLTZNXT-Beispiel. Die Anforderungen
stehen in [FRAGENKATALOG.md](FRAGENKATALOG.md).

**Regel (31.10.2026):** Tritt beim Bau ein Fehler in BLTZNXT zutage, hat dessen Behebung
Vorrang vor der Arbeit am Spiel. Jeder Fund wird doppelt festgehalten: in der Bugliste von
BLTZNXT (`Buglist.md`, dort wird er weitergeführt) und hier aus Sicht des Spiels – mit
Befund, Umweg im Spiel (falls nötig) und Commit der Behebung. So bleibt nachvollziehbar,
wie die Entwicklung des Remasters die Entwicklung von BLTZNXT beeinflusst hat.

Stand: 2026-10-31, BLTZNXT 0.7.1 (Commit 4417b2e), Windows 11.

| # | Art | Thema | Schwere | Bugliste |
|---|-----|-------|---------|----------|
| 1 | Ausbau | Keine Normal Maps (glTF-Lader liest nur die Grundfarbe) | mittel – zurückgestellt | WEAK-35 |
| 2 | Fehler | `ReadFile` auf eine fehlende Datei meldet sich auf stderr | niedrig – **behoben** (`6b4f82a`) | BUG-228 ✅ |

---

## 1. Keine Normal Maps

**Was:** Beim Klären der Anforderungen (Fragenkatalog A5/F39) sollte das Remaster
glTF-Modelle mit Normal Maps vorführen. Der glTF-Lader (`src/compiler/bb_loader_gltf.h`)
übernimmt aber nur `baseColorTexture`; Metallic, Roughness und Normal Maps lässt er laut
Kopfkommentar liegen. `BrushNormalMap` gibt es bisher nur als Plan in `ENGINE_DESIGN.md`.

**Folge im Spiel:** Der Tischrand (`assets/meshes/tableborder.glb`, Diffuse 2048²
eingebettet) erscheint ohne Oberflächenrelief; die Normal Map aus `_work/` bleibt ungenutzt.

**Entscheidung (31.10.):** Das Remaster startet ohne Normal Maps. Alle neuen `.glb` bringen
sie trotzdem eingebettet mit, damit beim Nachrüsten nur die Engine geändert werden muss.
Kein Fehler im Sinne des Kompatibilitätsziels, daher nicht vorgezogen.

## 2. `ReadFile` meldet fehlende Dateien auf stderr

**Was:** Beim ersten Testlauf (31.10.) fehlten Einstellungen, Bestenliste und
Autospeicherstand noch – das ist normal, das Spiel prüft mit `If Not ReadFile(...)`, ob
sie da sind. BLTZNXT schrieb dabei jedes Mal `[runtime] Cannot open file: ...` auf stderr.
Das Original liefert still 0, auch im Debug-Build (nachgemessen mit `blitzcc -d`).

**Folge im Spiel:** keine – nur drei Störzeilen in der Konsole bei jedem Start.

**Behoben:** sofort, vor der Weiterarbeit am Spiel (`src/compiler/bb_file.h`,
`bb_file_open_`); Regressionstest `tests/test_bug228_readfile_silent`, Commit `6b4f82a`.
