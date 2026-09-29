; Parst durch (Exit-Code-Pfad 2), scheitert erst in g++: Foo ist kein deklarierter Typ.
; Fixture kann veralten, sobald der Compiler das früher abfängt - der Smoke-Test
; erkennt das und überspringt den Fall (die Einsortierung prüft test/unit/diagnostics.test.js).
Function f()
	Return 1
End Function
Local o.Foo = f()
