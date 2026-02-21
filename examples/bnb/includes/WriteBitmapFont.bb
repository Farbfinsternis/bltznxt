; WriteBitmapFont
Function WriteBitmapFont(x,y,txt$)
	lang=Len(txt$)
	For i=1 To lang
		t0$=Mid$(txt$,i,1)
		For j=1 To font_len
			t1$=Mid$(font$,j,1)
			If t0$=t1$
				DrawImage fontimg,x,y,j-1
				x=x+char_w
				Exit
			EndIf
		Next
	Next
End Function