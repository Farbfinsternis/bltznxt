; BUG-19, Folgefall zu BUG-23: die Schleifenvariable wird jetzt vor der Schleife
; deklariert statt im C++-for-Kopf. Ein Goto ueber die ganze Schleife wuerde
; damit ihre Initialisierung ueberspringen, was C++ verbietet. hoistLocals()
; zieht sie deshalb an den Rumpfanfang, sobald der Rumpf ein Label enthaelt.

Goto weiter
For i = 1 To 3
Next
.weiter
Print "uebersprungen, i=" + i

For j = 1 To 3
  If j = 2 Then Goto raus
Next
.raus
Print "aus der Schleife gesprungen, j=" + j
