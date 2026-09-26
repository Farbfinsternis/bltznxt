; BUG-49 - ein Vorgabewert wird wie ein Const gefaltet und in den Typ des
; Parameters gewandelt (VarDeclNode::proto mit DECL_PARAM): Int(1.9) ist
; erlaubt und ergibt 2, 7/2 an einem Float-Parameter ist 3.0, 1.5 an einem
; String-Parameter "1.5". Die Erwartung stammt vom Original
; (build/default20260926).
Function I1(n=Int(1.9)) : Return n : End Function
Function I2(n=1.9) : Return n : End Function
Function I3(n=2.5) : Return n : End Function
Function I4(n="42") : Return n : End Function
Function I5(n=Pi) : Return n : End Function
Function I6(n=Float(3)/2) : Return n : End Function
Function I7(n=Abs(-3)) : Return n : End Function
Function I8(n=Not 0) : Return n : End Function
Function I9(n=2147483647+1) : Return n : End Function
Function I10(n="7"+1) : Return n : End Function
Function F1#(x#=7/2) : Return x : End Function
Function F2#(x#="2.5") : Return x : End Function
Function F3#(x#=Pi) : Return x : End Function
Function F4#(x#=0.1) : Return x : End Function
Function F5#(x#=Sgn(-0.5)) : Return x : End Function
Function F6#(x#=2^0.5) : Return x : End Function
Function F7#(x#=-K) : Return x : End Function
Function S1$(s$=1.5) : Return s : End Function
Function S2$(s$=7/2) : Return s : End Function
Function S3$(s$=Str(7)) : Return s : End Function
Function S4$(s$="a"+"b") : Return s : End Function
Function S5$(s$=Pi) : Return s : End Function
Function S6$(s$=K) : Return s : End Function
Function S7$(s$=1.0/3) : Return s : End Function
Function M(a, b#=K, c$=K+1) : Return a+b+Len(c) : End Function
Const K=1.5
Print I1() : Print I2() : Print I3() : Print I4() : Print I5()
Print I6() : Print I7() : Print I8() : Print I9() : Print I10()
Print F1() : Print F2() : Print F3() : Print F4() : Print F5() : Print F6() : Print F7()
Print S1() : Print S2() : Print S3() : Print S4() : Print S5() : Print S6() : Print S7()
Print M(1) : Print M(1, 0.5) : Print I1(9)
