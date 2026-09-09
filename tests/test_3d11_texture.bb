; 3D-11 - Texturen: LoadTexture, LoadAnimTexture, CreateTexture, FreeTexture,
; TextureWidth/Height/Name, EntityTexture, TextureBlend/Coords,
; ScaleTexture/PositionTexture/RotateTexture, TextureFilter, ActiveTextures.
;
; EntityTexture steht in 54, LoadTexture in 48 der 130 mitgelieferten
; Beispielprogramme - beides haeufiger als CreateLight (39). Die Flags stammen
; aus help/commands/3d_commands/CreateTexture.htm:
;
;   1 Color (Vorgabe)   2 Alpha      4 Masked     8 Mipmapped
;  16 Clamp U          32 Clamp V   64 Sphere   128 Cube
; 256 VRAM            512 High-Color
;
; **Was dieser Test zusichert und was nicht.** Geprueft sind die
; sprachsichtbaren Zusagen: Handles, Groessen, Namen, Framezerlegung, die
; dokumentierte Regel von FreeTexture und dass eine texturierte Szene
; durchlaeuft. Das *Aussehen* sichert er nicht zu. Die Richtung von
; ScaleTexture, PositionTexture und RotateTexture ist stattdessen am
; laufenden Original ausgemessen worden (siehe bb_texture.h und DEVLOG).

Graphics3D 320,240,32,2

cam = CreateCamera()
PositionEntity cam, 0, 0, -4
licht = CreateLight()
wuerfel = CreateCube()

; --- 1) laden
tex = LoadTexture("tests/assets/test_grid.png")
If tex <> 0 Then Print "handle" Else Print "FEHLER handle"
If TextureWidth(tex) = 16 And TextureHeight(tex) = 16 Then Print "groesse" Else Print "FEHLER groesse"

; TextureName liefert laut Doku den absoluten Dateinamen.
n$ = TextureName$(tex)
If Instr(n$, "test_grid.png") > 0 Then Print "name" Else Print "FEHLER name"
If Len(n$) > Len("test_grid.png") Then Print "name absolut" Else Print "FEHLER name absolut"

; Eine fehlende Datei ist kein Absturz, sondern Handle 0.
fehlt = LoadTexture("tests/assets/gibtsnicht.png")
If fehlt = 0 Then Print "fehlend" Else Print "FEHLER fehlend"

; --- 2) erzeugen
leer = CreateTexture(64, 32)
If leer <> 0 Then Print "erzeugt" Else Print "FEHLER erzeugt"
If TextureWidth(leer) = 64 And TextureHeight(leer) = 32 Then Print "erzeugt groesse" Else Print "FEHLER erzeugt groesse"
mehr = CreateTexture(8, 8, 1, 4)
If mehr <> 0 Then Print "erzeugt frames" Else Print "FEHLER erzeugt frames"

; --- 3) Animationstextur: der Streifen ist 32x8 und zerfaellt in vier
;        Frames zu 8x8. Gemeldet wird die Framegroesse, nicht die Bildgroesse.
anim = LoadAnimTexture("tests/assets/test_strip.png", 1, 8, 8, 0, 4)
If anim <> 0 Then Print "anim" Else Print "FEHLER anim"
If TextureWidth(anim) = 8 And TextureHeight(anim) = 8 Then Print "anim groesse" Else Print "FEHLER anim groesse"

; --- 4) an eine Entity binden, auch auf einem zweiten Index
EntityTexture wuerfel, tex
EntityTexture wuerfel, leer, 0, 1
EntityTexture wuerfel, anim, 2
Print "gebunden"

; Ein ungueltiger Index bleibt folgenlos statt daneben zu schreiben.
EntityTexture wuerfel, tex, 0, 99
Print "index ausserhalb"

; --- 5) die Einstellbefehle lassen das Handle unangetastet
ScaleTexture    tex, 2, 2
PositionTexture tex, 0.25, 0.5
RotateTexture   tex, 45
TextureBlend    tex, 3
TextureCoords   tex, 0
If TextureWidth(tex) = 16 Then Print "unveraendert" Else Print "FEHLER unveraendert"

ScaleTexture    tex, 1, 1
PositionTexture tex, 0, 0
RotateTexture   tex, 0
TextureBlend    tex, 2

; --- 6) Filterliste: beide Befehle laufen, ClearTextureFilters nimmt der
;        Vorgabe "",1+8 ihre Wirkung.
ClearTextureFilters
TextureFilter "_alpha", 1 + 2 + 8
ohne = LoadTexture("tests/assets/test_grid.png", 1)
If ohne <> 0 Then Print "filter" Else Print "FEHLER filter"

; --- 7) ActiveTextures zaehlt die lebenden Handles
vorher = ActiveTextures()
weg = CreateTexture(4, 4)
If ActiveTextures() = vorher + 1 Then Print "zaehler" Else Print "FEHLER zaehler"
FreeTexture weg
If ActiveTextures() = vorher Then Print "zaehler nach frei" Else Print "FEHLER zaehler nach frei"

If HWTexUnits() > 0 Then Print "texunits" Else Print "FEHLER texunits"

; --- 8) FreeTexture: das Handle ist danach unbrauchbar, aber laut Doku
;        verlieren bereits texturierte Entities ihre Textur *nicht*.
FreeTexture leer
If TextureWidth(leer) = 0 Then Print "frei" Else Print "FEHLER frei"

; --- 9) eine texturierte Szene laeuft durch
For i = 1 To 3
  UpdateWorld
  RenderWorld
  Flip
Next
If TrisRendered() > 0 Then Print "gerendert" Else Print "FEHLER gerendert"

Print "fertig"
