; 3D-13 (vierter Teil) - .x in der Binaerkodierung.
;
; Das Binaerformat ist keine andere Sprache, sondern dieselbe mit anderen
; Marken: statt Zeichen ein Strom aus 16-Bit-Marken, an denen je nach Marke
; Daten haengen. Der Objektbaum darueber und alle Bedeutungsregeln sind
; dieselben - deshalb ist die schaerfste Zusicherung nicht eine Liste von
; Zahlen, sondern dass **beide Kodierungen derselben Szene Zahl fuer Zahl
; dasselbe ergeben**. Weichen sie ab, war es der Tokenizer.
;
; tests/assets/test_frames_bin.x entsteht mit scripts/make_x_asset.py aus
; derselben Szene wie test_frames.x und enthaelt absichtlich die Stellen, an
; denen ein Binaerleser danebengreift:
;   * eine Vorlage mit GUID und Typwoertern, die uebersprungen werden muss,
;   * einen Text (TextureFileName) - nach einem Text steht KEINE weitere
;     Laengenangabe; wer dort vier Byte verbraucht, verschluckt die beiden
;     folgenden Marken, in der Praxis ein ';' und ein '}',
;   * einen Verweis in eigenen Klammern, {rot},
;   * beide Zahlenformen, die einzelne ganze Zahl und die Liste.
;
; Die acht binaeren .x-Dateien der Installation sind gegen das laufende
; Original gemessen (alle acht gleich in Flaechenzahl und allen drei
; Massen); das steht in der Commit-Botschaft, nicht hier - hier steht, was
; ohne die Installation pruefbar ist.

Graphics3D 320,240,32,2

cam = CreateCamera()
PositionEntity cam, 0, 0, -30
licht = CreateLight()

txt = LoadMesh("tests/assets/test_frames.x")
bina = LoadMesh("tests/assets/test_frames_bin.x")

If bina <> 0 Then Print "binaer geladen" Else Print "FEHLER binaer geladen"

; Die eigentliche Zusicherung: beide Kodierungen, dasselbe Ergebnis.
If MeshWidth(bina) = MeshWidth(txt) And MeshHeight(bina) = MeshHeight(txt) And MeshDepth(bina) = MeshDepth(txt) Then Print "masse gleich" Else Print "FEHLER masse gleich"
If CountSurfaces(bina) = CountSurfaces(txt) Then Print "flaechen gleich" Else Print "FEHLER flaechen gleich"

; Und dieselben absoluten Zahlen wie beim Textformat: ohne die Frame-Matrix
; waere die Breite 2 statt 11, ohne den aufgeloesten Verweis eine Flaeche
; statt zwei.
If MeshWidth(bina) = 11 Then Print "binaer frame-matrix" Else Print "FEHLER binaer frame-matrix"
If MeshHeight(bina) = 2 And MeshDepth(bina) = 0 Then Print "binaer ausmasse" Else Print "FEHLER binaer ausmasse"
If CountSurfaces(bina) = 2 Then Print "binaer flaechen" Else Print "FEHLER binaer flaechen"

; Zwei Vierecke werden zu vier Dreiecken zerlegt.
FreeEntity txt
UpdateWorld
RenderWorld
If TrisRendered() = 4 Then Print "binaer dreiecke" Else Print "FEHLER binaer dreiecke"
FreeEntity bina

; Ein gueltiger Kopf mit unbrauchbarem Inhalt darf nicht durchgehen und auch
; nicht hinter den Puffer lesen.
If LoadMesh("tests/assets/test_binary.x") = 0 Then Print "binaer leer" Else Print "FEHLER binaer leer"

Print "fertig"
