; BUG-153 - Alpha beim Laden einer Textur ohne Alphakanal: Flag 2 nimmt
; (R+G+B)/3, Flag 4 setzt Schwarz auf 0.
;
; Gemessen am Original (2026-09-17) ueber TextureBuffer.

Graphics3D 320,240,0,2

Function Alphas$(flags)
	t = LoadTexture("tests/assets/farben.bmp",flags)
	s$ = ""
	LockBuffer TextureBuffer(t)
	For x=0 To TextureWidth(t)-1
		p = ReadPixelFast(x,0,TextureBuffer(t))
		s = s + ((p Shr 24) And 255) + " "
	Next
	UnlockBuffer TextureBuffer(t)
	FreeTexture t
	Return s
End Function

Function Farben$(flags)
	t = LoadTexture("tests/assets/farben.bmp",flags)
	s$ = ""
	LockBuffer TextureBuffer(t)
	For x=0 To TextureWidth(t)-1
		p = ReadPixelFast(x,0,TextureBuffer(t))
		s = s + Hex(p And $FFFFFF) + " "
	Next
	UnlockBuffer TextureBuffer(t)
	FreeTexture t
	Return s
End Function

Print "flag 1: " + Alphas(1)
Print "flag 2: " + Alphas(2)
Print "flag 3: " + Alphas(3)
Print "flag 4: " + Alphas(4)
Print "flag 5: " + Alphas(5)
Print "flag 6: " + Alphas(6)
Print "flag 7: " + Alphas(7)
Print "farben 2: " + Farben(2)
Print "farben 4: " + Farben(4)
End
