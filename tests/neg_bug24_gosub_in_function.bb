; BUG-24 — Gosub ist ein Konstrukt des Hauptteils: ein blankes Return in einer
; Funktion kehrt aus der Funktion zurueck (Meilenstein 11), ein Unterprogramm
; darin koennte seinen Ruecksprung also gar nicht ausdruecken. Muss eine klare
; Diagnose geben statt nicht uebersetzbaren C++-Code.
Function F%(x%)
  If x > 0 Then
    Gosub Inner
  End If
  Return 1
  .Inner
  Print "inner"
  Return
End Function

Print F(1)
