; DebugLog - ohne Debugger keine sichtbare Wirkung, das Programm laeuft weiter.
;
; Gemessen am Original (2026-09-17): ein eigenstaendiges Programm gibt nichts
; aus und laeuft weiter; eine Zahl wird wie bei jedem String-Parameter
; umgewandelt. BLTZNXT schreibt die Zeile auf stderr, stdout bleibt gleich.

Print "vorher"
DebugLog "Meldung"
DebugLog 42
DebugLog 1.5 + 1
x$ = "Text " + 7
DebugLog x
Print "nachher"
