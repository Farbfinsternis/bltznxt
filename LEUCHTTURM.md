# BLTZNXT — Leuchtturm

Stand: 2026-09-26 · Entwurf

Ein kleiner Arena-Shooter im Stil von Quake III, geschrieben als **gewöhnliches Blitz3D-Programm**:
Blitz-Code, Blitz-Befehle, der Blitz3D-kompatible Renderer. Neu sind nur die Daten — Karte, Waffen
und Items kommen aus Blender, als glTF (`.glb`).

Das Projekt beantwortet eine Frage, die blox-n-balls nicht beantworten kann: **Kann man mit BLTZNXT
heute etwas Neues bauen, mit heutigen Werkzeugen?** Alte Programme laden weiter ihre `.x`- und
`.3ds`-Dateien; wer neu anfängt, exportiert aus Blender und bekommt dasselbe Blitz3D.

## Umfang

„Fertig" heißt:

| Teil | Inhalt |
|---|---|
| Karte | Eine Arena, in Blender gebaut, mit gebackenen Lightmaps |
| Bewegung | Laufen, Springen, Luftkontrolle, Strafen — das Gefühl von Quake III, nicht seine Formeln |
| Waffen | Drei, je mit eigener Munition |
| Items | Munition für jede Waffe, Rüstung, Gesundheit; erscheinen nach einer Zeit wieder |
| Oberfläche | Anzeige von Gesundheit, Rüstung, Munition und aktueller Waffe |

Vorschlag für die Waffen, weil sie drei verschiedene Techniken abdecken:

| Waffe | Art | Technik |
|---|---|---|
| Maschinengewehr | Sofort-Treffer, schnelle Folge, wenig Schaden | `LinePick` je Schuss |
| Raketenwerfer | Geschoss mit Flächenschaden, Rocket-Jump | Entity mit Kollision, Explosion mit Radius |
| Railgun | Sofort-Treffer, langsam, viel Schaden, sichtbare Spur | `LinePick`, Spur als Sprite-Kette oder Mesh |

**Nicht im Umfang:** eine zweite Karte, Spielmodi, Menüs über das Nötigste hinaus, Bots,
Mehrspieler. Gegner sind eine offene Entscheidung (unten).

## glTF im alten Pfad

Der glTF-Lader liest aus der Datei, was der Blitz3D-Renderer darstellen kann, und lässt den Rest
liegen. `LoadMesh("arena.glb")` und `LoadAnimMesh("waffe.glb")` funktionieren wie bei `.x`.

| glTF | Blitz3D |
|---|---|
| Knoten mit Name und Hierarchie | Entity bzw. Pivot, `EntityName`, `FindChild`, `GetChild` |
| Mesh-Primitive | Surface |
| Material: Grundfarbe, Alpha | Brush: `BrushColor`, `BrushAlpha` |
| `baseColorTexture` (UV-Satz 0) | Texturschicht 0 |
| Lightmap auf `TEXCOORD_1` | Texturschicht 1, multipliziert |
| `doubleSided` | FX 16 |
| `alphaMode` MASK / BLEND | Textur-Flag 4 bzw. Alpha-Blend |
| emissiv | FX 1 (Näherung) |
| Vertexfarben | Vertexfarben, FX 2 |
| Skinning, Animationen | Blitz-Knochen und Animationssequenzen (`Animate`, `AnimSeq` …) |
| Metallic, Roughness, Normal Maps, KHR-Erweiterungen | im alten Pfad ignoriert — der moderne Pfad liest sie später aus derselben Datei |

glTF ist rechtshändig, Y oben, in Metern; Blitz3D linkshändig. Der Lader spiegelt eine Achse und
kehrt die Umlaufrichtung um, wie der `.3ds`-Lader es mit seinem Achsentausch schon tut. Die
Einzelheiten werden am Original mit einem `.x`-Gegenstück gemessen, nicht geraten.

## Die Karte in Blender

Die Karte ist eine einzige `.glb`. Alles, was das Spiel über sie wissen muss, steckt in den
**Objektnamen** — damit reichen `EntityName`, `CountChildren` und `GetChild`, und es braucht keinen
neuen Befehl.

