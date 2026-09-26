; BUG-99 - ein zweites Const gleichen Namens ist im Original "Duplicate
; variable name". Vorher galt still der erste Wert.
Const c = 1
Const c = 2
Print c
