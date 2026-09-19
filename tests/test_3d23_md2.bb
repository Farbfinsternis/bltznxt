; 3D-23 - MD2: LoadMD2, AnimateMD2, MD2AnimTime, MD2AnimLength, MD2Animating.
;
; Der Test schreibt sich ein kleines MD2 selbst: ein Dreieck, drei Frames,
; die es je Frame um 2 Einheiten nach rechts schieben. Achsen wie im
; Original (md2rep.cpp): Blitz3D-x = md2.y, y = md2.z, z = md2.x.
; Alle Werte am Original gemessen (2026-09-19).

Graphics3D 200,200,0,2
SetBuffer BackBuffer()

Function Md2Schreiben(datei$)
	f=WriteFile(datei)
	WriteInt f,$32504449 : WriteInt f,8          ; "IDP2", Version 8
	WriteInt f,2 : WriteInt f,2                  ; skinWidth, skinHeight
	WriteInt f,52                                ; frameSize
	WriteInt f,0 : WriteInt f,3 : WriteInt f,3   ; numSkins, numVertices, numTexCoords
	WriteInt f,1 : WriteInt f,0 : WriteInt f,3   ; numTriangles, numGlCommands, numFrames
	WriteInt f,68 : WriteInt f,68 : WriteInt f,80 ; offsetSkins, offsetTexCoords, offsetTriangles
	WriteInt f,92 : WriteInt f,248 : WriteInt f,248 ; offsetFrames, offsetGlCommands, offsetEnd
	; UVs
	WriteShort f,0 : WriteShort f,0
	WriteShort f,2 : WriteShort f,0
	WriteShort f,0 : WriteShort f,2
	; Dreieck: Vertices 0,1,2, UVs 0,1,2
	WriteShort f,0 : WriteShort f,1 : WriteShort f,2
	WriteShort f,0 : WriteShort f,1 : WriteShort f,2
	; Frames
	For k=0 To 2
		WriteFloat f,0.1 : WriteFloat f,0.1 : WriteFloat f,0.1   ; scale (md2 x,y,z)
		WriteFloat f,0 : WriteFloat f,-5+2*k : WriteFloat f,-5   ; translate
		For i=1 To 16 : WriteByte f,0 : Next                     ; Name
		WriteByte f,0 : WriteByte f,0 : WriteByte f,0 : WriteByte f,0
		WriteByte f,0 : WriteByte f,100 : WriteByte f,0 : WriteByte f,0
		WriteByte f,0 : WriteByte f,0 : WriteByte f,100 : WriteByte f,0
	Next
	CloseFile f
End Function

Md2Schreiben("test_md2.md2")

cam=CreateCamera()
PositionEntity cam,0.013,0.031,-10.37   ; keine Kante genau auf Pixelmitten (Fuellregel)
CameraClsColor cam,0,0,0

Function Kante$(titel$)
	RenderWorld
	LockBuffer BackBuffer()
	l=-1 : r=-1
	For x=0 To 199
		If ReadPixelFast(x,130) And $FF0000
			If l<0 Then l=x
			r=x
		EndIf
	Next
	UnlockBuffer BackBuffer()
	Return titel+" kante="+l+"-"+r+" tris="+TrisRendered()
End Function

m=LoadMD2("test_md2.md2")
Print "klasse="+EntityClass(m)+" len="+MD2AnimLength(m)+" t="+MD2AnimTime(m)+" an="+MD2Animating(m)
Print "fehlt="+LoadMD2("gibtsnicht.md2")
EntityColor m,255,0,0
EntityFX m,1
Print Kante("frame0 fx1")
EntityFX m,1+16
Print Kante("frame0 fx17")

AnimateMD2 m,1,0.25,0,2
s$="loop:"
For i=1 To 10 : UpdateWorld : s=s+" "+MD2AnimTime(m) : Next
Print s
Print Kante("t="+MD2AnimTime(m))
AnimateMD2 m,2,0.75,0,2
s$="pingpong:"
For i=1 To 6 : UpdateWorld : s=s+" "+MD2AnimTime(m) : Next
Print s
Print Kante("t="+MD2AnimTime(m))
AnimateMD2 m,3,0.3,0,1   ; bis 2: das Original liest dann Frame 3 und stuerzt ab
s$="oneshot:"
For i=1 To 8 : UpdateWorld 2 : s=s+" "+MD2AnimTime(m)+"/"+MD2Animating(m) : Next
Print s
Print Kante("ende t="+MD2AnimTime(m))
AnimateMD2 m,1,1,0,1,2
UpdateWorld
Print Kante("uebergang halb")
UpdateWorld
Print Kante("uebergang fertig t="+MD2AnimTime(m))
HideEntity m
UpdateWorld
Print "versteckt t="+MD2AnimTime(m)
ShowEntity m
c=CopyEntity(m)
Print "kopie klasse="+EntityClass(c)+" t="+MD2AnimTime(c)+" an="+MD2Animating(c)
FreeEntity c
PositionEntity m,0,0,-20
Print Kante("hinter kamera")
FreeEntity m
DeleteFile "test_md2.md2"
