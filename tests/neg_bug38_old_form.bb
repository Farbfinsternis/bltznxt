; BUG-38 - "For Each <var>" war eine Erfindung dieses Projekts. Blitz3D
; kennt die Form nicht (compiler/parser.cpp liest im FOR-Zweig erst die
; Variable, dann '=', dann EACH), also wird sie abgelehnt - mit genau
; einer Meldung, die die richtige Schreibweise nennt.

Type Punkt
  Field x%
End Type

Local a.Punkt = New Punkt
a\x = 1

For Each p.Punkt
  Print p\x
Next

Print "nach der Schleife"
