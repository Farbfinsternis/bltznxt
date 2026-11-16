# BlitzNext Engine-Entwurf

**Vektoren, Materialien, Licht und Schatten, Render-Passes, Shader, Renderer, Oberflächen**

*Stand: 2026-09-17 (Abgleich mit der Bugliste eingearbeitet), ergänzt am
2026-11-16 um den Stand im Code, den gemeinsamen Unterbau (0.1) und die
Oberflächen (Abschnitt 11); am selben Tag O13 bis O16 entschieden. Entwurf,
nichts davon ist implementiert.*

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

### 0.1 Drei Vorhaben, ein Unterbau (2026-11-16)

Nach der Parität mit Blitz3D soll BLTZNXT an drei Stellen darüber
hinauswachsen: PBR-Rendering (Abschnitte 3 und 4), Shader in Blitz-Syntax
(Abschnitt 6) und Oberflächen, wie BlitzPlus sie hatte (Abschnitt 11).

Alle drei brauchen dasselbe, und es fehlt heute: **Rendern in Zieltexturen.**

| Vorhaben | Wozu die Zieltextur |
|----------|---------------------|
| PBR | das HDR-Bild, das am Ende von `RenderWorld` auf den Bildschirm umgerechnet wird |
| Shader | Quelle und Ziel jedes `RenderPass` (Abschnitt 5) |
| Oberflächen | ein Canvas, in das `RenderWorld` zeichnet (11.4) |

Stand im Code, geprüft am 2026-11-16:

- Die Framebuffer-Funktionen werden in `bb_gl_ctx.h` geladen, aber nirgends
  aufgerufen. Es gibt kein sRGB- und kein HDR-Format.
- `bb_shader.h` enthält drei feste GLSL-3.30-Shader (UNLIT, TEXTURED, LIT);
  LIT rechnet Blinn-Phong mit bis zu acht Lichtern.
- `Vec2`, `Vec3` und `Vec4` gibt es im Compiler nicht.
- Die Laufzeit kennt genau ein Fenster und einen 2D-Renderer, beide als
  globale Variablen in `bb_sdl.h`.

