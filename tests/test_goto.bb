; Milestone 11: Goto, Gosub, Return (Legacy Flow)
;
; Sprungmarken schreibt Blitz3D ausschliesslich als ".name" - gemessen am
; Original: "skip:" in eigener Zeile ergibt dort "Function 'skip' not found",
; der Bezeichner ist also ein Aufruf und der Doppelpunkt der Anweisungstrenner
; (BUG-64). Bis 2026-09-09 stand hier die Doppelpunktform und hat diese
; Fehlannahme gedeckt.
;
; Nach Goto und Gosub steht der Name **ohne** Punkt; "Goto .done" lehnt das
; Original mit "Expecting identifier" ab, und seit BUG-42 auch wir
; (neg_bug42_goto_dot.bb).

; --- Test 1: Goto vorwaerts ---
Goto skip
Print "SKIP"
.skip
Print "OK"

; --- Test 2: Goto ueber eine zweite Marke ---
Goto done
Print "SKIP2"
.done
Print "OK2"

; --- Test 3: Gosub + Return ---
Gosub greet
Print "Back from Gosub"
Goto endprog

.greet
Print "Hello from Gosub"
Return

.endprog
Print "Done"
