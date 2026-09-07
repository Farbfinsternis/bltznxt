; BUG-39, nebenbei aufgefallen: FirstExpr und LastExpr trugen keine Spalte,
; deshalb meldete jede Diagnose ueber sie Spalte 1 statt der Stelle, an der
; "First" bzw. "Last" wirklich steht. Diese Datei haelt die Stelle fest.

Type Punkt
  Field x%
End Type

Local p.Punkt = First Punkte
Local q.Punkt = Last Punktte
