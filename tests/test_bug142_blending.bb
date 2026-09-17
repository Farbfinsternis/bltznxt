; BUG-142 - durchscheinende Objekte mischen mit dem Hintergrund.
;
; Im Vorgabemodus (Blend 0) wurde glBlendFunc nie gesetzt: EntityAlpha mischte
; gar nicht oder mit der Mischfunktion des vorigen Objekts, BrushAlpha,
; Vertex-Alpha (EntityFX 32) und Alpha-Texturen wurden schwarz oder falsch.
; Alle Werte am Original gemessen (2026-09-17), EntityFX 1 vor blauem
; Hintergrund. Die Werte sind die klassische Alpha-Mischung; aendert eine
; spaetere Pipeline (HDR, Tone-Mapping) sie, bleibt zu pruefen, dass der
; Hintergrund im selben Anteil durchscheint.

Graphics3D 320,240,0,2
cam = CreateCamera() : PositionEntity cam,0,0,-3 : CameraClsColor cam,0,0,255

Function Px$(name$)
	RenderWorld
	Print name + " " + Hex(ReadPixel(160,120) And $FFFFFF)
End Function

c = CreateCube() : EntityFX c,1 : EntityColor c,255,0,0
Px("deckend")
EntityAlpha c,0.5
Px("entityalpha 0.5")
EntityBlend c,1
Px("entityalpha 0.5 blend 1")
EntityAlpha c,0
Px("entityalpha 0")
EntityAlpha c,1
EntityBlend c,3
Px("blend 3 add")
EntityBlend c,2
Px("blend 2 multiply")
FreeEntity c

c = CreateCube()
b = CreateBrush(255,0,0) : BrushAlpha b,0.25 : BrushFX b,1
PaintEntity c,b
Px("brushalpha 0.25")
FreeEntity c

c = CreateCube() : EntityFX c,1+2+32
s = GetSurface(c,1)
For v = 0 To CountVertices(s)-1 : VertexColor s,v,0,255,0,0.5 : Next
Px("fx 32 vertexalpha 0.5")
EntityFX c,1+2
Px("fx 2 ohne 32")
FreeEntity c

c = CreateCube() : EntityFX c,1
t = CreateTexture(16,16,1+2)
LockBuffer TextureBuffer(t)
For y = 0 To 15 : For x = 0 To 15 : WritePixelFast x,y,$80FFFFFF,TextureBuffer(t) : Next : Next
UnlockBuffer TextureBuffer(t)
EntityTexture c,t
Px("alpha-textur 0.5 weiss")
End
