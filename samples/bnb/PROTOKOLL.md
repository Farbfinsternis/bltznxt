# Blox'n'Balls – Protokoll: wie das Remaster BLTZNXT beeinflusst hat

Blox'n'Balls ist das Remaster eines alten Blitz3D-Spiels (Original unter
`F:\dev\Blitz3D\Games\bnb`), neu geschrieben als BLTZNXT-Beispiel. Die Anforderungen
stehen in [FRAGENKATALOG.md](FRAGENKATALOG.md).

**Regel (31.10.2026):** Tritt beim Bau ein Fehler in BLTZNXT zutage, hat dessen Behebung
Vorrang vor der Arbeit am Spiel. Jeder Fund wird doppelt festgehalten: in der Bugliste von
BLTZNXT (`Buglist.md`, dort wird er weitergeführt) und hier aus Sicht des Spiels – mit
Befund, Umweg im Spiel (falls nötig) und Commit der Behebung. So bleibt nachvollziehbar,
wie die Entwicklung des Remasters die Entwicklung von BLTZNXT beeinflusst hat.

Stand: 2026-11-01, BLTZNXT 0.7.1 (Commit ff8674e), Windows 11.

| # | Art | Thema | Schwere | Bugliste |
|---|-----|-------|---------|----------|
| 1 | Ausbau | Keine Normal Maps (glTF-Lader liest nur die Grundfarbe) | mittel – zurückgestellt | WEAK-35 |
| 2 | Fehler | `ReadFile` auf eine fehlende Datei meldet sich auf stderr | niedrig – **behoben** (`6b4f82a`) | BUG-228 ✅ |
| 3 | Fehler | glTF `doubleSided` beleuchtet die Rückseite nicht | mittel – **behoben** (`6c9ef83`) | BUG-229 ✅ |
| 4 | Fehler | Nach einem zweiten `Graphics3D` zeichnet `RenderWorld` nichts | mittel – **behoben** (`ff8674e`) | BUG-230 ✅ |

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

## 3. glTF `doubleSided` beleuchtet die Rückseite nicht

**Was:** Die Lampenschirme des Tischrands bestanden zuerst aus zwei deckungsgleichen
Schichten (Vorder- und Rückseite übereinander) – mit doppelseitigem Material ergab das
Z-Fighting, ein Fehler im Modell. Nach der Korrektur in Blender (31.10.) war das Flimmern
weg, aber die Schirme fast schwarz: von den verbliebenen Flächen zeigen von oben gesehen
viele ihre Rückseite. BLTZNXT setzte für glTF `doubleSided` nur FX 16 (Rückseiten
zeichnen); die Spezifikation verlangt dazu, die Rückseite mit umgedrehter Normale zu
beleuchten.

**Folge im Spiel:** dunkle Lampenschirme, obwohl das Licht von oben kommt.

**Behoben:** sofort, vor der Weiterarbeit am Spiel: eigenes Brush-Merkmal „zweiseitig
beleuchten“ nur für glTF, der Shader dreht für Rückseiten die Normale um. FX 16 bleibt wie
im Original (Blitz-Programme sehen unverändert aus). Regressionstest
`tests/test_bug229_gltf_zweiseitig`, Commit `6c9ef83`.

---

## 4. Nach einem zweiten `Graphics3D` zeichnet `RenderWorld` nichts

**Was:** Nach dem Release von 0.1.0 meldete der Nutzer (01.11.), Auflösung und Vollbild in
den Optionen bewirkten nichts. Das Spiel übernahm sie erst nach einem Neustart. Beim Umbau
auf sofortiges Umschalten (neues `Graphics3D`, danach alles neu laden) blieb die 3D-Welt
weiß, nur Texte und HUD-Zahlen waren zu sehen. BLTZNXT übersetzte die Shader nur einmal;
nach dem neuen GL-Kontext galten sie weiter als fertig.

**Folge im Spiel:** ohne Fix kein Moduswechsel zur Laufzeit möglich.

**Behoben:** sofort, vor der Weiterarbeit am Spiel: beim Schließen des Fensters gibt die
Laufzeit die Shader-Programme frei, solange der alte Kontext lebt; `RenderWorld` übersetzt
sie im neuen Kontext. Regressionstest `tests/test_bug230_graphics3d_zweimal`, Commit
`ff8674e`.

**Im Spiel:** `Gfx_Change()` in `bloxnballs.bb` öffnet den neuen Modus, lädt alles, was
zum Grafikmodus gehört, und gibt Blöcken, Bällen und Münzen des laufenden Spiels neue
Entities (`Blocks_Rebuild`, `Balls_Rebuild`, `Coins_Rebuild`) – der Spielstand bleibt.
