; Milestone 11: Goto, Gosub, Return (Legacy Flow)

; --- Test 1: Goto + colon-label on same line ---
Goto skip
Print "SKIP"
skip:
Print "OK"

; --- Test 2: Goto with dot-label syntax ---
Goto .done
Print "SKIP2"
.done
Print "OK2"

; --- Test 3: Gosub + Return ---
Gosub greet
Print "Back from Gosub"
Goto endprog

greet:
Print "Hello from Gosub"
Return

endprog:
Print "Done"
