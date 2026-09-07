; WEAK-17 — Gegenprobe: gueltige Aufrufe muessen durchgehen, auch die
; Ueberladungen und optionalen Parameter, an denen die alte Tabelle scheiterte.
Print Rand(1, 2) > 0
Print Rnd(1) >= 0
Print Rnd(5) >= 0
Print Rnd(1, 5) >= 1
Print Left("abc", 2)
Print Str(42)
Print Len("abc")
Print Mid("hallo", 2, 3)
Local n% = Rand(1, 1)
Print n