◇ Daraus folgt: Die Zieltexturen werden zuerst gebaut, vor dem Materialmodell
(siehe die Ergänzung in [Abschnitt 9](#9-reihenfolge)).

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

**Zu O1, gemessen am 2026-11-16** in denselben Dateien:
`Games/bnb/includes/cfg.bb` definiert selbst `Function CreateButton` — ein
Befehlsname aus BlitzPlus (Abschnitt 11). Mit E2 würde dieses Programm
abgelehnt, sobald es den Befehl gibt. Als ganzes Wort, Kommentare
eingeschlossen, kommen `Shader`, `Gadget`, `Canvas` und `Event` in keiner Datei
vor, `Uniform`, `Material` und `Window` in je einer.

◇ Vorschlag für O1: E2 gilt für **Typnamen und Schlüsselwörter** (`Vec3`),
nicht für Befehle. Definiert ein Programm eine Funktion, die wie ein neuer
Befehl heißt, gewinnt die Funktion des Programms; der neue Befehl ist in diesem
Programm dann nicht erreichbar. Für die Befehle von Blitz3D ändert sich nichts.

---

## 2. Vektoren

### 2.1 Festlegungen

| Punkt | Festlegung | Stand |
|-------|------------|-------|
| Namen | `Vec2`, `Vec3`, `Vec4` | ✔ |
| Zahlentyp | immer Kommazahl (32 Bit, wie `#`) | ✔ |
| Tags | `Vec3#` ist erlaubt und bedeutet dasselbe; `Vec3%` und `Vec3$` sind Fehler: `'Vec3' always holds floats - remove the '%'` | ✔ |
| Verhalten | Wert, kein Objekt (E4) | ◇ |
| Reservierte Namen | `Vec2`, `Vec3`, `Vec4` sind reserviert, auch für Variablen, Funktionen und Types; das Programm verliert (E2). Wie jedes reservierte Wort ohne Beachtung der Großschreibung (`vec3` ist ebenfalls reserviert) | ✔ |
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
| Vektor in String (`Print v`, `"" + v`) | ◇ jede Komponente wie `Str(float)` des Originals, durch Komma getrennt: `"1.0,2.0,3.0"`. Setzt BUG-68 voraus | ? |

### 2.3a Vektoren und die Implizit-Regeln von Blitz

? Offen (O12). Blitz legt eine Variable ohne Tag als Ganzzahl an und wandelt
Konstanten, `Data` und Vorgabewerte nach eigenen Regeln um. Für Vektoren ist
noch nichts davon festgelegt:

| Fall | Frage | Bezug |
|------|-------|-------|
| `v = Vec3(1,2,3)` ohne Tag | ◇ `Illegal type conversion` wie bei Objekten, denn eine implizite Variable ist int | BUG-80 |
| `Const up.Vec3 = Vec3(0,1,0)` | erlaubt? | BUG-99 |
| `Data` mit Vektoren | erlaubt? | BUG-107 |
| `Function F(v.Vec3 = Vec3(0,0,0))` | Vorgabewerte erlaubt? | BUG-49 |
| `Handle v`, `Object.Vec3(h)` | ◇ Meldung, Vektoren sind Werte (E4) | BUG-101 |

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
- **Vorher zu beheben**, weil der neue Werttyp genau auf diesen Pfaden
  aufsetzt: BUG-95 (Float→Int schneidet ab, die Regel „gerundet wie überall“
  aus 2.3 gilt heute nicht überall), BUG-97 (Float-Literale als double, Vektoren
  sind 32 Bit), BUG-100 (Local/Global in der Symboltabelle), BUG-80 (ungetaggte
  Zuweisung übernimmt den Typ der rechten Seite), BUG-68 (`Str(float)`, für
  das Format aus 2.3).
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
| Texturflag 64, 128 | Umgebungsabbildung (Kugel, Würfel) als Reflexion; Flag 64 fehlt heute ganz (BUG-130, für 0.7.0 vorgezogen und im heutigen Shader behoben; Schritt 5 übernimmt es ins Materialmodell) |
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

### 3.3 Was zum Paket gehört (2026-11-16)

Der Shader ist der kleinste Teil. Ohne Umgebungslicht sind Metalle schwarz, und
ohne Schatten und Tone-Mapping sieht PBR mit acht Punktlichtern schlechter aus
als die heutige Beleuchtung. Die Bausteine gehören deshalb zusammen:

| Baustein | Stand im Code |
|----------|---------------|
| Lineares Rechnen, Farbtexturen als sRGB | fehlt |
| HDR-Ziel und Tone-Mapping | fehlt (0.1) |
| Materialwerte am Brush (3.2) | fehlen; der Brush kennt Farbe, Deckkraft, Glanz, Blend, FX und Texturlagen |
| Tangenten, Normal Maps | fehlen; das Vertexformat hat keine Tangenten |
| Umgebungslicht (IBL) | fehlt |
| Schatten, zuerst Richtungslicht (4.2) | fehlen |
| Materialien aus glTF | der Lader liest Grundfarbe und Farbtextur und lässt Metallic, Roughness und Normal Maps liegen |

◇ Umgebungslicht ohne Umgebungsbild: `AmbientLight` gilt als einfarbige
Umgebung. Ein Programm, das nichts weiter tut, bekommt damit gleichmäßiges
Licht und eine einfarbige Spiegelung. Ein Umgebungsbild kommt über einen neuen
Befehl dazu; `stb_image.h` in `src/thirdparty` liest das `.hdr`-Format bereits.

### 3.4 Ein Rechenweg, zurückhaltende Vorgaben (2026-11-16)

✔ Entschieden am 2026-11-16 (O13). [VISION.md](VISION.md) vom 2026-09-26
widersprach 3.1: Dort stand die Empfehlung, den modernen Weg **pro Programm**
einzuschalten, weil lineare Farben und Tone-Mapping die Helligkeit alter Szenen
ändern. 3.1 legt dagegen ein Modell ohne Umschalter fest.

Die Entscheidung verbindet beides: Es bleibt bei **einem** Rechenweg, es gibt
keinen zweiten Renderer. Was das Bild als Ganzes prägt, wird eine Einstellung
der Szene mit einer Vorgabe:

| Einstellung | Vorgabe | Auf Wunsch |
|-------------|---------|------------|
| Tone-Mapping | keine Kurve, nur Begrenzung auf 0..1 | Kurve und Belichtung (O7) |
| Umgebungslicht | einfarbig aus `AmbientLight` | Umgebungsbild |
| Schatten | an (4.2) | `ShadowMode 0` |

Damit bleibt `EntityFX 1` farbgenau — die Tests prüfen dort exakte Pixelwerte —,
und ein neues Programm schaltet den Rest mit einem Befehl ein.

**Vorbehalt:** Alpha-, Add- und Multiply-Blending im linearen Farbraum
verändert Partikel, Sprites und Überblendungen alter Programme sichtbar. Das
lässt sich nicht am Schreibtisch beurteilen. Vor dem Bau des Materialmodells
(Schritt 5) wird es an den Beispielen der Blitz3D-Installation gemessen. Fällt
die Abweichung zu groß aus, greift der Schalter pro Programm aus VISION.md.

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

- ◇ `TextureBuffer`, `CopyRect` und `ReadPixel` (BUG-127, BUG-118) — die Frage
  wird geteilt:
  - **Bedeutung jetzt** (Schritt 1): was `ReadPixel`, `WritePixel`,
    `LockBuffer` und `CopyRect` auf einer Textur liefern und wann eine Änderung
    im Bild sichtbar wird, wird am Original gemessen, umgesetzt und mit Tests
    festgehalten.
  - **Speicherung später** (Schritt 7): ob der Puffer als CPU-Kopie mit
    Hochladen oder als GPU-Target lebt. Ziel bleibt, dass der alte Weg
    „Kamera → Textur“ nicht langsamer ist als der neue. Die Tests aus Schritt 1
    gelten weiter.
- ? Rechnet `RenderWorld` immer in HDR mit Tone-Mapping, auch ohne eigene
  Passes? Das ändert das Aussehen alter Szenen (zulässig nach der Richtlinie).
  ◇ Vorschlag in 3.4: immer in HDR rechnen, die Tone-Mapping-Kurve aber nur
  auf Wunsch.
- ? Mehrere Kameras mit Viewports: jede mit eigenem Target, oder Passes je
  Viewport? Das Beobachtbare (BUG-129: Bild und `CameraClsColor` in jedem
  Viewport) hängt nicht daran und wird vorher behoben; die Kameraschleife soll
  dabei ein eigenes Target je Kamera zulassen.

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

Feste Obergrenzen und feste Größen brauchen eine **typisierte
Konstantenauswertung** im Analyzer. Dieselbe Auswertung fehlt heute schon für
BUG-75 (`Division by zero`), BUG-78 (Index außerhalb fester Arrays), BUG-99
(`Const`) und BUG-107 (`Data`); sie wird deshalb in Schritt 3 gebaut, nicht erst
für den Dialekt.

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
Vorschläge dazu stehen in 6.4.

### 6.3 Der Dialekt als offizielle Shader-API (2026-11-16)

[VISION.md](VISION.md) warnt: GLSL als API legt BLTZNXT auf OpenGL fest,
solange es das Projekt gibt. Der Dialekt hebt das auf, weil `blitzcc`
erzeugt, was der Renderer gerade braucht — heute GLSL 3.30, später zum Beispiel
GLSL ES 3.00 für den Browser.

✔ Entschieden am 2026-11-16 (O14): Der Dialekt ist die **einzige offizielle**
Shader-API.
`.glsl` bleibt als Ausweg möglich, wird aber als nicht portabel gekennzeichnet
und nicht auf Dauer zugesichert. Die Reihenfolge aus 6.1 (GLSL zuerst, um das
Pass-Modell auszuprobieren) bleibt davon unberührt.

Was davon schon da ist: die Konstantenauswertung (`constfold.h`), die 6.2 für
feste Obergrenzen verlangt, und die Zeilenzuordnung über `Include` hinweg
(`sourcemap.h`), damit Fehlermeldungen auf die richtige Datei und Zeile zeigen.
Es fehlen die Vektoren (Abschnitt 2).

### 6.4 Zwei Stufen: Bild-Shader, dann Material-Shader (2026-11-16)

◇ **Bild-Shader zuerst.** Ein Shader über das ganze Bild hat nur Texturen und
Parameter als Eingabe und eine Farbe als Ausgabe (Skizze in 6.2). Er berührt
weder Licht noch Materialmodell.

◇ **Material-Shader als Funktion, die Materialwerte füllt.** Der Shader
bestimmt Grundfarbe, Normale, Roughness, Metallic, Leuchten und Deckkraft einer
Oberfläche; Licht, Schatten, Nebel und Knochen rechnet die Engine. So
überlebt ein Material-Shader jeden Umbau des Renderers (7.3), und er sieht
unter jedem Licht richtig aus. Skizze, Namen nicht festgelegt:

```blitzbasic
; lava.bbs
Uniform time#
Uniform noise.Texture

Function Surface(uv.Vec2)
  n.Vec4 = TextureSample(noise, uv + Vec2(0, time * 0.05))
  MaterialColor     Vec3(0.1, 0.02, 0.0)
  MaterialEmissive  Vec3(4.0, 1.2, 0.2) * n\x
  MaterialRoughness 0.9
End Function
```

Ein eigenes Beleuchtungsmodell (Toon-Shading u. ä.) ist damit nicht möglich. ?
Ob es dafür später einen zweiten Einhängepunkt gibt, bleibt offen (O10).

### 6.5 Eigene Datei oder Block im Programm (2026-11-16)

Gewünscht ist „Shader in BB-Code“. 6.2 sieht eigene `.bbs`-Dateien vor, damit
neue Schlüsselwörter keinem Programm einen Bezeichner nehmen. Ein Block im
`.bb`-Programm (`Shader … End Shader`) wäre nach der Messung in Abschnitt 1
ebenfalls möglich: `Shader` kommt in keiner der 729 Dateien vor.

◇ Vorschlag: mit eigenen Dateien beginnen; der Block kann später dazukommen,
ohne etwas zu brechen.

◇ Idee: Derselbe Shader-Code lässt sich auch nach C++ übersetzen und auf der
CPU ausführen. Das ergäbe Shader-Tests ohne Grafikkarte.

---

## 7. Renderer-Architektur

### 7.1 Heute

Forward-Renderer, ein Draw-Call je Mesh. Richtungslicht pro Pixel, Punkt- und
Spotlicht sowie Glanz je Vertex (Übergangsstand aus BUG-66/BUG-91).

Ergänzt am 2026-11-16: Die 2D-Befehle laufen im 3D-Modus über den
SDL-Renderer. Der hat einen eigenen OpenGL-2.1-Kontext und zeichnet in
denselben Bildpuffer des Fensters wie `RenderWorld` mit seinem 3.3-Kontext
(`bb_gl_ctx.h`, BUG-63). Für PBR stört das nicht: Das HDR-Bild wird am Ende von
`RenderWorld` in den Bildpuffer umgerechnet, die 2D-Befehle zeichnen danach
darüber wie bisher.

Einschätzung, nicht geprüft: Für den Browser und für 3D in einem Canvas (11.4)
ist die Konstruktion ein Hindernis, weil dort zwei Kontexte auf einer Fläche
nicht vorgesehen sind.

✔ Entschieden am 2026-11-16 (O16): Der Browser wird **offengehalten**, er ist
kein festes Ziel. Daraus folgt:

- Alles Neue (Zieltexturen, Materialmodell, Shader-Dialekt) wird so gebaut,
  dass es auch mit WebGL 2 ginge, also im gemeinsamen Teil von OpenGL 3.3 und
  GLSL ES 3.00.
- Der 2D-Weg über den SDL-Renderer bleibt, wie er ist. Umgebaut wird er erst,
  wenn der Browser wirklich ansteht oder 3D in einem Canvas ihn verlangt.

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

◇ Ergänzt am 2026-11-16: Für das Materialmodell (Schritt 5) reicht der heutige
einfache Forward-Renderer mit acht Lichtern. Forward+ lohnt erst, wenn deutlich
mehr Lichter ein Ziel werden. Material-Shader nach 6.4 hängen an keiner der
beiden Bauweisen.

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

◇ Vorschlag, am 2026-09-17 mit der Bugliste abgeglichen:

1. **Beobachtbare 3D-Fehler beheben**, die jede Engine braucht:
   1. BUG-126 (Umlaufrichtung) **zuerst**. Gemessen am 2026-09-17: Der
      Renderer schneidet bei **allen** Netzen die falsche Seite weg, auch bei
      geladenen Modellen. Die sehen von außen nur richtig aus, weil man bei
      einem geschlossenen Netz die Innenseite der Rückwand sieht. Die Primitive
      und `UpdateNormals` sind andersherum gebaut, um das auszugleichen. Die
      Umkehr kommt an eine Stelle; damit stimmen `TriangleVertex` beim Würfel
      (BUG-65) und die Normalen geladener Modelle wieder. Danach BUG-134
      (`UpdateNormals` mittelt über gleiche Positionen) und BUG-135
      (`.3ds`-Loader).
   2. BUG-69/93/133 (Kugel, Kegel, Zylinder) in der gemessenen
      Indexreihenfolge des Originals.
   3. BUG-128 (`CopyEntity`), BUG-129 (Viewports), BUG-131 (Farbe nach
      `Graphics3D`), BUG-120 (`ClearWorld`-Schalter).
   4. BUG-127 (`TextureBuffer`) und BUG-118 (`CopyRect`) mit der Bedeutung
      nach 5.4; die Speicherung folgt in Schritt 7.
   5. BUG-76 (Texturpfade aus Modellen), sobald entschieden ist, ob wir dem
      Original folgen.
2. **3D-Verhaltenstests** (WEAK-16: Generatoren, Matrizen, Kameraschleife,
   Pixelproben mit `EntityFX 1`), danach **Renderer gliedern** (7.2) ohne
   sichtbare Änderung. Ohne die Tests ist „ohne sichtbare Änderung“ nicht
   prüfbar.
3. **Sprachgrundlage für Vektoren:** BUG-95, BUG-97, BUG-100, BUG-80, BUG-68
   (siehe 2.7) und die typisierte Konstantenauswertung (BUG-75/78/99/107, siehe
   6.2); O2 und O12 entscheiden.
4. **Vektoren** in der Sprache (Abschnitt 2) — unabhängig vom Renderer, sofort
   nützlich.
5. **Materialmodell und Licht pro Pixel** (Abschnitte 3, 4.1), HDR und
   Tone-Mapping; dabei BUG-130 (Texturflag 64, für 0.7.0 vorgezogen) ins
   Materialmodell übernehmen und WEAK-25
   (Beleuchtungstests) umstellen. Die Übergangsstände aus BUG-66/BUG-91 enden
   hier.
6. **Schatten** (4.2).
7. **Render-Targets und Passes mit GLSL** (Abschnitt 5), dazu die Speicherung
   von `TextureBuffer`/`CopyRect` (O8).
8. **Fertige Effekte** (Bloom, SSAO, Farbkorrektur).
9. **Blitz-Shader-Dialekt** (Abschnitt 6).
10. **Renderer-Entscheidung** Forward+ / Deferred (7.3), spätestens vor 9.

**Ergänzung am 2026-11-16.** Die Nummern oben bleiben, weil der Code auf sie
verweist (`bb_canvas.h` nennt Schritt 7 und O8). ◇ Vorschläge:

- **Zieltexturen vor Schritt 5.** Das HDR-Bild des Materialmodells ist selbst
  eine Zieltextur (0.1). Der Teil „Render-Targets“ aus Schritt 7 wird deshalb
  vorgezogen, ohne sichtbare Änderung; `RenderPass`, die Shader-Befehle und die
  Speicherung von `TextureBuffer`/`CopyRect` bleiben in Schritt 7.
- **Schritt 5 als Paket.** Materialmodell, Tangenten, Umgebungslicht und das
  Lesen der glTF-Materialien gehören zusammen (3.3); Schritt 6 (Schatten)
  folgt unmittelbar.
- **Schritt 9 in zwei Stufen:** erst Bild-Shader, dann Material-Shader (6.4).
- **Oberflächen (Abschnitt 11) als eigener Strang.** Sie hängen an keinem der
  Schritte 1 bis 10; nur 3D in einem Canvas braucht die Zieltexturen.

Stand der Voraussetzungen: Schritt 4 (Vektoren) ist nicht begonnen. O13 und O16
sind seit dem 2026-11-16 entschieden; vor Schritt 5 steht noch die Messung zum
Blending im linearen Farbraum (3.4).

Vom Entwurf unabhängig und jederzeit möglich sind die übrigen offenen Einträge
der Bugliste (Include, Objektlebensdauer, Gosub, Dateiformat, Zufallszahlen,
2D-Kollision, Stringprüfungen u. a.).

---

## 10. Offene Entscheidungen

| Nr. | Frage | Abschnitt |
|-----|-------|-----------|
| O1 | Gilt „das Programm verliert“ (E2) für alle neuen Befehle oder nur für `Vec`? | 1 |
| O2 | Vergleich `a = b` bei Vektoren, Format bei `Print v` (◇ wie `Str(float)`, BUG-68) | 2.3 |
| O3 | Matrizen: Schreibweise, Elementzugriff, Befehle | 2.5 |
| O4 | Kurve Shininess → Roughness | 3.1 |
| O5 | Lightmaps: vorberechnete Beleuchtung oder nur Multiplikation | 3.1 |
| O6 | Befehlsnamen für Schatten, Verhalten bei Alpha und Maske | 4.2 |
| O7 | Belichtung als Befehl; HDR/Tone-Mapping immer oder nur mit Passes | 4.1, 5.4 |
| O8 | `TextureBuffer`/`CopyRect` auf GPU-Targets (◇ geteilt: Bedeutung in Schritt 1, Speicherung in Schritt 7) | 5.4 |
| O9 | Passes und mehrere Kameras/Viewports (BUG-129 unabhängig davon vorher) | 5.4 |
| O10 | Ein- und Ausgaben des Shader-Dialekts, Material-Shader | 6.2 |
| O11 | Forward+ oder Deferred | 7.3 |
| O12 | Vektoren und die Implizit-Regeln: ungetaggte Zuweisung, `Const`, `Data`, Vorgabewerte, `Handle` (BUG-80/99/107/49/101) | 2.3a |
| O13 | ✔ 2026-11-16: PBR-Weg — ein Rechenweg mit zurückhaltenden Vorgaben, kein Schalter. Vorbehalt: die Messung zum Blending im linearen Farbraum; fällt sie durch, greift der Schalter pro Programm aus VISION.md | 3.4 |
| O14 | ✔ 2026-11-16: Shader-API — der Dialekt ist die einzige offizielle API, GLSL bleibt als gekennzeichneter Ausweg | 6.3 |
| O15 | ✔ 2026-11-16: Oberflächen — selbst gezeichnet auf SDL3; Ziel ist die Fähigkeit mit den Namen und Signaturen von BlitzPlus, nicht dass alte BlitzPlus-Programme unverändert laufen | 11.1, 11.2 |
| O16 | ✔ 2026-11-16: Browser — offengehalten, kein festes Ziel. Neues wird WebGL-2-tauglich gebaut, der 2D-Weg bleibt vorerst | 7.1 |

Zu O1 gibt es seit dem 2026-11-16 eine Messung und einen Vorschlag
(Abschnitt 1), zu O7 und O10 je einen Vorschlag (3.4, 6.4). Diese drei sind
weiter offen.

---

## 11. Oberflächen (BlitzPlus-Fähigkeiten)

*Ergänzt am 2026-11-16. Bauweise und Anspruch sind entschieden (O15), alles
Übrige in diesem Abschnitt ist ◇ Vorschlag oder ? offen. Befehle, Signaturen und Ereignisnummern sind am selben Tag gegen die
veröffentlichten BlitzPlus-Quellen geprüft
(<https://github.com/blitz-research/blitzplus_msvc2017>, zlib/libpng-Lizenz);
die Fundstellen stehen in 11.3. Das Verhalten der einzelnen Befehle ist noch
nicht geprüft. Für alles, was BlitzPlus betrifft, gilt: in den Quellen
nachsehen, nicht aus dem Gedächtnis schreiben.*

### 11.1 Worum es geht

BlitzPlus war Blitz2D mit Fenstern, Bedienelementen (Gadgets) und einer
Ereignisschlange, aber ohne 3D. Blitz3D war das Umgekehrte. Ein Fenster mit
Menü, Knöpfen und einer Fläche, in der `RenderWorld` zeichnet, konnte in der
Blitz-Welt deshalb keines von beiden — genau das brauchen Editoren und
Werkzeuge.

✔ Entschieden am 2026-11-16 (O15): Ziel ist die **Fähigkeit**. Namen und
Signaturen folgen BlitzPlus; dass alte BlitzPlus-Programme unverändert laufen,
gehört nicht zum Anspruch. Das wäre ein eigenes Vorhaben mit eigenem Korpus,
denn BlitzPlus weicht auch außerhalb der Oberflächen von Blitz3D ab.

### 11.2 Bauweise

| | Native Bedienelemente | Selbst gezeichnet |
|---|---|---|
| Aussehen | wie das Betriebssystem | überall gleich |
| Plattformen | je Plattform eine eigene Umsetzung | eine Umsetzung auf SDL3 |
| Browser | nicht möglich | möglich |
| 3D in einem Canvas | Fenster im Fenster, je Plattform anders | eine Zieltextur wie jede andere (0.1) |
| Texteingabe, Bedienhilfen | bringt das System mit | muss selbst gebaut werden |

✔ Entschieden am 2026-11-16 (O15): selbst gezeichnet. Nativ bleibt, was SDL3
ohnehin liefert: Dateidialoge und Meldungsfenster.

BlitzPlus trennt selbst zwischen den Befehlen und ihrer Umsetzung: `gui/gui.h`
beschreibt mit `BBGuiDriver` eine Schnittstelle (ein `create…` je
Bedienelement, dazu Meldungen und Abfragen), und `win32gui/` setzt sie mit den
Bedienelementen von Windows um. Die Naht für eine selbst gezeichnete Umsetzung
liegt also an derselben Stelle wie im Original.

### 11.3 Umfang in Stufen

Quelle: die Befehlstabellen in `blitzplus/gui_link.cpp` (110 aktive Einträge),
`blitzplus/b2d_link.cpp` (Canvas, Ereignisse, Maus) und
`blitzplus/sys_link.cpp` (Meldungen, Timer, `FlushEvents`, `HotKeyEvent`).
Alle `Create…`-Befehle für Gadgets haben dieselbe Form
`(x, y, width, height, group [,style])`; bei `CreateButton`, `CreateLabel` und
`CreateWindow` steht ein Text davor, bei `CreateToolBar` die Bilddatei.

| Stufe | Inhalt |
|-------|--------|
| 1 | **Fenster:** `CreateWindow(text$, x, y, w, h [,group [,style]])` mit Stil 15 als Vorgabe (1 Titelzeile, 2 größenveränderlich, 4 Menü, 8 Statuszeile, 16 Werkzeugfenster, 32 Maße als Innenfläche), `ActiveWindow`, `ActivateWindow`, `MinimizeWindow`, `MaximizeWindow`, `RestoreWindow`, `WindowMinimized`, `WindowMaximized`, `SetStatusText`, `SetMinWindowSize`, `Desktop`, `ClientWidth`, `ClientHeight`. **Menüs:** `CreateMenu(text$, id, parent)`, `WindowMenu`, `UpdateWindowMenu`, `SetMenuText`, `MenuText`, `CheckMenu`, `UncheckMenu`, `MenuChecked`, `EnableMenu`, `DisableMenu`, `MenuEnabled`. **Alle Gadgets:** `FreeGadget`, `SetGadgetShape`, `SetGadgetLayout gadget, left, right, top, bottom`, `SetGadgetText`, `GadgetText`, `SetGadgetFont`, `GadgetFont`, `ShowGadget`, `HideGadget`, `EnableGadget`, `DisableGadget`, `ActivateGadget`, `GadgetX`, `GadgetY`, `GadgetWidth`, `GadgetHeight`, `GadgetGroup`. **Einzelne:** `CreateButton` (Stil 1 Knopf, 2 Ankreuzfeld, 3 Auswahlknopf, 4 löst bei Return aus, 5 bei Escape) mit `SetButtonState`/`ButtonState`; `CreateLabel`; `CreateTextField` mit `TextFieldText`; `CreateSlider` mit `SetSliderRange`, `SetSliderValue`, `SliderValue`; `CreateProgBar` mit `UpdateProgBar`; `CreatePanel` mit `SetPanelColor`, `SetPanelImage`. **Canvas:** `CreateCanvas`, `CanvasBuffer`, `FlipCanvas`. **Ereignisse:** siehe 11.3a. **Abfragen:** `RequestFile`, `RequestDir` |
| 2 | `CreateListBox`, `CreateComboBox`, `CreateTabber` mit den gemeinsamen Eintragsbefehlen (`AddGadgetItem`, `InsertGadgetItem`, `ModifyGadgetItem`, `RemoveGadgetItem`, `SelectGadgetItem`, `SelectedGadgetItem`, `CountGadgetItems`, `GadgetItemText`, `ClearGadgetItems`); `CreateTreeView` mit elf Knotenbefehlen (`TreeViewRoot`, `AddTreeViewNode`, `SelectTreeViewNode`, `ExpandTreeViewNode` …); `CreateToolBar` mit `EnableToolBarItem`, `DisableToolBarItem`, `SetToolBarTips`; Bildstreifen (`LoadIconStrip`, `FreeIconStrip`, `SetGadgetIconStrip`); `RequestColor` mit `RequestedRed`/`Green`/`Blue`, `RequestFont` |
| 3 | `CreateTextArea` mit 15 weiteren Befehlen (`SetTextAreaText`, `AddTextAreaText`, `FormatTextAreaText`, `TextAreaText`, `TextAreaCursor`, `TextAreaSelLen`, `LockTextArea` …) — ein mehrzeiliger Editor mit Formatierung, der teuerste Einzelposten |
| — | `CreateHtmlView` und seine sieben Befehle sind nicht vorgesehen. Im Repository sind sie in der Befehlstabelle auskommentiert, ebenso `CreateSplitter`/`SplitterPanel`; die Hilfe beschreibt HtmlView noch. |

Einschätzung: Stufe 1 reicht für einfache Werkzeuge wie einen Level-Editor.

**Überschneidung mit dem, was BLTZNXT schon hat** (`commands.h`):

| Befehl | BLTZNXT heute | BlitzPlus |
|--------|---------------|-----------|
| `Notify`, `Confirm`, `Proceed` | ein Parameter `msg$` | zusätzlich `serious=0` |
| `CreateTimer`, `FreeTimer`, `WaitTimer` | vorhanden, `hz%` | `hertz#`; dazu `TimerTicks`, `ResetTimer`, `PauseTimer`, `ResumeTimer` und das Ereignis `$4001` |
| `MouseX`, `MouseY`, `MouseXSpeed`, `MouseYSpeed`, `MoveMouse`, `ShowPointer`, `HidePointer` | ohne Canvas | zusätzlich `canvas=0` am Ende |

Alle drei Erweiterungen sind zusätzliche Parameter mit Vorgabewert; ein
Blitz3D-Programm merkt davon nichts.

### 11.3a Ereignisse

Befehle (`b2d_link.cpp`, `sys_link.cpp`): `WaitEvent([timeout])` mit −1 als
Vorgabe, `PeekEvent`, `EventID`, `EventData`, `EventX`, `EventY`, `EventZ`,
`EventSource`, `FlushEvents [kind [,source]]`,
`HotKeyEvent rawkey, modifier, ev_id [,ev_data, ev_x, ev_y, ev_z, ev_source]`,
`AutoSuspend enable`.

Nummern aus `event/event.h`:

| Nummer | Ereignis |
|--------|----------|
| `$101`, `$102`, `$103` | Taste gedrückt, losgelassen, Zeichen |
| `$201`, `$202`, `$203`, `$204` | Maustaste gedrückt, losgelassen, Maus bewegt, Mausrad |
| `$205`, `$206` | Maus betritt, verlässt |
| `$401` | Gadget betätigt |
| `$801`, `$802`, `$803`, `$804` | Fenster verschoben, Größe geändert, geschlossen, aktiviert |
| `$1001` | Menüpunkt gewählt |
| `$2001`, `$2002` | Programm angehalten, fortgesetzt |
| `$2003`, `$2004`, `$2005` | Anzeige geändert, modaler Dialog beginnt, endet |
| `$4001` | Timer |

Die Quelle kennt außerdem `$104`/`$105` (`KEY_OKAY`, `KEY_CANCEL`) und `$402`
(`GADGET_ACTIVATE`). ? Ob Programme sie zu sehen bekommen, ist nicht geprüft.

### 11.4 Umbau der Laufzeit

- **Mehrere Fenster.** Heute gibt es ein Fenster und einen 2D-Renderer als
  globale Variablen (0.1). Daraus werden Objekte; `Graphics` und `Graphics3D`
  legen weiter ihr eines Fenster an, alte Programme merken nichts.
- **Ereignisschlange neben dem Abfragen.** `KeyHit`, `MouseX` und die übrigen
  Eingabebefehle arbeiten weiter wie bisher. Die Ereignisse der Oberfläche
  kommen in dieselbe Verarbeitung (`bb_sdl_process_event_`) als zweite Sicht
  auf dieselben Eingaben. BlitzPlus ist genauso gebaut: Die Ereignisse sind
  dort das Erste, und ein Filter (`B2DEventFilter` in `blitz2d/b2devent.cpp`)
  leitet daraus den Zustand für `KeyDown`, `KeyHit`, `GetKey`, `MouseDown`,
  `MouseHit` und `MouseZ` ab.
- **Canvas als Zieltextur.** Ein Canvas ist ein Puffer für `SetBuffer` und ein
  mögliches Ziel für `CameraRenderTarget` (5.3). 2D in einem Canvas geht mit
  den Mitteln aus `bb_canvas.h`; 3D in einem Canvas braucht die Zieltexturen
  und hängt an O16.

### 11.5 Namen

◇ Die Befehle heißen wie in BlitzPlus (`CreateWindow`, `CreateButton`,
`WaitEvent` …). Wer BlitzPlus kennt, findet sich sofort zurecht, und Anleitungen
von damals gelten weiter.

Die Namen stoßen mit alten Blitz3D-Programmen zusammen, die sich ihre
Oberfläche selbst gebaut haben; gemessen ist das für `CreateButton` in bnb
(Abschnitt 1). Mit dem Vorschlag zu O1 — die eigene Funktion des Programms
gewinnt — laufen solche Programme unverändert weiter.
