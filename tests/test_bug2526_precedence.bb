; BUG-25 / BUG-26 — Operatorvorrang gegen die Blitz3D-Referenz
; (blitz-research/blitz3d, compiler/parser.cpp). Kette von stark nach schwach:
; Primaer -> unaer -> ^ -> * / Mod -> Shl Shr Sar -> + - -> Vergleiche
; -> And Or Xor -> Not

; BUG-25: Not ist die aeusserste Stufe
Print Not 0 And 0
Print Not 1 Or 1
Print Not (0 And 0)
Print (Not 0) And 0
Print 1 And (Not 0)  ; ohne Klammer lehnt Blitz3D es ab (BUG-41)

; BUG-26: Shl/Shr/Sar zwischen + - und * / Mod
Print 1 Shl 2 * 3
Print 8 Shr 1 * 2
Print 2 * 3 Shl 1
Print 1 + 2 Shl 3
Print -8 Shr 1
Print -8 Sar 1

; unveraendert und bereits richtig: ^ linksassoziativ, And/Or/Xor eine Stufe
Print 2 ^ 3 ^ 2
Print 1 Or 0 And 0
Print 7 Mod 4 * 2
Print 1 + 2 * 3

If Not 0 And 0 Then Print "wahr" Else Print "falsch"
