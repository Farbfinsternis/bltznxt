; 3D-13 (zweiter Teil) - LoadMesh und LoadAnimMesh fuer .3ds.
;
; Die Roadmap plante hier einen .b3d-Loader. Gezaehlt an der mitgelieferten
; Installation laden die 32 Beispielprogramme aber 39-mal .x und 20-mal .3ds
; und kein einziges Mal .b3d; .b3d-Dateien gibt es dort ueberhaupt nicht.
; Deshalb faengt der Loader mit .3ds an.
;
; Die Testdatei tests/assets/test_box.3ds ist eigens erzeugt, damit die
; erwarteten Zahlen feststehen: zwei Quader, zwei Materialien, keine Textur.
; In 3DS-Koordinaten misst das Ganze x=12, y=4, z=6.
;
; Am laufenden Original nachgemessen und hier zugesichert:
;   Die Achsen tauschen y und z - blitz(x,y,z) = 3ds(x,z,y). Aus x=12, y=4,
;   z=6 wird also w=12, h=6, d=4. Genau das meldet das Original fuer
;   dieselbe Datei, und zwei Flaechen fuer die zwei Materialien.
;
; Weitere am Original gemessene Zusagen, die diese Datei nicht sichtbar macht
; und die deshalb im DEVLOG stehen: das lokale Koordinatensystem wird
; ignoriert, der Drehpunkt aus dem Keyframe-Abschnitt wird abgezogen, gleiche
; Brushes werden zu einer Flaeche zusammengefasst, die Materialfarbe gilt nur
; ohne Textur, und zweiseitige Materialien werden ohne
; Rueckseitenentfernung gezeichnet.

Graphics3D 320,240,32,2

cam = CreateCamera()
PositionEntity cam, 0, 0, -30
licht = CreateLight()

; --- 1) laden und die Achsen pruefen
m = LoadMesh("tests/assets/test_box.3ds")
If m <> 0 Then Print "geladen" Else Print "FEHLER geladen"
If EntityClass$(m) = "Mesh" Then Print "klasse" Else Print "FEHLER klasse"
If MeshWidth(m) = 12 Then Print "breite" Else Print "FEHLER breite"
If MeshHeight(m) = 6 Then Print "hoehe (3ds z)" Else Print "FEHLER hoehe (3ds z)"
If MeshDepth(m) = 4 Then Print "tiefe (3ds y)" Else Print "FEHLER tiefe (3ds y)"

; --- 2) zwei Materialien werden zu zwei Flaechen
If CountSurfaces(m) = 2 Then Print "flaechen" Else Print "FEHLER flaechen"

; --- 3) das geladene Netz ist ein gewoehnliches Netz
ScaleMesh m, 2, 2, 2
If MeshWidth(m) = 24 Then Print "scalemesh darauf" Else Print "FEHLER scalemesh darauf"
ScaleMesh m, 0.5, 0.5, 0.5
UpdateWorld
RenderWorld
If TrisRendered() = 24 Then Print "dreiecke" Else Print "FEHLER dreiecke"

; --- 4) es laesst sich bewegen wie jedes Entity
PositionEntity m, 5, 0, 0
If EntityX(m) = 5 Then Print "positionierbar" Else Print "FEHLER positionierbar"
FreeEntity m

; --- 5) Elternknoten. Laut Doku entsteht das Netz trotzdem bei 0,0,0 -
;        dieselbe Regel wie bei CreateLight.
piv = CreatePivot()
PositionEntity piv, 10, 10, 10
k = LoadMesh("tests/assets/test_box.3ds", piv)
If k <> 0 Then Print "mit eltern" Else Print "FEHLER mit eltern"
If GetParent(k) = piv Then Print "eltern gesetzt" Else Print "FEHLER eltern gesetzt"
If EntityX(k) = 0 And EntityY(k) = 0 Then Print "eltern-position" Else Print "FEHLER eltern-position"
FreeEntity k

; --- 6) LoadAnimMesh laedt dieselbe Geometrie. Hierarchie und Animation
;        gibt es noch nicht; der Befehl meldet das einmal und laedt sonst
;        wie LoadMesh.
a = LoadAnimMesh("tests/assets/test_box.3ds")
If a <> 0 Then Print "animmesh" Else Print "FEHLER animmesh"
If MeshWidth(a) = 12 And CountSurfaces(a) = 2 Then Print "animmesh gleich" Else Print "FEHLER animmesh gleich"
FreeEntity a

