; BUG-230: Ein zweites Graphics3D (neue Aufloesung, Vollbild an/aus) baut
; Fenster und GL-Kontext neu. Die Shader-Programme blieben als "fertig"
; markiert und fehlten im neuen Kontext: RenderWorld zeichnete nichts, nur
; 2D war zu sehen. Jetzt werden sie beim Moduswechsel freigegeben und im
; neuen Kontext neu uebersetzt. Alles 3D muss danach neu angelegt werden,
; wie im Original (BUG-120).

Function Szene()
	SetBuffer BackBuffer()
	AmbientLight 255,255,255
	cam = CreateCamera()
	CameraClsColor cam,0,0,255
	PositionEntity cam,0,0,-5
	w = CreateCube()
	EntityColor w,255,0,0
	EntityFX w,1
End Function

Function Mitte$()
	RenderWorld
	LockBuffer BackBuffer()
	m = ReadPixelFast(GraphicsWidth() / 2, GraphicsHeight() / 2, BackBuffer())
	r = ReadPixelFast(2, 2, BackBuffer())
	UnlockBuffer BackBuffer()
	Return ((m Shr 16) And 255) + "," + (m And 255) + " Rand " + ((r Shr 16) And 255) + "," + (r And 255)
End Function

Graphics3D 320,240,0,2
Szene()
Print "erstes Fenster " + GraphicsWidth() + ": " + Mitte()
Graphics3D 400,300,0,2
Szene()
Print "zweites Fenster " + GraphicsWidth() + ": " + Mitte()
Graphics3D 320,240,0,2
Szene()
Print "drittes Fenster " + GraphicsWidth() + ": " + Mitte()
End
