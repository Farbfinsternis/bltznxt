; BUG-111 - Hex und Bin liefern immer alle 32 Bit, mit fuehrenden Nullen.
;
; Referenz: bbHex/bbBin in bbruntime/bbstring.cpp fuellen einen festen Puffer
; Stelle fuer Stelle - acht Hex- bzw. 32 Binaerziffern, Grossbuchstaben, kein
; Praefix. Negative Werte sind die 32 Bit des Zweierkomplements. Alle Werte
; sind am laufenden Original gemessen (2026-09-16).

Print Hex(0)
Print Hex(1)
Print Hex(255)
Print Hex(65535)
Print Hex(-1)
Print Hex(-255)
Print Hex($80000000)
Print Hex(2147483647)

Print Bin(0)
Print Bin(1)
Print Bin(5)
Print Bin(-1)
Print Bin($80000000)

; Die feste Breite ist beobachtbar: Laenge und Zusammensetzung
Print Len(Hex(7)) + " " + Len(Bin(7))
Print "0x" + Hex(255)
