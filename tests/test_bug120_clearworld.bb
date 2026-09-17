; BUG-120 - ClearWorld gibt nur frei, was seine Schalter verlangen, und ein
; Moduswechsel leert die 3D-Welt.
;
; Wie bbClearWorld und blitz3d_close im Original, am 2026-09-17 gemessen:
;  - ClearWorld 0,0,0 laesst alles stehen;
;  - freigegebene Brushes und Texturen nehmen bemalten Entities ihr Aussehen
;    nicht, und ein Brush ueberlebt ClearWorld 1,0,0;
;  - ClearWorld ohne Argumente gibt alles frei;
;  - Graphics3D und Graphics schliessen eine offene 3D-Szene mit
;    ClearWorld 1,1,1 - die alte Kamera und der alte Wuerfel rendern nicht mehr.

Graphics3D 320, 240, 0, 2
Global cam

Function Szene()
	cam = CreateCamera()
	PositionEntity cam, 0, 0, -4
	CameraClsColor cam, 0, 0, 0
End Function

Function Px$()
	RenderWorld
	Return ReadPixel(160, 120) And $FFFFFF
End Function

; Pruefsumme einer Bildzeile ueber den Wuerfel
Function Zeile$()
	RenderWorld
	s = 0
	For x = 80 To 240 Step 4
		s = s + ((ReadPixel(x, 120) And $FFFFFF) Mod 9973) * (x Mod 7 + 1)
	Next
	Return s
End Function

Szene()
c = CreateCube() : EntityFX c, 1 : EntityColor c, 255, 0, 0
NameEntity c, "kept"

ClearWorld 0, 0, 0
Print "0,0,0: name " + EntityName(c) + " px " + Px() + " tris " + TrisRendered()

tex = LoadTexture("tests/assets/test_grid.png")
EntityColor c, 255, 255, 255
EntityTexture c, tex
b = CreateBrush(0, 255, 0)
c2 = CreateCube() : PositionEntity c2, 0, 0, -1 : ScaleEntity c2, 0.2, 0.2, 0.2
PaintEntity c2, b
z1$ = Zeile()
p1$ = Px()
ClearWorld 0, 1, 1
Print "0,1,1: zeile gleich " + (Zeile() = z1) + " px gleich " + (Px() = p1) + " name " + EntityName(c)

b3 = CreateBrush(0, 0, 255)
ClearWorld 1, 0, 0
ClsColor 255, 255, 0 : Cls
RenderWorld
Print "1,0,0: px " + (ReadPixel(160, 120) And $FFFFFF) + " tris " + TrisRendered()
Szene()
c3 = CreateCube() : EntityFX c3, 1
PaintEntity c3, b3
Print "brush ueberlebt: " + Px()

ClearWorld
ClsColor 255, 0, 255 : Cls
RenderWorld
Print "ohne argumente: px " + (ReadPixel(160, 120) And $FFFFFF) + " tris " + TrisRendered()

Szene()
c5 = CreateCube() : EntityFX c5, 1
Graphics3D 320, 240, 0, 2
Szene()
Print "nach graphics3d: " + Px() + " tris " + TrisRendered()

c6 = CreateCube() : EntityFX c6, 1
Graphics 320, 240, 0, 2
Graphics3D 320, 240, 0, 2
Szene()
Print "nach graphics und graphics3d: " + Px() + " tris " + TrisRendered()

Print "fertig"
End
