; 3D-15 (erster Teil) - Brushes als Sprachobjekte.
;
; Ein Brush ist das Aussehen als Wert: Farbe, Deckkraft, Glanz, Blendmodus,
; FX-Flags und bis zu acht Texturlagen. An der Flaeche haengt er seit dem
; .3ds-Loader; neu ist, dass ein Programm ihn anfassen kann.
;
; Gemessen wird ueber TrisRendered - die einzige Groesse, die beide Systeme
; aus der Sprache heraus melden und die auf die Deckkraft reagiert, denn ein
; Entity mit Alpha 0 wird gar nicht gezeichnet. Alle Zahlen hier sind am
; laufenden Original nachgemessen, nicht abgeleitet; drei davon haetten wir
; anders geraten:
;
;   * PaintEntity ersetzt den GANZEN Brush. Ein vorher gesetztes
;     EntityAlpha 0 ist danach weg, der Wuerfel wieder da.
;   * PaintMesh mit Deckkraft 0 laesst den Wuerfel TROTZDEM zeichnen: fuer
;     das Weglassen zaehlt allein die Deckkraft der Entity, nicht das
;     Produkt aus Flaeche und Entity.
;   * Ein Brush wird beim Auftragen KOPIERT. Wer ihn danach aendert, aendert
;     das Bemalte nicht mehr - im Original ist Brush eine Wertklasse.

Graphics3D 320, 240, 32, 2
cam = CreateCamera()
PositionEntity cam, 0, 0, -6
licht = CreateLight()

w = CreateCube()
zeig("grundfall", 12)

EntityAlpha w, 0
zeig("entityalpha 0", 0)

b = CreateBrush()
PaintEntity w, b
zeig("paintentity nach alpha 0", 12)

BrushAlpha b, 0
zeig("brush nachtraeglich auf 0", 12)

PaintEntity w, b
zeig("paintentity mit alpha 0", 0)

b2 = CreateBrush()
PaintEntity w, b2
zeig("paintentity mit frischem brush", 12)

b3 = CreateBrush()
BrushAlpha b3, 0
PaintMesh w, b3
zeig("paintmesh mit alpha 0", 12)

b4 = CreateBrush()
PaintMesh w, b4
zeig("paintmesh mit alpha 1", 12)

; Ein freigegebenes Handle darf nichts anrichten
FreeBrush b4
PaintEntity w, b4
zeig("paintentity mit freigegebenem brush", 12)

; GetEntityBrush gibt eine Kopie: neues Handle, und Aenderungen daran
; erreichen die Entity erst beim naechsten Auftragen
g = GetEntityBrush(w)
If g <> 0 And g <> b2 Then Print "getentitybrush neues handle" Else Print "FEHLER getentitybrush neues handle"
BrushAlpha g, 0
zeig("kopie geaendert, nicht aufgetragen", 12)
PaintEntity w, g
zeig("kopie aufgetragen", 0)

; --- Texturen am Brush ---

If LoadBrush("tests/assets/gibtsnicht.bmp") = 0 Then Print "loadbrush fehlend" Else Print "FEHLER loadbrush fehlend"

lb = LoadBrush("tests/assets/test_grid.png")
If lb <> 0 Then Print "loadbrush geladen" Else Print "FEHLER loadbrush geladen"

t = GetBrushTexture(lb)
If t <> 0 Then Print "getbrushtexture lage 0" Else Print "FEHLER getbrushtexture lage 0"
If TextureWidth(t) = 16 And TextureHeight(t) = 16 Then Print "textur groesse" Else Print "FEHLER textur groesse"

; Eine leere Lage hat keine Textur
If GetBrushTexture(lb, 3) = 0 Then Print "getbrushtexture leere lage" Else Print "FEHLER getbrushtexture leere lage"

; Die zurueckgegebene Textur ist ein eigenes Handle - sie freizugeben nimmt
; dem Brush seine Textur nicht weg.
FreeTexture t
t2 = GetBrushTexture(lb)
If t2 <> 0 Then Print "textur ueberlebt freetexture" Else Print "FEHLER textur ueberlebt freetexture"

; Eine Lage ausserhalb des Bereichs tut nichts und stuerzt nicht ab
BrushTexture lb, t2, 0, 99
Print "brushtexture ausserhalb ok"

Print "fertig"
End

Function zeig(name$, erwartet)
	UpdateWorld
	RenderWorld
	If TrisRendered() = erwartet Then Print name$ Else Print "FEHLER " + name$ + " (" + Str(TrisRendered()) + " statt " + Str(erwartet) + ")"
End Function
