; BUG-56 - Objektparameter "Function F(p.T)".
;
; Referenz gemessen an Blitz3D 11.8 (G:\dev\Blitz3D), einschliesslich der
; Lebensdauerfrage, vor der A-11 in ASTRA_Findings.md warnt:
;
; Ein Objektparameter ist dort ein gewoehnlicher **Wertparameter und wird
; nicht referenzgezaehlt** - anders als eine lokale oder globale
; Objektvariable. Im Assembler des Originals liest der Rumpf ihn direkt aus
; dem Stack-Slot, und "p = New T" im Rumpf schreibt mit einem schlichten
; "mov [ebp+20],eax" zurueck, ohne _bbObjStore/_bbObjRelease. Der Aufrufer
; sieht die Zuweisung also nicht. Ein roher Zeiger als C++-Parameter bildet
; das genau ab - unser Modell zaehlt ohnehin nirgends Referenzen.
;
; Weiter am Original gemessen: gemischte Parameterlisten sind erlaubt, ein
; Objektparameter darf zurueckgegeben und geloescht werden, "Null" ist ein
; zulaessiges Argument, und ein Vorgabewert ist es **nicht**
; ("Function F(p.T=Null)" -> "Expression must be constant").

Type T
  Field v
End Type

; --- 1) Feldzugriff ueber den Parameter
Function Wert(p.T)
  Return p\v
End Function

a.T = New T : a\v = 5
Print Wert(a)

; --- 2) gemischte Parameterliste
Function Misch(s$, p.T, n%)
  Return p\v + n
End Function

Print Misch("x", a, 10)

; --- 3) by-value: eine Zuweisung im Rumpf erreicht den Aufrufer nicht
Function Ersetzen(p.T)
  p = New T
  p\v = 99
End Function

Ersetzen(a)
Print a\v

; --- 4) ein Objektparameter darf zurueckgegeben werden
Function Durchreichen.T(p.T)
  Return p
End Function

b.T = Durchreichen(a)
Print b\v

; --- 5) Null ist ein zulaessiges Argument
Function IstNull(p.T)
  If p = Null Return 1
  Return 0
End Function

Print IstNull(Null)
Print IstNull(a)

; --- 6) Delete auf dem Parameter wirkt auf die Liste. Es bleibt genau ein T
;        uebrig: das "New T" aus Ersetzen() oben ist ein echtes Objekt und
;        steht weiterhin in der Liste, waehrend a geloescht wird.
Function Weg(p.T)
  Delete p
End Function

Weg(a)
n = 0
For q.T = Each T
  n = n + 1
Next
Print "verbliebene T: " + n
