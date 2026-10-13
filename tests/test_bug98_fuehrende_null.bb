; BUG-98: Ganzzahl-Literale sind immer dezimal wie atoi im Original -
; "010" ist 10, nicht oktal 8, und "08" ist gueltig. Auch in Const, Data,
; Case, For, Array-Index und vor einem Punkt. Was nicht in 32 Bit passt,
; laeuft um (BUG-11). Erwartung am Original Blitz3D V11.8 gemessen (2026-10-13).
Const C1 = 010
Const C2 = 09 + 1
Dim a(20)
a(010) = 5
Data 010, 0099, -08
Print 010
Print 08
Print 0
Print 000
Print -010
Print 007 * 2
Print C1
Print C2
Print a(10)
Read d1, d2, d3
Print d1
Print d2
Print d3
x = 012
Print x
Print 010.5
Print 08.25
Print 00.5
Print 010 + 0.5
Print "a" + 010
Print 2147483647
Print 2147483648
Print 4294967297
Print -2147483648
Print 0000000000000000012
k = 011
Select k
	Case 011
		Print "elf"
	Default
		Print "anders"
End Select
For i = 08 To 010 Step 02
	Print i
Next
Print Abs(-010)
Print 010 Shl 01
