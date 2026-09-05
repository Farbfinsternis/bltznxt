; BUG-30 — in der Referenz liest der FOR-Fall den Zaehler mit parseVar(),
; derselben Funktion wie jede andere Variablenreferenz. Damit sind ein
; Array-Element und ein Typfeld gueltige Zaehler. Alle Faelle hier waren
; vorher Parse-Fehler.

Dim a(3)
For a(0) = 1 To 3
Next
Print "Array-Element danach: " + a(0)

Dim g(2, 2)
For g(1, 1) = 5 To 1 Step -2
Next
Print "zweidimensional danach: " + g(1, 1)

Type P
  Field n%
  Field f#
End Type
Local p.P = New P

For p\n = 1 To 3
Next
Print "Feld danach: " + p\n

For p\f = 1.0 To 2.0 Step 0.5
Next
Print "Float-Feld danach: " + p\f

; Der Zaehler wird bei jedem Durchlauf geschrieben, nicht erst am Ende.
Dim spur(4)
Local i% = 0
For a(1) = 1 To 3
  spur(i) = a(1)
  i = i + 1
Next
Print "Spur: " + spur(0) + " " + spur(1) + " " + spur(2)

; Ein Feld als Zaehler wird auch im Rumpf gelesen.
Local summe% = 0
For p\n = 1 To 4
  summe = summe + p\n
Next
Print "Summe ueber das Feld: " + summe
