; BUG-99 - Const hat den Typ seines Tags (ohne Tag int) und wird beim
; Uebersetzen gefaltet wie im Original (VarDeclNode::proto, exprnode.cpp).
; Die Erwartung stammt vom Original (build/const20260926).
;
; Ein Type darf ein Const benutzen, das erst spaeter deklariert wird.
Type T
Field a[N]
End Type
Local p.T = New T
p\a[N] = 9
Print p\a[2]

Const c01=1.5, c02=2.5, c03=-1.5, c04=-2.5, c05=0.5, c06=3.5, c07=0.49999
Print c01 : Print c02 : Print c03 : Print c04 : Print c05 : Print c06 : Print c07
Const c08="42", c09="abc", c10="3.7", c11=" 12x", c12="-5.9"
Print c08 : Print c09 : Print c10 : Print c11 : Print c12
Const c13=7/2, c14=7/2.0, c15=7.0/2, c16#=7/2, c17$=1.5, c18$=7/2, c19%=1.5, c20#="2.5"
Print c13 : Print c14 : Print c15 : Print c16 : Print c17 : Print c18 : Print c19 : Print c20
Const c21=Pi, c22#=Pi, c23$=Pi, c24=True, c25=False
Print c21 : Print c22 : Print c23 : Print c24 : Print c25
Const c26="a"+"b", c27$="a"+1, c28$=1+"a", c29$=1.5+"x", c30=2^10, c31#=2^0.5
Print c26 : Print c27 : Print c28 : Print c29 : Print c30 : Print c31
Const c32=10 Mod 3, c33=-7 Mod 3, c34#=7.5 Mod 2, c35=-7.5 Mod 2
Print c32 : Print c33 : Print c34 : Print c35
Const c36=1 Shl 4, c37=-16 Sar 2, c38=-16 Shr 28, c39=5 And 3, c40=5 Or 3, c41=5 Xor 3
Print c36 : Print c37 : Print c38 : Print c39 : Print c40 : Print c41
Const c42=Not 0, c43=Not 5, c44=~5, c45=-(3), c46=+2.5, c47=1.5 And 2.5
Print c42 : Print c43 : Print c44 : Print c45 : Print c46 : Print c47
Const c48=3>2, c49="a"<"b", c50=1.5=1.5, c51="10"<"9", c52=2.5>2
Print c48 : Print c49 : Print c50 : Print c51 : Print c52
Const c53=Int(1.9), c54=Int(2.5), c55=Int(-2.5), c56#=Float(7)/2, c57$=Str(1.5), c58#=Int(1.5)
Print c53 : Print c54 : Print c55 : Print c56 : Print c57 : Print c58
Const c59=Abs(-3), c60#=Abs(-2.5), c61=Sgn(-7), c62#=Sgn(0.25), c63=Abs(-2.5)
Print c59 : Print c60 : Print c61 : Print c62 : Print c63
Const c64=c01*2, c65#=c01/4, c66=1/3.0*3, c67#=1/3, c68=2147483647+1, c69=$FF, c70=%101
Print c64 : Print c65 : Print c66 : Print c67 : Print c68 : Print c69 : Print c70
Const c71$=1.0/3, c72$=c14, c73#=c72, c74="7"+1, c75$="7"+1, c76=-33/16, c77=-7/2
Print c71 : Print c72 : Print c73 : Print c74 : Print c75 : Print c76 : Print c77
Const c78=1000.0, c79#=100000000.0*100, c80$=123456789.0, c81=16777217.0, c82=3000000000.0
Print c78 : Print c79 : Print c80 : Print c81 : Print c82
Local v# = c01 : Print v
Local w% = c14 : Print w
Print c01/4 : Print c01/4.0 : Print c14*1.5 : Print c08+1 : Print c16/2
Print c01 + "x" : Print c17 + 1

; Die Consts wirken ueberall mit ihrem gefalteten Wert.
Const K=1.5, N=2.5, S0=0.5
Local vv[N]
vv[2]=7 : Print vv[2]
Function F(a=K)
Return a
End Function
Function G#(a#=K)
Return a
End Function
Function H$(a$=K)
Return a
End Function
Print F() : Print G() : Print H()
Global g#=K
Local l$=K
Print g : Print l
For i#=0 To 2 Step S0
Print "nie"
Next
Const M#=N/3, S$=N+M
Print M : Print S
Const n1=Not "0", n2=Not "", n3=Not 0.0, s1=1 Shl 33, s2=-1 Shr 33
Print n1 : Print n2 : Print n3 : Print s1 : Print s2
Const d$=-0.0
Print d

; Ein Local in einer Funktion verdeckt den Const.
Function L()
Local c01
c01 = 5
Return c01
End Function
Print L() : Print c01
