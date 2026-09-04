; WEAK-14 — der semantische Pass muss einen widersprechenden Type-Tag melden
; (Blitz3D: "Variable type mismatch", varnode.cpp). Vorher lief das stumm
; durch und zerstoerte den String (BUG-21).
Local a$ = "hallo"
a% = 5
Print a
