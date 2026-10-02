# BLTZCRFT – Protokoll: wo BLTZNXT die Arbeit erschwert hat

BLTZCRFT ist das zweite Leuchtturmprojekt (nach Friendly Fire, Arbeitstitel „Blockwelt“): ein Minecraft-artiger
Klon, geschrieben als gewöhnliches Blitz3D-Programm. Dieses Protokoll hält fest, wo
BLTZNXT beim Bau durch Fehler, fehlende Befehle oder Eigenheiten im Weg stand – mit
Messwerten, dem Umweg im Spiel und einem Vorschlag.

Stand: 2026-10-02, BLTZNXT 0.6.2 (Commit 781f1f0), Windows 11, RTX 5070 Ti.

Jeder Punkt steht auch in der Bugliste von BLTZNXT (`Buglist.md`, Spalte „Bugliste“);
dort wird er weitergeführt, hier bleibt der Befund aus Sicht des Spiels.

| # | Art | Thema | Schwere | Bugliste |
|---|-----|-------|---------|----------|
| 1 | Leistung | `blitzcc` übersetzt ohne Optimierung (`-O0`), kein Schalter dafür | hoch – **behoben** (`54392bc`) | BUG-189 ✅ |
| 2 | Leistung | Nach jeder Geometrieänderung rechnet `RenderWorld` die Hüllquader **aller** Meshes neu | mittel – **behoben** (`2c4beca`) | BUG-190 ✅ |
| 3 | fehlender Befehl | `CameraFogMode` / `CameraFogRange` / `CameraFogColor` fehlen | mittel | BUG-191 |
| 4 | Einschränkung | Texturen immer linear gefiltert, kein Punktfilter für Pixelkunst | niedrig | WEAK-27 |
| 5 | Leistung | Mehrdimensionale `Dim`-Felder sind verschachtelte `std::vector` mit `.at()` je Dimension | mittel | WEAK-26 (Ursprung WEAK-07) |
| 6 | Erweiterung | Chunks und Objekte ploppen auf, statt weich zu erscheinen – Prototyp „Einblenden per Raster“ | mittel | WEAK-28 |

---

## 1. Keine Compiler-Optimierung (`-O0`)

**Was:** `blitzcc` ruft g++ ohne `-O`-Stufe auf (`src/compiler/blitzcc.cpp`, Aufbau von
`cmd`), also mit `-O0`. Es gibt keinen Schalter dafür; `-release` ist laut Hilfe nur ein
Alias für die Vorgabe. Die Laufzeit steckt komplett in Headern (`bb_*.h`) – Banks hinter
`std::unordered_map`, Felder mit `.at()`, kleine Hilfsfunktionen – und genau dieser Code
wird ohne Optimierung extrem langsam.

**Gemessen** (`werkzeug/foto.bb`, Sichtweite 8: 361 Chunks erzeugen, 289 bauen, 380 297
Flächen). Derselbe Quelltext, einmal wie BLTZNXT übersetzt, einmal das erzeugte C++
(`blitzcc -c`) von Hand mit `-O2`:

| | BLTZNXT (-O0) | -O2 | Faktor |
|---|---|---|---|
| Chunk erzeugen | 13,5 ms | 0,67 ms | 20 |
| Chunk bauen (Mesh) | 27,5 ms | 1,14 ms | 24 |
| Bild rendern | 2,7 ms | 0,4 ms | 7 |

Die Übersetzungszeit war in beiden Fällen gleich (~15 s). Auch ein einzelner
Engine-Aufruf ist teuer: 1000 Vierecke (je 4 × `AddVertex`, 4 × `VertexColor`,
2 × `AddTriangle`) kosten 3,8 ms, also rund 0,4 µs je Aufruf.

**Folge:** Eine Voxelwelt muss beim Laufen ständig Chunks erzeugen und bauen. Mit
41 ms je Chunk passt keiner in ein Bild – das Nachladen ruckelt, beim Fliegen läuft man
der Welt davon.

Nach den Umwegen im Spielcode (unten) sind es 6,9 ms Erzeugen + 8,7 ms Bauen je Chunk
(-O2: 0,6 + 0,9 ms). Der eingebaute Rundflug (`bltzcrft.exe 2026 -rundflug`, 20 s
geradeaus mit 11 m/s, Sichtweite 8, ständiges Nachladen) misst:

