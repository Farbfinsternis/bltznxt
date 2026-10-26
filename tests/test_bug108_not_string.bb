; BUG-108 - Not auf einem String vergleicht mit "0".
;
; Das Original baut Not als Vergleich mit 0 (parser.cpp, RelExprNode), bei
; einem String also als Stringvergleich: Not "0" ist 1, Not "" und Not "abc"
; 0. Gemessen an Blitz3D 11.8 (build/bug108_20261025/not): genau diese
; Ausgabe. Vorher endete Not auf einem String in einem C++-Fehler.
Print Not "0"
Print Not ""
Print Not "abc"
Print Not "00"
s$ = "0" : Print Not s
Print Not 0
Print Not 5
Print Not 0.0
Print Not 0.4
x# = 0 : Print Not x
Print Not Sin(0)
Print Not (1 = 2)
If Not s Then Print "ja"
