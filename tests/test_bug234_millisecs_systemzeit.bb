; BUG-234 - MilliSecs zaehlt seit dem Systemstart, nicht seit Programmstart.
;
; Das Original gibt timeGetTime() zurueck (gemessen 2026-11-16: 92756990 bei
; gut 25 Stunden Laufzeit des Rechners). BLTZNXT lieferte die Zeit seit
; Programmstart, also direkt am Anfang 0 - SeedRnd MilliSecs() ergab bei
; jedem Start fast dieselbe Zufallsfolge. Annahme des Tests: der Rechner
; laeuft schon laenger als eine Minute. Abstaende muessen weiter stimmen.
t0 = MilliSecs()
Print t0 > 60000 Or t0 < 0
Delay 100
d = MilliSecs() - t0
Print d >= 90 And d < 1000
