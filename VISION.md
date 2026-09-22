# BLTZNXT — Vision

Stand: 2026-09-22

Der Wert von BLTZNXT liegt in der Kompatibilität, nicht in PBR: zwanzig Jahre Blitz3D-Code
laufen wieder auf Maschinen ohne DX7. Das kann keine andere Engine nachbauen, und das NEXT
sollte diese Position ausbauen statt sie zu verlassen.

## Die Wette

Der Burggraben ist Phase 1, nicht Phase 2. PBR gibt es 2026 überall: Godot, Unity, Raylib,
three.js — niemand wechselt die Engine wegen eines Cook-Torrance-Shaders.

Was nur BLTZNXT hat: Blitz3D-Code, Tutorials, Forenschnipsel und fertige Spiele aus zwanzig
Jahren laufen unverändert. Diese Position kann sich niemand mehr erarbeiten, weil niemand mehr
bereit ist, x87-Rundung, D3D7-Füllregeln und DirectDraw-Timing nachzumessen. Genau das ist die
Arbeit der letzten Monate.

Daraus folgt die Rahmung des NEXT: nicht „BLTZNXT wird auch eine moderne Engine", sondern
**„dein alter Code läuft weiter, und du kannst ihn Datei für Datei modernisieren"**. Das ist ein
Versprechen, das keine andere Engine geben kann.

## Phase 1: wann ist „fertig"?

„Fertig" braucht ein Kriterium, das kein Bugzähler ist. 46 offene Punkte klingen nach Ziellinie,
aber jedes neue Programm bringt neue Funde — blox-n-balls allein hat ein Dutzend geliefert, die
MD2-Arbeit vier weitere.

Besser ein Korpus: *diese* Installationsdemos und *diese* fünf Community-Spiele laufen
pixelvergleichbar durch. Das ist Phase 1, fertig, eingefroren. Alles danach ist Pflege, nicht
Projektziel.

Zwei Lücken stehen dem Anspruch „jeder existierende Blitz3D-Code" heute im Weg:

| Lücke | Stand | Warum sie den Anspruch blockiert |
|---|---|---|
| Userlibs (`.decls`) | `userlibs/` ist leer, `.decls` wird nirgends verarbeitet, steht in keinem offenen Punkt | Sehr viele reale Programme binden DLLs so ein. Technisch unangenehm, weil diese DLLs 32-Bit sind |
| Zielplattformen | Der Compiler hängt an der Windows-API (WEAK-24), `build_linux.sh` baut nicht | Wenn „aktuelle Systeme" Linux, macOS oder den Browser einschließt, ist das Phase 1 und nicht NEXT |

## Die Naht zwischen alt und neu

PBR ist doch ein Architekturthema — nur nicht dort, wo [ROADMAP3D.md](ROADMAP3D.md) es verortet.
Dort steht: „PBR ist kein Architekturthema, nur ein anderes Lighting-Modell im Shader." Für den
Shader stimmt das, für das Ganze nicht.

Vernünftiges PBR braucht lineares Rechnen, IBL und einen Tonemapper. Ohne die drei sieht es mit
acht Punktlichtern und ohne Schatten **schlechter** aus als die jetzige feste Pipeline. Mit ihnen
ändern sich genau die Zahlen, die die Suite festnagelt: BUG-66, BUG-91, BUG-92 und WEAK-25 sichern
gemessene D3D7-Helligkeiten per `ReadPixel`.

Der Farbraum ist also die Naht, an der alt und neu getrennt gehören — nicht das Renderer-Backend.

Und die Trennung gehört **nicht pro Entity**, auch wenn `EntityMetallic` verlockend klingt. Licht,
Ambient, Tonemapping und die Halbpixel-Verschiebung sind Eigenschaften der Szene, nicht des
Objekts; gemischt in einem Bild ergeben sie Resultate, die niemand erklären kann. Besser pro
Programm oder pro Kamera: ein Befehl, mit dem ein Programm einmal den modernen Pfad wählt — ab da
lineare Farben, PBR-Materialien, kein D3D7-Clamp, keine 1/2 − 1/256-Verschiebung. Alte Programme
sagen ihn nie und merken nichts, und die Suite bleibt in zwei sauber getrennte Korpora teilbar.

## Reihenfolge für NEXT

Die Liste GLTF/PBR/Shader ist richtig, aber in dieser Reihenfolge falsch sortiert.

| Schritt | Was | Warum hier |
|---|---|---|
| 1 | Farbraum und Tonemapping als Schalter | Unsichtbar, aber Voraussetzung für alles Weitere. Legt die Naht fest, bevor Features daran hängen |
| 2 | Materialmodell und IBL | Das eigentliche PBR. Mit vorhandenen Meshes testbar, ohne neues Dateiformat |
| 3 | glTF | Der Behälter, der genau diese Materialien transportiert. Vorher gebaut lädt man Modelle, die man nicht korrekt schattieren kann |
| 4 | Eigene Shader | Permanente API-Festlegung — zuletzt, mit Bedacht |

GLSL als API freizugeben legt BLTZNXT auf OpenGL fest, solange es das Projekt gibt: kein Vulkan,
kein WebGPU, kein Metal. Das darf eine Entscheidung sein, aber eine bewusste.

## Offene Entscheidungen

Fünf Fragen stehen vor dem ersten Schritt. Keine davon ist technisch schwer, alle färben auf Jahre
ab.

- [ ] Welcher Korpus definiert „Phase 1 fertig"? Welche Demos, welche Spiele?
- [ ] Gehören Userlibs (`.decls`) zum Anspruch — und wenn ja, wie mit 32-Bit-DLLs?
- [ ] Was heißt „aktuelle Systeme": Windows allein, oder auch Linux, macOS, Browser?
- [ ] Wo liegt der Schalter zwischen altem und modernem Pfad — pro Programm oder pro Kamera?
- [ ] Wird GLSL die Shader-API, mit der Festlegung auf OpenGL?

Ein Gedanke zum Schluss, weil er in der Liste fehlt: Wäre es meine Engine, stünde **vor** PBR der
Browser. BLTZNXT ist ein Transpiler nach C++, Emscripten ist damit näher, als es aussieht — und
„dein Blitz3D-Spiel von 2004 läuft in einem Link" schlägt jeden PBR-Screenshot.