; --- 7) eine fehlende Datei ist Handle 0, kein Absturz
If LoadMesh("tests/assets/gibtsnicht.3ds") = 0 Then Print "fehlend" Else Print "FEHLER fehlend"

; --- 8) noch nicht umgesetzte Formate liefern 0 mit Meldung, statt still
;        ein leeres Netz zu bauen
If LoadMesh("tests/assets/test_grid.png") = 0 Then Print "fremde endung" Else Print "FEHLER fremde endung"

; --- 9) richtige Endung, unbrauchbarer Inhalt: der Hauptchunk gibt eine
;        Laenge weit hinter dem Dateiende an. Das muss in den
;        Grenzpruefungen haengenbleiben und 0 liefern statt zu lesen,
;        was hinter dem Puffer liegt.
If LoadMesh("tests/assets/test_broken.3ds") = 0 Then Print "kaputte datei" Else Print "FEHLER kaputte datei"

; --- 10) LoaderMatrix. Der Achsentausch ist im Original kein Sonderfall
;         des .3ds-Lesers, sondern eine Matrix je Endung, die das
;         Programm aendern kann. Vorgabe laut Doku:
;           LoaderMatrix "3ds",1,0,0, 0,0,1, 0,1,0
;         Setzt man dort die Einheitsmatrix, entfaellt der Tausch und
;         aus 3DS x=12, y=4, z=6 wird w=12, h=4, d=6.
LoaderMatrix "3ds", 1,0,0, 0,1,0, 0,0,1
e = LoadMesh("tests/assets/test_box.3ds")
If MeshWidth(e) = 12 And MeshHeight(e) = 4 And MeshDepth(e) = 6 Then Print "loadermatrix einheit" Else Print "FEHLER loadermatrix einheit"
FreeEntity e

; Vorgabe wieder herstellen
LoaderMatrix "3ds", 1,0,0, 0,0,1, 0,1,0
e = LoadMesh("tests/assets/test_box.3ds")
If MeshWidth(e) = 12 And MeshHeight(e) = 6 And MeshDepth(e) = 4 Then Print "loadermatrix zurueck" Else Print "FEHLER loadermatrix zurueck"
FreeEntity e

; --- 11) .x im Textformat. tests/assets/test_frames.x ist eigens erzeugt
;         (scripts/make_x_asset.py): zwei Vierecke, das zweite in einem
;         Frame um +10 in x verschoben, zwei Materialien - eines inline,
;         eines als Verweis {rot} - und die Vorlage absichtlich als
;         "TextureFileName" mit grossem N geschrieben, wie in interior.X
;         der Installation.
;
;         Das Original meldet fuer dieselbe Datei w=11 h=2 d=0, zwei
;         Flaechen und vier Dreiecke.
x = LoadMesh("tests/assets/test_frames.x")
If x <> 0 Then Print "x geladen" Else Print "FEHLER x geladen"

; Ohne die Frame-Matrix waere die Breite 2 statt 11 - das prueft, dass
; die Hierarchie in die Vertices gerechnet wird.
If MeshWidth(x) = 11 Then Print "x frame-matrix" Else Print "FEHLER x frame-matrix"
If MeshHeight(x) = 2 And MeshDepth(x) = 0 Then Print "x ausmasse" Else Print "FEHLER x ausmasse"

; Zwei Materialien ergeben zwei Flaechen. Der Verweis {rot} muss dabei
; aufgeloest werden; wer die Klammern falsch zaehlt, verliert alles
; dahinter und kommt auf eine.
If CountSurfaces(x) = 2 Then Print "x flaechen" Else Print "FEHLER x flaechen"

; Zwei Vierecke werden zu vier Dreiecken zerlegt.
UpdateWorld
RenderWorld
If TrisRendered() = 4 Then Print "x dreiecke" Else Print "FEHLER x dreiecke"
FreeEntity x

; --- 12) ein gueltiger Binaerkopf mit unbrauchbarem Inhalt. Die Kodierung
;         selbst pruefe tests/test_3d13_x_binaer.bb; hier geht es nur
;         darum, dass der Leser daran haengenbleibt statt hinter den
;         Puffer zu lesen.
If LoadMesh("tests/assets/test_binary.x") = 0 Then Print "x binaer" Else Print "FEHLER x binaer"

Print "fertig"
