; BUG-164 - ein Feldzugriff ueber ein Null-Objekt endet wie im Original mit
; "Memory access violation" (seTranslator in bbruntime_dll.cpp), nicht mit
; einem stummen Absturz. "vor" muss erhalten bleiben, "nach" darf nicht kommen.
Type T
	Field a
End Type
Local t.T
Print "vor"
Print t\a
Print "nach"
