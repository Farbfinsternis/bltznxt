; BUG-85 - `Read` nimmt eine Liste, und ein Ziel ist alles, was auch links
; von einem "=" stehen darf.
;
; Referenz (`compiler/parser.cpp:288`):
;
;     case READ:
;         do{ toker->next(); VarNode *var=parseVar();
;             stmts->push_back( d_new ReadNode( var ) );
;         }while( toker->curr()==',' );
;
; Daraus folgen die beiden Dinge, die hier geprueft werden. Erstens ist
; `Read a,b,c` **drei** Anweisungen - genau wie `Read a : Read b : Read c`.
; Zweitens liest das Ziel derselbe `parseVar()` wie in einer Zuweisung, also
; sind `Read arr(1)` und `Read p\feld` gueltig. Wir kannten bis hierher nur
; `Read name` und meldeten bei allem anderen `unexpected token ','`.
;
; Alle Werte am laufenden Original gemessen (2026-09-11).

Type T
	Field v
	Field name$
	Field w
End Type

Global p.T = New T
Dim arr(5)

; --- 1) Die schlichte Liste ---
Read a, b, c
If a = 1 And b = 2 And c = 3 Then Print "liste" Else Print "FEHLER liste"

; --- 2) Liste mit Tags und gemischten Typen ---
Read x%, y#, s$
If x = 10 And y > 2.49 And y < 2.51 And s = "hallo" Then Print "tags" Else Print "FEHLER tags"

; --- 3) Das Tag wandelt, wie bei einer Zuweisung ---
;
; In der Data-Zeile steht eine Zahl, gelesen wird eine Zeichenkette.
Read t$
If t = "42" Then Print "zahl als kette" Else Print "FEHLER zahl als kette"

; Und umgekehrt: was keine Zahl ist, wird zu 0 - ohne Meldung und ohne
; Abbruch. Im Original macht das atoi; ein std::stoi wuerde hier eine
; Ausnahme werfen (derselbe Unterschied wie in BUG-82, eine Ebene tiefer).
Read k1, k2
If k1 = 0 And k2 = 0 Then Print "kette in zahl" Else Print "FEHLER kette in zahl"
Read k3#
If k3 > 1.49 And k3 < 1.51 Then Print "kette in kommazahl" Else Print "FEHLER kette in kommazahl"

; --- 4) Ein Arrayelement als Ziel, auch in einer Liste ---
Read arr(0), arr(1)
If arr(0) = 7 And arr(1) = 8 Then Print "array" Else Print "FEHLER array"

; Der Index darf gerechnet sein.
i = 1
Read arr(i + 2)
If arr(3) = 55 Then Print "array mit ausdruck" Else Print "FEHLER array mit ausdruck"

; --- 5) Ein Feld als Ziel ---
Read p\v, p\name
If p\v = 99 And p\name = "feldname" Then Print "feld" Else Print "FEHLER feld"

; --- 6) Gemischt in einer Zeile, von links nach rechts ---
Read m, arr(4), p\w
If m = 100 And arr(4) = 200 And p\w = 300 Then Print "gemischt" Else Print "FEHLER gemischt"

; --- 7) Die Liste liest in der Reihenfolge, in der sie dasteht ---
Read e1$, e2$
If e1 = "erst" And e2 = "dann" Then Print "reihenfolge" Else Print "FEHLER reihenfolge"

; --- 8) Dieselbe Wirkung wie einzelne Reads ---
;
; Die Gegenprobe auf die Zusicherung oben: drei einzelne Reads holen dieselben
; drei Werte wie eine Liste.
Read g1, g2, g3
Restore vergleich
Read h1
Read h2
Read h3
If g1 = h1 And g2 = h2 And g3 = h3 Then Print "liste gleich einzeln" Else Print "FEHLER liste gleich einzeln"

; --- 9) In einer Funktion, auf lokale Variablen ---
f_ergebnis = LiesDrei()
If f_ergebnis = 60 Then Print "in funktion" Else Print "FEHLER in funktion"

Print "fertig"
End

Function LiesDrei()
	Local u, v, w
	Read u, v, w
	Return u + v + w
End Function

Data 1,2,3
Data 10,2.5,"hallo"
Data 42
Data "erst",""
Data "1.5x"
Data 7,8
Data 55
Data 99,"feldname"
Data 100,200,300
Data "erst","dann"
.vergleich
Data 11,22,33
Data 10,20,30
