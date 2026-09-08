; BUG-46 - Eine Zahl hat hoechstens einen Punkt. "1..5" zerfaellt in die
; beiden Zahlen "1." und ".5"; zwei Zahlen nebeneinander sind kein Ausdruck.
; Das Original lehnt ebenfalls ab ("Expecting end-of-file", Blitz3D 11.8).
Print 1..5
