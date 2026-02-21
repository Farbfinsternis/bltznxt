; Test Overloading support
; B3D allows Abs%() and Abs#() to coexist.

val_i% = -5
val_f# = -5.5

; These should call different implementations (or at least return correct types)
res_i% = Abs(val_i)
res_f# = Abs(val_f#)

Print "Abs(-5) (Int) = " + res_i
Print "Abs(-5.5) (Float) = " + res_f#

If res_i <> 5 Then RuntimeError "Abs(Int) failed: " + res_i
If res_f# <> 5.5 Then RuntimeError "Abs(Float) failed: " + res_f#

; Test Sgn
sgn_i% = Sgn(-10)
sgn_f# = Sgn(-10.5)

Print "Sgn(-10) = " + sgn_i
Print "Sgn(-10.5) = " + sgn_f#

If sgn_i <> -1 Then RuntimeError "Sgn(Int) failed"
If sgn_f# <> -1.0 Then RuntimeError "Sgn(Float) failed"

Print "Overloading Tests Passed"
