; Test Operators
a = 1
b = 2
c = 3
zero = 0

Print "1 And 2 = " + (a And b)
Print "1 Or 2 = " + (a Or b)
Print "1 Xor 3 = " + (a Xor c)

Print "Not 0 = " + (Not zero)
Print "Not -1 = " + (Not -1)
Print "Not 1 = " + (Not 1)

sl = 1 Shl 2
sr = 4 Shr 1
sar_res = -4 Shr 1 ; Blitz3D 'Shr' is usually arithmetic for signed ints? Or is 'Sar' explicit?
                   ; B3D only has 'Shr', which is arithmetic for signed in standard B3D?
                   ; Actually B3D docs say Shr is logical shift? Let's check.
                   ; But transpiler has SAR token?

Print "1 Shl 2 = " + sl
Print "4 Shr 1 = " + sr
Print "-4 Shr 1 = " + sar_res
