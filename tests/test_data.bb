; Milestone 12: Data, Read, Restore
;
; In Blitz3D ALL Data statements form ONE sequential pool.
; Read pulls the next value; Restore resets the pointer.

; --- Pool layout: 10, 20, 30, "Hello", "World", 1, 2 ---
Data 10, 20, 30
Data "Hello", "World"
Data 1, 2

; Test 1 (Roadmap): Read two ints, sum them
Read a
Read b
Print a + b         ; -> 30

; Test 2: Read the third int
Read c
Print c             ; -> 30

; Test 3: Read two strings
Read s$
Read t$
Print s$ + " " + t$ ; -> Hello World

; Test 4: Restore resets pointer to 0
Restore
Read x
Print x             ; -> 10 (first element again)

; Test 5: Explicit type hint
Restore
Read n%
Read m%
Print n% + m%       ; -> 30 (10 + 20)
