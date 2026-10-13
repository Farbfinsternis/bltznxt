; BUG-11: Hex-, Binaer- und Dezimalliterale behalten die unteren 32 Bit,
; wie der Parser des Originals sie aufsammelt - ohne Fehlermeldung.
; Erwartung am Original Blitz3D V11.8 gemessen (2026-10-13).
Print 99999999999
Print $100000000
Print $123456789
Print $FFFFFFFFFFFFFFFFFF
Print %111111111111111111111111111111111
Print %100000000000000000000000000000001
Print -$80000000
Const H = $100000005
Print H
