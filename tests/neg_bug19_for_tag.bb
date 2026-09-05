; BUG-19 — die Schleifenvariable ist eine gewoehnliche Variable, also gilt fuer
; sie auch die Regel aus BUG-21: ein widersprechender Type-Tag ist ein Fehler.
Global zaehler% = 1
For zaehler$ = 1 To 3
Next
