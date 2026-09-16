# BlitzNext Engine-Entwurf

**Vektoren, Materialien, Licht und Schatten, Render-Passes, Shader, Renderer**

*Stand: 2026-09-16. Entwurf, nichts davon ist implementiert.*

Zeichen in diesem Dokument: **✔ entschieden** · **◇ Vorschlag** · **? offen**

---

## 0. Worum es geht

BlitzNext soll eine moderne 3D-Engine bekommen, in der mehr möglich ist als in
Blitz3D. Die alten 3D-, Surface- und Brush-Befehle bleiben gültig: Sie werden
auf die neue Engine abgebildet und, wo nötig, emuliert. Neue Möglichkeiten
kommen als neue Befehle und als eine Erweiterung der Sprache um Vektoren dazu.

Grundlage ist die Richtlinie in [ROADMAP3D.md](ROADMAP3D.md), Abschnitt
„Richtlinie: Was exakt stimmen muss und was besser werden darf“:

- **✔** Was ein Programm beobachten oder voraussetzen kann, stimmt exakt mit
  Blitz3D überein (Geometrie, Sichtbarkeit, Kameras, Texturinhalte,
  Rückgabewerte).
- **✔** Die Schattierung ist frei. Es gibt einen modernen Weg und keinen
  kompatiblen Beleuchtungsmodus.
- **✔** Alte Szenen dürfen anders hell wirken; das korrigiert man im Programm.

