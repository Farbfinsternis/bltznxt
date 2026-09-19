; BUG-172 - LoadImage maskiert sofort mit Schwarz, auch ohne MaskImage.
;
; Gemessen am Original (2026-09-19): nextstage.bmp aus blox-n-balls hat
; 9040 Pixel 0,0,0; DrawImage laesst sie aus, bei 16 und 32 Bit. Bei uns
; wurden sie schwarz gezeichnet ("Level complete" auf schwarzem Kasten).
; Das Bild hier schreibt der Test selbst: 8x4 Pixel, 24 Bit.
; Spalten: 0-1 schwarz, 2-3 fast schwarz (1,1,1), 4-5 magenta, 6-7 rot.

Graphics 64,32,32,2
SetBuffer BackBuffer()

Function Farbe(f,x)
	If x<2 Then WriteByte f,0 : WriteByte f,0 : WriteByte f,0 : Return
	If x<4 Then WriteByte f,1 : WriteByte f,1 : WriteByte f,1 : Return
	If x<6 Then WriteByte f,255 : WriteByte f,0 : WriteByte f,255 : Return
	WriteByte f,0 : WriteByte f,0 : WriteByte f,255
End Function

f=WriteFile("bug172.bmp")
WriteByte f,66 : WriteByte f,77 : WriteInt f,54+8*4*3
WriteInt f,0 : WriteInt f,54
WriteInt f,40 : WriteInt f,8 : WriteInt f,4
WriteShort f,1 : WriteShort f,24
WriteInt f,0 : WriteInt f,8*4*3 : WriteInt f,2835 : WriteInt f,2835
WriteInt f,0 : WriteInt f,0
For y=0 To 3 : For x=0 To 7 : Farbe(f,x) : Next : Next
CloseFile f

Function Zeile$(txt$)
	LockBuffer BackBuffer()
	s$=""
	For x=0 To 7
		s=s+" "+Hex(ReadPixelFast(x,1) And $FFFFFF)
	Next
	UnlockBuffer BackBuffer()
	Return txt+":"+s
End Function

img=LoadImage("bug172.bmp")
ClsColor 0,255,0 : Cls
DrawImage img,0,0
Print Zeile("LoadImage")
ClsColor 0,255,0 : Cls
DrawBlock img,0,0
Print Zeile("DrawBlock")
MaskImage img,255,0,255
Cls
DrawImage img,0,0
Print Zeile("MaskImage magenta")
MaskImage img,0,0,0
Cls
DrawImage img,0,0
Print Zeile("MaskImage schwarz")
anim=LoadAnimImage("bug172.bmp",4,4,0,2)
Cls
DrawImage anim,0,0,0
DrawImage anim,4,0,1
Print Zeile("LoadAnimImage")
LockBuffer ImageBuffer(img)
Print "ReadPixelFast im Bild: "+Hex(ReadPixelFast(0,0,ImageBuffer(img)))+" "+Hex(ReadPixelFast(2,0,ImageBuffer(img)))
UnlockBuffer ImageBuffer(img)
LockBuffer ImageBuffer(img)
WritePixelFast 6,1,0,ImageBuffer(img)
WritePixelFast 0,1,$FF0000FF,ImageBuffer(img)
UnlockBuffer ImageBuffer(img)
Cls
DrawImage img,0,0
Print Zeile("WritePixelFast schwarz und blau")
DeleteFile "bug172.bmp"