| | BLTZNXT (-O0) | -O2 |
|---|---|---|
| Bilder in 20 s | 2 239 | 13 240 |
| mittlere Bildzeit | 8,9 ms | 1,5 ms |
| Bilder über 33 ms (Ruckler) | 158 | 1 (der erste, Hochladen aller Meshes) |

Ohne Optimierung ruckelt das Nachladen also sichtbar (etwa acht Ruckler je Sekunde), mit
-O2 läuft es glatt.

**Umweg im Spiel:** Arbeit je Chunk gesenkt (siehe Abschnitt „Umwege“ unten), Zeitbudget
je Bild, Sichtweite 8.

**Vorschlag:** `-O2` als Vorgabe für Programme (Debug-Build mit `-d` weiter ohne), oder
wenigstens ein Schalter `-O`. Vorher die Testsuite mit `-O2` laufen lassen – Optimierung
kann undefiniertes Verhalten im erzeugten Code sichtbar machen (z. B. Ganzzahlüberlauf).

**Behoben (2026-10-02):** `blitzcc` übersetzt Programme jetzt mit
`-O2 -fwrapv -fno-strict-aliasing`; `-fwrapv` lässt Ganzzahlen wie in Blitz3D überlaufen,
statt dem Optimierer undefiniertes Verhalten zu überlassen. `-d` bleibt `-O0 -g`. Die
Übersetzungszeit blieb bei ~15 s. Rundflug danach: 13 693 Bilder in 20 s, Mittel 1,46 ms,
längstes Bild 23 ms, kein Ruckler.

---

## 2. Hüllquader aller Meshes nach jeder Geometrieänderung

**Was:** `bb_mesh_geom_version_` (`bb_mesh_core.h`) ist ein *globaler* Zähler, den jedes
`AddVertex`/`AddTriangle`/`VertexCoords`… erhöht. Beim Zeichnen vergleicht jedes Mesh
seinen gemerkten Hüllquader mit diesem Zähler (`bb_mesh.h`, Sichtkegeltest) – ist er
veraltet, wird der Hüllquader aus allen Vertices neu berechnet. Ändert sich also *ein*
Mesh, rechnen *alle* ihren Quader neu.

**Gemessen:** Nach dem Neubau eines einzigen Chunks dauert das nächste `RenderWorld`
20–27 ms statt 2,7 ms (-O0), mit -O2 6 ms statt 0,4 ms – bei 289 Chunk-Meshes.

**Folge:** Jedes Bild, in dem ein Chunk nachgeladen oder ein Block abgebaut wird, kostet
diesen Aufschlag zusätzlich. In BLTZCRFT ist das fast jedes Bild, solange man sich
bewegt.

**Umweg im Spiel:** keiner möglich (die Meshes müssen sich ändern). Gebaut wird nur, was
nötig ist.

**Vorschlag:** Version je Mesh (oder je Surface ein Schmutz-Bit, das zum Mesh hochgereicht
wird) statt eines globalen Zählers. Der Kollisionsbaum (`bb_collision.h`) hängt am selben
Zähler und hat dasselbe Problem.

**Behoben (2026-10-02):** Jede Surface trägt den Stempel ihrer letzten Änderung
(`bb_MeshData_::geom`); Hüllbox und Kollisionsbaum eines Meshes vergleichen mit dem
jüngsten Stempel *seiner* Surfaces. Gemessen (-O2, 289 Chunk-Meshes, ein Chunk hinter der
Kamera 50-mal neu gebaut): das Bild danach kostete 5,08 ms, jetzt 0,44 ms – so viel wie
ein Bild ohne jede Änderung.

---

## 3. Kein Kameranebel

**Was:** `CameraFogMode`, `CameraFogRange` und `CameraFogColor` fehlen (KNOWN_ISSUES.md,
„Missing commands“). Ein Programm, das sie benutzt, wird nicht übersetzt.

**Folge:** Der Rand der Sichtweite ist eine harte Kante: Chunks tauchen sichtbar auf, statt
aus dem Nebel zu kommen. Unter Wasser sieht man so weit wie über Wasser.

**Umweg im Spiel:** Himmelsfarbe, `CameraRange` knapp hinter der Sichtweite. Unter Wasser
ein blauer, halbdurchsichtiger Sprite vor der Kamera.

