; BUG-164 - Ganzzahldivision durch 0 zur Laufzeit endet wie im Original mit
; "Integer divide by zero" (stderr, Exit 1). Vorher stuerzte das Programm stumm
; ab und verlor die schon geschriebene Zeile "vor". "nach" darf nicht kommen.
Local z% = 0
Print "vor"
Print 7 / z
Print "nach"
