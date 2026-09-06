; BUG-33: Eine Konstante gehoert dem ganzen Programm, nicht dem Anweisungsstrom
; des Hauptteils. Sie muss deshalb in jeder Funktion sichtbar sein - auch dann,
; wenn die Funktion vor der Const-Zeile steht.

Function Vorwaerts%()
  Return SPAETER
End Function

Global zaehler% = 0
Const K% = 5
Const GRUSS$ = "welt"
Const HALB# = 2.5
Const ZWEI% = K * 2
Const SPAETER% = 42

Function Hoch()
  zaehler = zaehler + K
End Function

Function Anrede$()
  Return "hallo " + GRUSS
End Function

Function Skaliert#(x#)
  Return x * HALB
End Function

Hoch()
Hoch()
Print "zaehler: " + zaehler
Print Anrede()
Print "skaliert: " + Skaliert(4)
Print "abgeleitet: " + ZWEI
Print "vorwaerts: " + Vorwaerts()
Print "im hauptteil: " + K