**Vorschlag:** Nebel im Shader (linear, wie Blitz3D `CameraFogMode 1`) – für fast jedes
3D-Spiel mit offener Welt nötig.

---

## 4. Keine Punktfilterung für Texturen

**Was:** Vergrößerte Texturen werden immer linear gefiltert (`bb_texture.h`,
`GL_TEXTURE_MAG_FILTER` fest `GL_LINEAR`). Das ist wie im Original – Blitz3D kannte auch
keinen Punktfilter –, für Pixelkunst aber unpassend: 16×16-Blocktexturen werden aus der
Nähe zu Matsch.

**Umweg im Spiel:** Texturen achtfach vergrößert erzeugt (128×128 statt 16×16); die
lineare Filterung verwischt dann nur noch einen schmalen Saum je Texel.

**Vorschlag:** ein NEXT-Texturflag (z. B. „Punktfilter“), das Blitz3D-Programme nicht
berührt.

---

## 5. Mehrdimensionale Felder sind langsam

**Was:** `Dim feld(17, 129, 17)` wird zu
`std::vector<std::vector<std::vector<int>>>`, jeder Zugriff zu
`feld.at(bb_IntegerContext(x)).at(…).at(…)`. Das sind je Dimension eine Bereichsprüfung
und ein Zeigersprung in einen eigenen Speicherblock. In Blitz3D war ein `Dim`-Feld ein
zusammenhängender Block mit einer einzigen Indexrechnung.

**Gemessen** (-O0): 40 000 Zugriffe der Form `nz_d(5 + fl_ao(2, k, 0, 0), …)` – ein
3D-Feld mit drei 4D-Feldern als Index – kosten 4 ms, also rund 25 ns je einzelner
Feldzugriff; ein Zugriff auf ein eindimensionales Feld kostet etwa 6 ns.

**Umweg im Spiel:** Der Mesh-Bau rechnet mit eindimensionalen Feldern und vorberechneten
Versätzen (`Index = X + Z * 18 + Y * 324`). Zusammen mit dem einmaligen Kopieren des
Chunks aus der Bank ins Feld sank die Bauzeit von 27,5 auf 8,7 ms je Chunk.

**Vorschlag:** ein `Dim`-Feld als ein `std::vector` mit gespeicherten Maßen und einer
Indexrechnung; Bereichsprüfung nur im Debug-Build, wie im Original.

---

## 6. Aufploppen – Einblenden per Raster wie in der Unreal Engine

**Was:** Ein neu gebautes Chunk-Mesh ist von einem Bild aufs nächste ganz da; am Rand der
Sichtweite (hier 128–192 m) ploppen Hügel, Bäume und ganze Chunks auf, beim Fliegen und
im Werbevideo gut zu sehen. Blitz3D bietet dagegen nur `EntityAutoFade`, und das blendet
über echte Transparenz: das Mesh wird durchscheinend, muss sortiert werden, und seine
eigenen Flächen scheinen durcheinander durch – für ein Gelände unbrauchbar.

**Das Verfahren:** Unreal überblendet LOD-Stufen und Sichtgrenzen mit „Dithered Opacity“:
ein festes Pixelraster (Bayer-Matrix) verwirft im Fragment-Shader so viele Pixel, wie
unsichtbar sein sollen. Das Objekt bleibt deckend – Tiefenpuffer, keine Sortierung,
keine Überdeckungsfehler. Unreal glättet das Raster anschließend mit TAA, so dass es
wie echte Transparenz aussieht.

**Prototyp (2026-10-02, nicht committet, `build/prototyp_raster_einblenden.patch`):**
ein Baustein im gemeinsamen Fragment-Teil von TEXTURED und LIT (`bb_shader.h`), ein
8×8-Bayer-Muster, der Abstand planar aus der Bildtiefe (`1/gl_FragCoord.w`, wie
Blitz3D-Nebel). Je Kamera ein Band: ab `near` nimmt die Sichtbarkeit bis `far` auf null
ab. Zum Ausprobieren der Befehl `CameraDitherRange kamera, near#, far#`.

**Ergebnis:**
- Es wirkt: ferne Bäume und Hänge lösen sich gleichmäßig in den Himmel auf, statt hart
  abzubrechen; neue Chunks entstehen im Band fast unsichtbar und werden beim Näherkommen
  dichter.
- Kosten: 0,44 → 0,70 ms je Bild bei 289 Chunks (`discard` schaltet den frühen
  Tiefentest im Band ab). Vernachlässigbar.
