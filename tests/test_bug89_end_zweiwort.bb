; BUG-89 - die Gegenprobe: EndIf und ElseIf gibt es in einem Wort, die
; Abschluesse von Type, Function und Select nur als "End X" mit genau einem
; Leerzeichen (compiler/toker.cpp, BUG-57). Weil "EndType" & Co. dort
; Bezeichner sind, darf ein Programm den Namen auch als Variable benutzen.

Type Punkt
  Field x
End Type

Function Wert(p.Punkt)
  If p = Null
    Return -1
  ElseIf p\x > 0
    Return p\x
  Else If p\x = 0
    Return 0
  EndIf
  Return -2
End Function

p.Punkt = New Punkt
p\x = 5
Select Wert(p)
  Case 5 : Print "fuenf"
  Default : Print "nie"
End Select

EndType = 3
EndFunction = EndType + 1
Print "als Variable: " + EndFunction
