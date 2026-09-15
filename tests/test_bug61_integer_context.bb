; BUG-61: conditions and array subscripts cast to integer in Blitz3D.
; Reference: compiler/stmtnode.cpp, varnode.cpp, exprnode.cpp.
; The same program is measured with WriteLine in place of Print.
Global calls=0
Function Value$(s$)
 calls=calls+1
 Return s
End Function
Function Truth(s$)
 If s Then Return 1
 Return 0
End Function
Function FloatTruth(n#)
 If n Then Return 1
 Return 0
End Function
Print Truth("x")
Print Truth("12")
Print Truth("0")
Print Truth("")
Print Truth("0.6")
Print Truth(" -2tail")
Print FloatTruth(0.4)
Print FloatTruth(0.5)
Print FloatTruth(0.6)
Print FloatTruth(-0.5)
Print FloatTruth(-0.6)
If "x" Then
 Print "wrong"
ElseIf Value("12") Then
 Print "elseif"
Else
 Print "wrong"
EndIf
Print calls
If 0.5 Then Print "wrong" Else Print "half false"
If 0.6 Then Print "fraction true" Else Print "wrong"
Local s$="1"
Local n=0
While Value(s)
 n=n+1
 s="0"
Wend
Print n
Print calls
Repeat
 n=n+1
Until Value("1")
Print n
Print calls
; String bounds and subscripts use the integer prefix; floats round to even.
Dim a("3.9",2.5)
a("1.9","1")=11
a(1.9,1.5)=22
Print a("1", "1.9")
Print a(2,2)
Print a("x","")
Local index#=1.9
Print a(index,index)
a(2.5,0)=25
Print a(2,0)
Print a(3,0)
Print a(0,-0.5)
Dim b(1.6)
b(2)=42
Print b(2)
Local v[3]
v["1.9"]=31
v[1.9]=32
Print v["1"]
Print v[index]
Type Box
 Field values[3]
End Type
Local box.Box=New Box
box\values["1.9"]=41
box\values[index]=42
Print box\values["1"]
Print box\values[2]
; Index function runs once in a read and once in a write.
calls=0
a(Value("1"),0)=51
Print a(Value("1"),0)
Print calls
; Read and For targets also pass through the same subscript conversion.
Data 61,62
Read a("1",0),v["1"]
Print a(1,0)
Print v[1]
For a("1",0)=1 To 2
 Print a(1,0)
Next
Delete box