| Blender-Objekt | Name | Wirkung im Spiel |
|---|---|---|
| Sichtbare Geometrie | frei | Gerendert, nicht kollidierend |
| Kollisionsgeometrie | endet auf `-col` | Unsichtbar, `EntityType` für die Kollision; einfacher als die sichtbare |
| Spawnpunkt | `spawn` | Leeres Objekt; Position und Blickrichtung des Spielers |
| Waffe | `weapon_mg`, `weapon_rl`, `weapon_rail` | Leeres Objekt; dort liegt die Waffe |
| Munition | `ammo_mg`, `ammo_rl`, `ammo_rail` | Leeres Objekt |
| Rüstung | `armor_25`, `armor_50`, `armor_100` | Leeres Objekt, Zahl = Punkte |
| Gesundheit | `health_25`, `health_50`, `health_100` | Leeres Objekt, Zahl = Punkte |

Blender hängt bei Kopien `.001`, `.002` an; das Spiel liest nur den Teil vor dem Punkt.

Weitere Regeln:

- **Maßstab:** 1 Blender-Einheit = 1 Meter = 1 Blitz-Einheit. Spielerhöhe etwa 1,8.
- **Lightmap:** in Cycles gebacken, auf einen zweiten UV-Satz namens `Lightmap`; die gebackene
  Textur liegt als Occlusion-Textur am Material, damit der Export sie mitnimmt.
- **Export:** glTF Binary (`.glb`), „+Y Up", Modifier anwenden, Custom Properties dürfen mit,
  werden aber nicht gebraucht.
- **Sichtbarkeit:** Für eine Arena genügt das vorhandene Frustum-Culling; kein PVS, keine Portale.

Freie Assets gibt es unter CC0 unter anderem von Kenney und Quaternius (Modelle) und Poly Haven
(Texturen). Inhalte aus Quake III sind tabu.

## Was BLTZNXT dafür braucht

| Baustein | Stand | Gehört zu |
|---|---|---|
| Kollision, `LinePick`, `EntityType`, `Collisions` | vorhanden | — |
| Sprites, Partikel, mehrere Texturschichten, 2D über 3D | vorhanden | — |
| `FindChild`, `GetChild`, `EntityName`, `CopyEntity` | vorhanden | — |
| Animationssystem (`Animate`, `SetAnimTime`, `AnimSeq`, `ExtractAnimSeq` …) | fehlt | Phase 1 — alte `.x`/`.b3d` brauchen es ebenso |
| glTF-Lader, statisch: Geometrie, Hierarchie, Brushes, zwei UV-Sätze | fehlt | Leuchtturm |
| glTF-Lader, animiert: Skinning, Animationen auf dem Blitz-System | fehlt | Leuchtturm, nach dem Animationssystem |
| 3D-Klang (`CreateListener`, `EmitSound`) | fehlt | Phase 1 |

## Reihenfolge

| Schritt | Was | Ergebnis |
|---|---|---|
| 1 | Animationssystem, am Original gemessen | Animierte `.x`-Modelle laufen wie in Blitz3D |
| 2 | glTF statisch | Die Arena lädt, mit Lightmap |
| 3 | Spiel: Bewegung und Kollision | Man läuft und springt durch die Arena |
| 4 | Spiel: Waffen | Drei Waffen feuern, Treffer und Explosionen |
| 5 | glTF animiert | Waffenmodelle mit Animation in der Hand |
| 6 | 3D-Klang | Schüsse und Items sind räumlich zu hören |
| 7 | Spiel: Items und Anzeige | Aufsammeln, Wiedererscheinen, HUD — Umfang erfüllt |

Schritt 3 kann mit einem Platzhalter beginnen, sobald Schritt 2 steht; die Waffen in Schritt 4
dürfen bis Schritt 5 statisch sein.

## Offene Entscheidungen

- [ ] **Gegen wen spielt man?** Zuerst Ziele und Zeitrennen, dann Bots (Wegpunkte als leere
  Objekte `waypoint` in Blender) oder Mehrspieler (Listen-Server, siehe VISION.md, Abschnitt
  Netzwerk)?
- [ ] **Assets:** selbst gebaut, CC0-Pakete oder gemischt?
- [ ] **Wo lebt das Spiel:** im Repository unter `samples/`, oder als eigenes Repository?
- [ ] **Name** des Spiels.