Dieses Dokument beschreibt, *wie* die moderne Engine aussehen soll. Es löst
einen früheren, nicht veröffentlichten Plan mit drei Darstellungsmodi ab — was
daraus übernommen wurde, steht in
[Abschnitt 8](#8-was-aus-dem-früheren-drei-modi-plan-übernommen-wurde).

---

## 1. Grundregeln für Spracherweiterungen

Die Vektoren sind die erste echte Erweiterung der Sprache. Die Regeln, die hier
festgelegt werden, gelten für alles, was danach kommt.

| Nr. | Regel | Stand |
|-----|-------|-------|
| E1 | Erweiterungen sind **neue Namen**. Die Grammatik von Blitz3D wird nicht verändert; ein gültiges Blitz3D-Programm bleibt syntaktisch gültig. | ✔ (seit 2026-09-05) |
| E2 | **Reservierte Namen einer Erweiterung gewinnen.** Verwendet ein altes Programm einen solchen Namen selbst, wird es mit einer erklärenden Meldung abgelehnt und muss umbenannt werden. | ✔ für `Vec2`/`Vec3`/`Vec4`; **?** ob das für alle neuen Befehle gilt |
| E3 | Neue Namen werden vorher gegen die Befehlsliste von Blitz3D und gegen die Beispielprogramme der Installation geprüft. Häufig benutzte Wörter bekommen ein Präfix. | ◇ |
| E4 | Kleine Datentypen (Vektoren, Matrizen) sind **Werte**, keine Objekte: Zuweisen kopiert, kein `New`/`Delete`. Ressourcen (Shader, Render-Targets) sind wie in Blitz3D **Handles** (Ganzzahlen) mit `Create…`/`Load…`/`Free…`. | ◇ |

**Zu E2 und E3, gemessen am 2026-09-16** in den 729 `.bb`-Dateien der
Blitz3D-Installation: `Vec2`, `Vec3`, `Vec4`, `Mat3`, `Mat4` und alle unten
vorgeschlagenen Pipeline-Befehle kommen **nirgends** vor. `Dot` steht in 3
Dateien, `Cross` in 1, `Length` in 4 (davon eine eigene Definition). Deshalb
tragen die Vektorbefehle das Präfix `Vec`.

---

## 2. Vektoren

### 2.1 Festlegungen

| Punkt | Festlegung | Stand |
|-------|------------|-------|
| Namen | `Vec2`, `Vec3`, `Vec4` | ✔ |
| Zahlentyp | immer Kommazahl (32 Bit, wie `#`) | ✔ |
| Tags | `Vec3#` ist erlaubt und bedeutet dasselbe; `Vec3%` und `Vec3$` sind Fehler: `'Vec3' always holds floats - remove the '%'` | ✔ |
| Verhalten | Wert, kein Objekt (E4) | ◇ |
| Reservierte Namen | `Vec2`, `Vec3`, `Vec4` sind reserviert, auch für Variablen, Funktionen und Types; das Programm verliert (E2) | ✔ |
| Komponenten | `v\x`, `v\y`, `v\z`, `v\w` | ◇ |
| Ganzzahlvektoren | nicht vorgesehen. Falls später nötig, kommt `Vec3%` als neue Schreibweise dazu, ohne bestehende Programme zu brechen | ✔ |

**Warum Werte:** Ein Blitz-`Type` ist ein Objekt, die Variable zeigt nur darauf.
Nach `b = a` würde eine Änderung an `b` auch `a` ändern. Bei Vektoren erwartet
das niemand, und auf der GPU gibt es solche Verweise nicht.

### 2.2 Wie es sich liest

```blitzbasic
Local pos.Vec3 = Vec3(0, 1, 5)
Local vel.Vec3 = Vec3(0, 0, -0.1)

pos = pos + vel * 2.0                      ; komponentenweise
dir.Vec3 = VecNormalize(target - pos)
If VecDot(dir, forward) > 0.9 Then Print "vorn"

Print pos\x + "," + pos\y + "," + pos\z

Dim path.Vec3(100)                         ; Arrays wie gewohnt
Global gravity.Vec3 = Vec3(0, -9.81, 0)

Function Midpoint.Vec3(a.Vec3, b.Vec3)
  Return (a + b) * 0.5
End Function
```

### 2.3 Rechnen

| Ausdruck | Bedeutung | Stand |
|----------|-----------|-------|
| `a + b`, `a - b` | komponentenweise | ◇ |
| `a * b`, `a / b` | komponentenweise, wie GLSL | ◇ |
| `a * 2.0`, `2.0 * a`, `a / 2.0` | jede Komponente | ◇ |
| `-a` | jede Komponente negiert | ◇ |
| `a = b`, `a <> b` | alle Komponenten gleich / nicht alle gleich | ? |
| `a < b` usw. | nicht erlaubt | ◇ |
| `Vec3` mit `Vec2` mischen | Fehler, keine stille Anpassung | ◇ |
| Komponente in Ganzzahl (`n% = v\x`) | wird gerundet, wie überall in Blitz | ✔ |
| Vektor in String (`Print v`, `"" + v`) | ? Format, z. B. `"1.0,2.0,3.0"` | ? |

### 2.4 Befehle

◇ Vorschlag, alle mit Präfix `Vec`:

| Befehl | Ergebnis |
|--------|----------|
| `Vec2(x,y)`, `Vec3(x,y,z)`, `Vec4(x,y,z,w)` | neuer Vektor |
| `Vec3(v2, z)`, `Vec4(v3, w)` | ? erweitern aus kleinerem Vektor |
| `VecDot(a, b)` | Skalarprodukt (`#`) |
| `VecCross(a, b)` | Kreuzprodukt, nur `Vec3` |
| `VecLength(v)`, `VecDistance(a, b)` | Länge, Abstand (`#`) |
| `VecNormalize(v)` | Länge 1 |
| `VecLerp(a, b, t)` | lineare Überblendung |
| `VecMin(a,b)`, `VecMax(a,b)`, `VecClamp(v,lo,hi)` | komponentenweise |

Nicht verwechseln: Blitz3D kennt `VectorYaw` und `VectorPitch` mit einzelnen
Koordinaten. Die gehören zu den fehlenden Blitz3D-Befehlen und kommen mit ihrer
alten Signatur.

### 2.5 Matrizen

? Nach demselben Schema `Mat3`, `Mat4`, immer Kommazahl, Wert.
Gebraucht werden sie vor allem in Shadern und für eigene 3D-Mathematik
(`mat * vec`). Zu klären: Zugriff auf Elemente, Befehle zum Aufbau
(`MatIdentity`, `MatRotate`…), Anschluss an `GetMatElement` aus Blitz3D.

### 2.6 Anschluss an die alten 3D-Befehle

Blitz kennt kein Überladen, `PositionEntity ent, pos` kann es also nicht geben.
◇ Neue Befehle mit eigenem Namen, z. B. `EntityPosition(ent)` → `Vec3` und
`PositionEntityVec ent, pos`. Nicht dringend: Die alten Befehle mit `x, y, z`
funktionieren weiter.

### 2.7 Umsetzung

- **Compiler:** neuer Werttyp im Analyzer (heute: int, float, string, Objekt),
  Operatoren, `\x` auf Werten statt Zeigern, reservierte Namen mit Meldung.
- **C++:** kleine Struktur mit Rechenoperatoren.
- **GLSL:** entspricht 1:1 `vec2`/`vec3`/`vec4` bzw. `mat3`/`mat4`.

---

## 3. Materialien

### 3.1 Ein Materialmodell für alles

✔ Alle Oberflächen laufen durch **ein** modernes Materialmodell
(Metallic-Roughness-PBR, Beleuchtung pro Pixel). Die alten Befehle setzen dessen
Werte; es gibt keinen getrennten Legacy-Shaderpfad.

◇ Übersetzung der alten Parameter (die Rollen stehen verbindlich in der
Richtlinie, die Formeln werden beim Bau festgelegt):

| Blitz3D | Modernes Material |
|---------|-------------------|
| `EntityColor`, `BrushColor` | Grundfarbe (Albedo) |
| `EntityAlpha`, `BrushAlpha` | Deckkraft |
| `EntityShininess`, `BrushShininess` | Roughness, umgekehrt: 0 → rau, 1 → glatt. **?** Kurve |
| — | Metallic = 0 |
| `EntityFX 1` | unbeleuchtet (Farbe direkt) |
| `EntityFX 2` | Vertexfarbe ersetzt die Grundfarbe |
| `EntityFX 4` | flach schattiert (Normale je Dreieck) |
| `EntityFX 8` | kein Nebel |
| `EntityFX 16` | beidseitig |
| `EntityFX 32` | Alpha-Blending erzwingen |
| `EntityBlend` 1/2/3 | Alpha / Multiplizieren / Addieren |
| `TextureBlend` 0–5, mehrere Texturlagen | emuliert: die Lagen werden vor der Beleuchtung zur Grundfarbe verrechnet |
| Texturflag 64, 128 | Umgebungsabbildung (Kugel, Würfel) als Reflexion |
| Lightmap in zweiter UV-Lage | **?** als vorberechnete Beleuchtung behandeln oder nur multiplizieren |

### 3.2 Neue Materialbefehle

◇ Übernommen aus dem früheren Drei-Modi-Plan, weil sie ins Brush-Konzept
passen:

```blitzbasic
material = CreateBrush()
BrushColor material, 180, 185, 190
BrushMetallic material, 1.0
BrushRoughness material, 0.25
BrushNormalMap material, LoadTexture("robot_n.png")
PaintEntity robot, material
```

`BrushMetallic`, `BrushRoughness`, `BrushNormalMap`, `BrushMetallicMap`,
`BrushRoughnessMap`, `BrushOcclusionMap`, `BrushEmissive`, `BrushEmissiveMap`.

**Nicht übernommen:** `BrushPBR` als Umschalter — es gibt nur ein Modell, jeder
Brush ist PBR.

Ebenfalls übernommen: Farbtexturen werden als sRGB gelesen, Roughness,
Metallic, Normal und Occlusion als lineare Daten; Tangenten werden für
Normalmapping erzeugt; Normalen werden auch bei ungleichmäßiger Skalierung
richtig transformiert.

---

## 4. Licht und Schatten

### 4.1 Licht

✔ Pro Pixel. Die Lichtbefehle behalten ihre Rollen (Richtlinie): Typ 1/2/3,
`LightColor` mit negativen Werten zum Abdunkeln, `LightRange` als Reichweite,
außerhalb nichts, `LightConeAngles`, `AmbientLight`.

◇ HDR-Rendering mit Tone-Mapping am Ende. **?** Belichtung als Befehl.

? Anzahl der Lichter: Blitz3D garantiert 8. Die neue Engine sollte mindestens 8
können; mehr hängt an der Renderer-Entscheidung (Abschnitt 7).

### 4.2 Schatten

✔ Schatten sind **standardmäßig an**.

◇ Abschalten in drei Ebenen, jeweils mit neuen Namen (E1):

| Ebene | Vorschlag | Wirkung |
|-------|-----------|---------|
| global | `ShadowMode 0` / `1` | alle Schatten aus / an |
| pro Licht | `LightShadows light, 0` | dieses Licht wirft keine Schatten |
| pro Entity | `EntityShadows ent, cast, receive` | wirft / empfängt |

Damit sieht ein altes Spiel mit **einer Zeile** wieder aus wie früher.

◇ Reihenfolge (aus dem früheren Drei-Modi-Plan): zuerst Richtungslicht, dann
Spotlicht, zuletzt Punktlicht (braucht sechs Richtungen und ist teuer).

**Bekannte Folge, offen zu kommunizieren:** Viele Blitz3D-Spiele haben Schatten
schon eingebacken (Lightmaps, Vertexfarben, Blob-Schatten, negative Lichter).
Mit echten Schatten wird es dort doppelt dunkel. Nach der Richtlinie ist das
zulässig; `ShadowMode 0` hilft.

? Wie reagieren Schatten auf `EntityAlpha`, maskierte Texturen (Flag 4) und
`EntityFX 1`?

---

## 5. Render-Passes und Bildeffekte

### 5.1 Das Problem

Ein Effekt wie Bloom braucht mehrere Durchgänge über das fertige Bild: helle
Anteile herausziehen, in X-Richtung weichzeichnen, in Y-Richtung weichzeichnen,
über das Bild legen. Blitz3D hat dafür nichts:

| Baustein | Blitz3D |
|----------|---------|
| in eine Zieltextur auf der GPU rendern | nur über `CameraViewport` + `CopyRect` in einen `TextureBuffer`, über die CPU, 8 Bit |
| Werte über 1.0 (HDR) | nein |
| ein Shader über das ganze Bild | nein |
| Passes verketten | nein |
| Einhängepunkt vor dem 2D-HUD | nein |

### 5.2 Das Modell

◇ **Explizite Befehle in der eigenen Hauptschleife.** Blitz-Programmierer
schreiben `UpdateWorld`, `RenderWorld`, 2D, `Flip` ohnehin selbst; Passes stehen
genau dort. Die Reihenfolge ist sichtbar, das HUD nach den Passes bleibt
unberührt.

```blitzbasic
scene  = CreateRenderTarget(640, 480, 1)        ; 1 = HDR
bright = CreateRenderTarget(320, 240, 1)
blurX  = CreateRenderTarget(320, 240, 1)
blurY  = CreateRenderTarget(320, 240, 1)
CameraRenderTarget cam, scene

sh_bright = LoadShader("bright.glsl")
sh_blur   = LoadShader("blur.glsl")
sh_comp   = LoadShader("bloom_composite.glsl")

While Not KeyHit(1)
  UpdateWorld
  RenderWorld                                   ; rendert in "scene"

  RenderPass sh_bright, bright, scene
  ShaderVec sh_blur, "direction", Vec2(1, 0)
  RenderPass sh_blur, blurX, bright
  ShaderVec sh_blur, "direction", Vec2(0, 1)
  RenderPass sh_blur, blurY, blurX
  RenderPass sh_comp, BackBuffer(), scene, blurY

  Text 0, 0, "HUD"                               ; nach den Passes
  Flip
Wend
```

◇ **Organisation mit Types ist ein Muster, keine Sprachmechanik.** Wer viele
Passes hat, legt sie in einen eigenen Type und arbeitet sie mit `For Each` ab.
Dafür braucht es keine Erweiterung:

```blitzbasic
Type Pass
  Field shader, target, source
End Type

For p.Pass = Each Pass
  RenderPass p\shader, p\target, p\source
Next
```

◇ **Fertige Effekte als einfache Befehle** für alle, die keine Shader schreiben
wollen, z. B. `CameraBloom cam, threshold#, strength#`. Sie laufen innerhalb von
`RenderWorld` und benutzen intern dieselben Passes.

### 5.3 Befehle

◇ Vorschlag:

| Befehl | Zweck |
|--------|-------|
| `CreateRenderTarget(w, h [,flags])`, `FreeRenderTarget rt` | Zieltextur auf der GPU; Flags: HDR, Tiefe |
| `CameraRenderTarget cam, rt` | Kamera rendert in das Target statt in den Backbuffer; `0` = zurück |
| `RenderTargetTexture(rt)` | Target als Textur für `EntityTexture` (Spiegel, Monitore im Spiel) |
| `LoadShader(file$)`, `FreeShader sh` | Shader laden (`.glsl` oder `.bbs`, Abschnitt 6) |
| `ShaderFloat sh, name$, f#` / `ShaderVec sh, name$, v` / `ShaderTexture sh, name$, tex` / `ShaderMat sh, name$, m` | Parameter setzen |
| `RenderPass sh, target, source1 [,source2 …]` | Shader über das ganze Bild |

### 5.4 Anschluss an Blitz3D

- ? `TextureBuffer`, `CopyRect` und `ReadPixel` (BUG-127, BUG-118) sollten auf
  dieselben GPU-Targets aufsetzen, damit der alte Weg „Kamera → Textur“ weiter
  funktioniert und nicht langsamer ist als der neue.
- ? Rechnet `RenderWorld` immer in HDR mit Tone-Mapping, auch ohne eigene
  Passes? Das ändert das Aussehen alter Szenen (zulässig nach der Richtlinie).
- ? Mehrere Kameras mit Viewports: jede mit eigenem Target, oder Passes je
  Viewport?

---

## 6. Shader

### 6.1 Zwei Formate

| Format | Wann | Stand |
|--------|------|-------|
| **GLSL** (`.glsl`) | zuerst, damit das Pass-Modell schnell ausprobiert werden kann; bleibt danach als Ausweg | ◇ |
| **Blitz-Shader-Dialekt** (`.bbs`) | eigenes, späteres Projekt; blitzcc übersetzt nach GLSL | ✔ Richtung, ◇ Zeitpunkt |

Der Gewinn des Dialekts: eine Sprache für alles, und Fehlermeldungen in
Blitz-Begriffen statt GLSL-Meldungen. Der Compiler hat Lexer, Parser und
semantische Prüfung schon; ein zweites Ausgabeziel GLSL liegt nahe.

### 6.2 Der Dialekt

Shader stehen in **eigenen Dateien**. Neue Schlüsselwörter des Dialekts gelten
nur dort; kein `.bb`-Programm verliert dadurch einen Bezeichner.

**Erlaubt:** `%`, `#`, `Vec2`–`Vec4`, `Mat3`/`Mat4`, `If`/`Select`, `For` mit
fester Obergrenze, `While`, Funktionen ohne Rekursion, `Const`, die Mathematik
(`Sin`, `Sqr`, `Abs`…), Vektorbefehle, Texturzugriff.

**Nicht erlaubt**, weil es auf der GPU nicht existiert: `$`, Types als Objekte,
`New`/`Delete`, `Goto`/`Gosub`, `Data`/`Read`, Dateibefehle, 2D-Befehle,
Rekursion, `Dim` mit veränderlicher Größe.

◇ Skizze:

```blitzbasic
; blur.bbs
Uniform source.Texture
Uniform direction.Vec2

Function Pixel.Vec4(uv.Vec2)
  sum.Vec4 = Vec4(0, 0, 0, 0)
  For i = -4 To 4
    sum = sum + TextureSample(source, uv + direction * TexelSize(source) * i) * BlurWeight(i)
  Next
  Return sum
End Function
```

? Offen sind die Grundbausteine: wie Eingaben (`Uniform`, Texturen,
Vertexdaten) und Ausgaben (Farbe, mehrere Targets) heißen, ob es neben
Bild-Shadern auch Material-Shader (für eine Oberfläche) gibt, und wie
Material-Shader mit dem Materialmodell aus Abschnitt 3 zusammenspielen.

---

## 7. Renderer-Architektur

### 7.1 Heute

Forward-Renderer, ein Draw-Call je Mesh. Richtungslicht pro Pixel, Punkt- und
Spotlicht sowie Glanz je Vertex (Übergangsstand aus BUG-66/BUG-91).

### 7.2 Gliederung

◇ Übernommen aus dem früheren Drei-Modi-Plan — der Renderer wird vor jeder
optischen Erweiterung in klare Schritte zerlegt:

1. Szene und Kameras erfassen.
2. Effektive Materialien bestimmen (Entity, Brush, Surface, FX, Texturlagen).
3. Zeichenaufträge mit Reihenfolge und Zuständen bilden (`EntityOrder`,
   Transparenz).
4. Schatten-Durchgänge.
5. Undurchsichtige, maskierte und transparente Geometrie zeichnen.
6. Passes und Tone-Mapping.
7. Mit der 2D-Ausgabe zusammenführen.

### 7.3 Forward+ oder Deferred

? Offen. Die Entscheidung fällt, wenn feststeht, was Shader-Befehle können
sollen. Bloom und andere Bild-Effekte funktionieren mit beiden.

| | Forward+ (Lichter in Kacheln/Clustern) | Deferred |
|---|---|---|
| viele Lichter | gut | sehr gut |
| Transparenz (`EntityAlpha`, Add/Multiply, Sprites, Partikel — in Blitz allgegenwärtig) | direkt | braucht zusätzlich einen Forward-Durchgang |
| Kantenglättung (MSAA) | direkt | schwierig |
| Vielfalt alter Materialien (`TextureBlend`, FX-Kombinationen) | pro Material ein Shader | muss in einen festen G-Buffer passen |
| mehrere Kameras, Render-to-Texture | günstig | je Kamera ein G-Buffer |
| SSAO, SSR, eigene Beleuchtungsmodelle | mit Tiefen-/Normalen-Vorpass möglich | direkt |
| eigene **Material**-Shader | einfach | schwieriger |

◇ Empfehlung: den Renderer hinter der Gliederung aus 7.2 halten, zuerst
Forward+ mit Tiefen-/Normalen-Vorpass; Deferred bleibt möglich, falls eigene
Beleuchtungsmodelle ein Ziel werden. `ROADMAP3D.md` beschreibt den
Deferred-Erweiterungspfad bereits.

---

## 8. Was aus dem früheren Drei-Modi-Plan übernommen wurde

Ein lokaler, nie veröffentlichter Plan vom 2026-09-15 ging von drei Modi aus
(Legacy als Standard, Enhanced, Modern). Die Entscheidungen vom 2026-09-16 ändern
das; der Plan wurde danach gelöscht. Was davon weiter gilt:

| Früherer Plan | Heute |
|---------------------|-------|
| Drei Modi Legacy / Enhanced / Modern, Legacy Standard | **ersetzt:** ein moderner Weg |
| `RenderMode`-Befehl | **entfällt** |
| Legacy-Referenzszenen gegen Blitz3D | **eingeschränkt:** nur für Beobachtbares (Geometrie, Sichtbarkeit, Texturinhalte, Kameras) |
| Enhanced mit MSAA und anisotroper Filterung | **aufgehen lassen** in den einen Weg |
| PBR über Brushes, Befehlsnamen | **übernommen**, ohne `BrushPBR` |
| Getrennter Legacy-Shaderpfad | **ersetzt:** alte Parameter werden ins Materialmodell übersetzt |
| sRGB/linear, Tangenten, Normalen bei Skalierung | **übernommen** |
| Forward zuerst | **übernommen**, Forward+ als Ziel offen |
| Schatten zuerst Richtungslicht, cast/receive je Entity | **übernommen**, dazu: Schatten standardmäßig an |
| Umgebungsbeleuchtung (IBL) | **übernommen** |
| Bloom, SSAO als optionale Effekte | **übernommen**, eingeordnet in Abschnitt 5 |
| Gliederung des Renderers in Schritte | **übernommen** (7.2) |

---

## 9. Reihenfolge

◇ Vorschlag:

1. **Beobachtbare Fehler beheben**, die jede Engine braucht: BUG-126
   (Umlaufrichtung), BUG-127 (`TextureBuffer`), BUG-128 (`CopyEntity`), BUG-129
   (Viewports), BUG-131 (Farbe nach `Graphics3D`), BUG-69/93/133 (Primitive).
2. **Renderer gliedern** (7.2) ohne sichtbare Änderung.
3. **Vektoren** in der Sprache (Abschnitt 2) — unabhängig vom Renderer, sofort
   nützlich.
4. **Materialmodell und Licht pro Pixel** (Abschnitte 3, 4.1), HDR und
   Tone-Mapping; dabei WEAK-25 (Beleuchtungstests) umstellen.
5. **Schatten** (4.2).
6. **Render-Targets und Passes mit GLSL** (Abschnitt 5).
7. **Fertige Effekte** (Bloom, SSAO, Farbkorrektur).
8. **Blitz-Shader-Dialekt** (Abschnitt 6).
9. **Renderer-Entscheidung** Forward+ / Deferred (7.3), spätestens vor 8.

---

## 10. Offene Entscheidungen

| Nr. | Frage | Abschnitt |
|-----|-------|-----------|
| O1 | Gilt „das Programm verliert“ (E2) für alle neuen Befehle oder nur für `Vec`? | 1 |
| O2 | Vergleich `a = b` bei Vektoren, Format bei `Print v` | 2.3 |
| O3 | Matrizen: Schreibweise, Elementzugriff, Befehle | 2.5 |
| O4 | Kurve Shininess → Roughness | 3.1 |
| O5 | Lightmaps: vorberechnete Beleuchtung oder nur Multiplikation | 3.1 |
| O6 | Befehlsnamen für Schatten, Verhalten bei Alpha und Maske | 4.2 |
| O7 | Belichtung als Befehl; HDR/Tone-Mapping immer oder nur mit Passes | 4.1, 5.4 |
| O8 | `TextureBuffer`/`CopyRect` auf GPU-Targets | 5.4 |
| O9 | Passes und mehrere Kameras/Viewports | 5.4 |
| O10 | Ein- und Ausgaben des Shader-Dialekts, Material-Shader | 6.2 |
| O11 | Forward+ oder Deferred | 7.3 |
