; BUG-223: GetBrushTexture gibt auch fuer eine leere Lage ein Handle zurueck.
; bbGetBrushTexture legt immer d_new Texture(b->getTexture(index)) an; das
; Handle zeigt dann auf keine Textur. TextureName liefert darauf "",
; TextureBuffer 0, und EntityTexture mit ihm nimmt der Entity die Textur.
; Alle Zeilen am Original gemessen (Blitz3D 1.108, build/bug223_20261027).
; TextureWidth auf ein leeres Handle beendet das Original ohne Meldung und
; wird deshalb nicht geprueft.
Graphics3D 320,240,0,2
cam=CreateCamera() : PositionEntity cam,0,0,-3
c=CreateCube() : EntityFX c,1
tx=CreateTexture(16,16) : SetBuffer TextureBuffer(tx) : ClsColor 0,255,0 : Cls : SetBuffer BackBuffer()
EntityTexture c,tx
leer=GetBrushTexture(CreateBrush())
Print "leer "+(leer<>0)+" anders "+(leer<>GetBrushTexture(CreateBrush()))
Print "name ["+TextureName(leer)+"] puffer "+(TextureBuffer(leer)<>0)
RenderWorld : p=ReadPixel(160,120) And $FFFFFF : Print "mit textur "+Hex(p)
EntityTexture c,leer
RenderWorld : p=ReadPixel(160,120) And $FFFFFF : Print "nach leerer textur "+Hex(p)
b=CreateBrush() : BrushTexture b,tx : PaintEntity c,b : EntityTexture c,tx
br=GetEntityBrush(c) : t0=GetBrushTexture(br) : Print "volle lage "+(t0<>0)+" name gleich "+(TextureName(t0)=TextureName(tx))
Print "lage 1 "+(GetBrushTexture(br,1)<>0)
FreeTexture leer
Print "frei ok"
End
