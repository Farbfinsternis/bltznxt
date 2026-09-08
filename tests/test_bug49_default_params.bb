; BUG-49 - Vorgabewerte fuer Funktionsparameter.
;
; Referenz gemessen an Blitz3D 11.8 (G:\dev\Blitz3D). Zwei Regeln, die man
; nicht raten sollte:
;
; 1) **Pflicht ist alles bis zum LETZTEN Parameter ohne Vorgabe.** "F(a=1,b)"
;    wird angenommen, verlangt aber beide Argumente - "F(1)" ist dort
;    "Not enough parameters". Eine Vorgabe vor einem Parameter ohne Vorgabe ist
;    also erlaubt, aber nie weglassbar. Genau deshalb laesst sich die Form auf
;    C++-Vorgabeargumente abbilden, obwohl C++ sie nur am Ende erlaubt: der
;    Emitter setzt sie nur fuer den abschliessenden Lauf, und das aendert die
;    Bedeutung nicht.
;
; 2) **Der Vorgabewert muss ein konstanter Ausdruck sein.** Literal, Vorzeichen,
;    "1+1", "1 Shl 2", ein Const und die reservierten True/False/Pi gehen; eine
;    Variable meldet im Original "Expression must be constant". Ein Const darf
;    dabei hinter der Funktion stehen, die es benutzt.

; --- 1) einfache Vorgabe, weggelassen und gesetzt
Function Eins(x=1)
  Return x
End Function
Print Eins()
Print Eins(5)

; --- 2) Vorgabe nur am Ende
Function Zwei(a, b=2)
  Return a * 10 + b
End Function
Print Zwei(1)
Print Zwei(1, 7)

; --- 3) alle Parameter mit Vorgabe
Function Drei(a=1, b=2)
  Return a * 10 + b
End Function
Print Drei()
Print Drei(5)
Print Drei(5, 6)

; --- 4) Vorgabe vor einem Parameter ohne Vorgabe: erlaubt, aber beide Argumente
;        sind Pflicht (siehe neg_bug49_default_before_required.bb)
Function Vier(a=1, b)
  Return a * 10 + b
End Function
Print Vier(3, 4)

; --- 5) String und Float
Function Text$(s$ = "vorgabe")
  Return s
End Function
Print Text()

Function Komma#(f# = 1.5)
  Return f
End Function
Print Komma()

; --- 6) konstante Ausdruecke als Vorgabe
Function Ausdruck(x = 1 + 1)
  Return x
End Function
Print Ausdruck()

Function Schieben(x = 1 Shl 2)
  Return x
End Function
Print Schieben()

Function Negativ(x = -5)
  Return x
End Function
Print Negativ()

Function Wahr(x = True)
  Return x
End Function
Print Wahr()

; --- 7) ein Const als Vorgabe, hier absichtlich ERST DANACH deklariert
Function MitConst(x = GRENZE)
  Return x
End Function
Print MitConst()

Const GRENZE = 42