- Grenze: Ohne TAA bleibt das Raster sichtbar, aus der Nähe als feines Gitter
  („Screen-Door“); in voller Auflösung wirkt es eher wie Dunst. Da das Muster fest am
  Bildschirm hängt, wandert die Geometrie beim Bewegen durch das Gitter.

**Bewertung und Vorschlag:**
1. **Zuerst Nebel (Punkt 3, BUG-191).** Er ist Blitz3D-Pflicht und löst den größten Teil:
   im Band hat das Gelände dann schon fast die Himmelsfarbe, und ein verworfenes Pixel
   (Himmel) unterscheidet sich kaum von einem gezeichneten (Nebelfarbe) – das Raster
   verschwindet im Nebel. Unreal kombiniert genau so.
2. **Raster als NEXT-Erweiterung dazu**, nur auf ausdrücklichen Aufruf, damit alte
   Programme unverändert aussehen. Zwei Formen mit demselben Shader-Baustein:
   - je Kamera ein Band am Sichtrand (wie der Prototyp) – gegen das Aufploppen beim
     Nachladen;
   - je Entity ein Einblenden über die Zeit (z. B. 300 ms nach dem Erzeugen oder
     `ShowEntity`) – gegen das Aufploppen beim Wechsel von Detailstufen und für neu
     gebaute Meshes in Sichtweite.
3. Optional: `EntityAutoFade` mit Raster statt Transparenz, als Schalter.
4. Später, mit dem modernen Pfad aus VISION.md: TAA, dann sieht das Raster aus wie echte
   Transparenz.

**Offen (Entscheidung):** Namen und Form der neuen Befehle – laut VISION.md ist jeder
Befehl jenseits von Blitz3D eine bewusste API-Festlegung.

---

## Geprüft und *kein* BLTZNXT-Problem

Damit sie nicht noch einmal verdächtigt werden:

- **Lokale Variablen in Blöcken (BUG-100):** Ein `x# = …` zum ersten Mal in einem
  `If`-Zweig und `x = …` im `Else`-Zweig verhalten sich wie in Blitz3D (eine Variable für
  die ganze Funktion); nachgeprüft mit einem Minimalbeispiel. Ein durchlöchertes Gelände
  kam stattdessen von einem eigenen Fehler: `Dim feld(…)` ohne `#` ist ein Ganzzahlfeld –
  wie in Blitz3D.
- **Einzeiliges `If … Then a : b`** führt beide Anweisungen nur unter der Bedingung aus,
  wie im Original. Ein scheinbar verschluckter `Print` war eine Namensgleichheit: eine
  `Global stamm` im Test wurde von der gleichnamigen Variable in `Baum()` überschrieben –
  auch das ist Blitz3D-Verhalten.
- Meshes mit 32-Bit-Indizes: ein Chunk darf mehr als 65 535 Vertices haben.
- `WritePixelFast` in `TextureBuffer` mit Mipmaps (Flag 8) funktioniert.

## Umwege im Spielcode

- Kollision des Spielers direkt gegen das Blockraster statt `Collisions`/`EntityType`: kein
  BLTZNXT-Mangel, aber schneller und ohne Nähte zwischen den Chunk-Meshes.
- Jede Textur ist eine eigene Textur samt eigener Surface statt eines Atlas – ein Atlas
  blutet mit Mipmaps an den Kachelrändern (auch in Blitz3D so).

Wegen Punkt 1 und 5 (Leistung ohne Optimierung) zusätzlich:

- Mesh-Bau: der Chunk samt Rand wird einmal aus den Banks in eindimensionale Felder
  kopiert, statt für jeden Nachbarn `PeekByte` über eine Funktion aufzurufen (jeder
  `PeekByte` sucht die Bank in einer Hash-Tabelle). Bauen: 27,5 → 8,7 ms je Chunk.
- Höhlen: das 3D-Rauschen liegt auf einem groben Gitter (alle 4 Blöcke); je Säule wird
  einmal in der Ebene interpoliert, je Block nur noch in der Höhe, ohne Funktionsaufruf.
  Das zweite Rauschfeld wird nur befragt, wenn das erste nahe null liegt. Erzeugen:
  13,5 → 6,9 ms je Chunk.
- Mit `-O2` wären diese Umwege weitgehend unnötig gewesen.
