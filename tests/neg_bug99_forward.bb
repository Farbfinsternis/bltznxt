; BUG-99 - ein Const darf nur auf Consts zeigen, die VOR ihm stehen; das
; Original wertet sie in Quelltextreihenfolge aus ("Expression must be
; constant"). Vorher meldete g++ einen unbekannten Namen.
Const A = B + 1
Const B = 2
Print A
