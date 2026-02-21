; Test for HidePointer and ShowPointer
Graphics 800,600,0,2
AppTitle "Mouse Pointer Test"

Print "Move mouse and press H to hide, S to show, ESC to quit."
Print "Pointer is currently visible."

While Not KeyHit(1)
	If KeyHit(35) ; H
		HidePointer
		Print "Pointer Hidden"
	EndIf
	If KeyHit(31) ; S
		ShowPointer
		Print "Pointer Shown"
	EndIf
	
	Flip
Wend
End
