; TFormPoint / TFormVector / TFormNormal und die drei Getter.
;
; Referenz (bbruntime/bbblitz3d.cpp:1608): alle drei schreiben in denselben
; Merker `tformed`, den TFormedX/Y/Z auslesen. Die Reihenfolge ist immer
; dieselbe - erst mit der Quelle in den Weltraum, dann mit der **Inversen**
; des Ziels hinein; Handle 0 heisst Weltraum und tut nichts:
;
;   TFormPoint    src->getWorldTform()  * v    volle Transform, mit Verschiebung
;   TFormVector   src->getWorldTform().m * v   nur 3x3, ein Vektor wird nicht verschoben
;   TFormNormal   (...m).cofactor()     * v    Kofaktormatrix, danach normalisiert
;
; **Die Doku ist bei TFormNormal falsch.** Sie sagt "exactly the same as
; TFormVector but with one added feature" (normalisiert). Der Quelltext nimmt
; die Kofaktormatrix - det(M) * (M^-1)^T -, und sobald ungleichmaessig
; skaliert wird, trennen sich die beiden. Genau das misst Abschnitt 3.
;
; Alle Werte am laufenden Original gemessen (2026-09-11).

Graphics3D 640, 480, 16, 2
SetBuffer BackBuffer()

; Verschoben, gegiert und **ungleichmaessig** skaliert - nur so wird der
; Unterschied zwischen Vektor und Normale sichtbar.
e = CreatePivot()
PositionEntity e, 10, 20, 30
RotateEntity   e, 0, 90, 0
ScaleEntity    e, 1, 2, 4

b = CreatePivot()
PositionEntity b, 1, 1, 1

UpdateWorld

; --- 1) TFormPoint: ein Punkt traegt die Verschiebung mit ---
TFormPoint 0, 1, 0, e, 0
If Abs(TFormedX#() - 10) < 0.001 And Abs(TFormedY#() - 22) < 0.001 And Abs(TFormedZ#() - 30) < 0.001 Then Print "punkt lokal nach welt" Else Print "FEHLER punkt lokal nach welt"

; Der Rueckweg trifft wieder den Ausgangspunkt.
TFormPoint 10, 22, 30, 0, e
If Abs(TFormedX#()) < 0.001 And Abs(TFormedY#() - 1) < 0.001 And Abs(TFormedZ#()) < 0.001 Then Print "punkt welt nach lokal" Else Print "FEHLER punkt welt nach lokal"

; Von einer Entity in eine andere, ohne Umweg ueber die Sprache.
TFormPoint 0, 1, 0, e, b
If Abs(TFormedX#() - 9) < 0.001 And Abs(TFormedY#() - 21) < 0.001 And Abs(TFormedZ#() - 29) < 0.001 Then Print "punkt e nach b" Else Print "FEHLER punkt e nach b"

; Handle 0 auf beiden Seiten heisst Weltraum - nichts geschieht.
TFormPoint 5, 6, 7, 0, 0
If Abs(TFormedX#() - 5) < 0.001 And Abs(TFormedY#() - 6) < 0.001 And Abs(TFormedZ#() - 7) < 0.001 Then Print "punkt welt nach welt" Else Print "FEHLER punkt welt nach welt"

; --- 2) TFormVector: dieselbe Drehung und Skalierung, aber ohne Verschiebung ---
;
; Derselbe Aufruf wie oben, nur als Vektor: (0,1,0) wird von der Skalierung
; auf (0,2,0) gestreckt und bleibt sonst, wo es ist - die 10,20,30 der Entity
; spielen keine Rolle.
TFormVector 0, 1, 0, e, 0
If Abs(TFormedX#()) < 0.001 And Abs(TFormedY#() - 2) < 0.001 And Abs(TFormedZ#()) < 0.001 Then Print "vektor ohne verschiebung" Else Print "FEHLER vektor ohne verschiebung"

; Die x-Achse der Entity zeigt nach einer Gierung um 90 Grad nach +z.
TFormVector 1, 0, 0, e, 0
If Abs(TFormedZ#() - 1) < 0.001 And Abs(TFormedX#()) < 0.001 Then Print "vektor x nach z" Else Print "FEHLER vektor x nach z"

; Und die z-Achse nach -x, mal der Skalierung 4. Das ist zugleich eine
; unabhaengige Gegenprobe auf die Drehrichtung aus BUG-84.
TFormVector 0, 0, 1, e, 0
If Abs(TFormedX#() + 4) < 0.001 And Abs(TFormedZ#()) < 0.001 Then Print "vektor z nach minus x" Else Print "FEHLER vektor z nach minus x"

; Rueckwaerts kehrt sich die Skalierung um: aus 1 wird 0.5.
TFormVector 0, 1, 0, 0, e
If Abs(TFormedY#() - 0.5) < 0.001 Then Print "vektor rueckweg" Else Print "FEHLER vektor rueckweg"

; --- 3) TFormNormal ist NICHT TFormVector mit Normalisierung ---
;
; Gemessen: TFormNormal 1,1,0 liefert (0, 0.447, 0.894); derselbe Vektor durch
; TFormVector und von Hand normalisiert liefert (0, 0.894, 0.447). Die beiden
; mittleren Werte sind vertauscht - das ist der ganze Unterschied zwischen der
; Matrix und ihrer Kofaktormatrix, und die Doku behauptet, es gebe ihn nicht.
TFormNormal 1, 1, 0, e, 0
nx# = TFormedX#() : ny# = TFormedY#() : nz# = TFormedZ#()
If Abs(ny - 0.447) < 0.002 And Abs(nz - 0.894) < 0.002 Then Print "normale kofaktor" Else Print "FEHLER normale kofaktor"

TFormVector 1, 1, 0, e, 0
vx# = TFormedX#() : vy# = TFormedY#() : vz# = TFormedZ#()
l# = Sqr(vx*vx + vy*vy + vz*vz)
If Abs(vy/l - 0.894) < 0.002 And Abs(vz/l - 0.447) < 0.002 Then Print "vektor von hand normiert" Else Print "FEHLER vektor von hand normiert"

If Abs(ny - vy/l) > 0.1 Then Print "normale ist nicht vektor" Else Print "FEHLER normale ist nicht vektor"

; Eine Normale hat Laenge 1, auch wenn die Eingabe laenger war.
TFormNormal 0, 3, 0, e, 0
ll# = Sqr(TFormedX#()*TFormedX#() + TFormedY#()*TFormedY#() + TFormedZ#()*TFormedZ#())
If Abs(ll - 1) < 0.001 Then Print "normale hat laenge eins" Else Print "FEHLER normale hat laenge eins"

; --- 4) Durch eine Hierarchie hindurch ---
;
; Das Kind erbt die Drehung des Elternteils. Hier zaehlt, dass die
; Umrechnung die **Elternkette** heranzieht und nicht nur die eigene Lage.
vater = CreatePivot()
PositionEntity vater, 0, 5, 0
RotateEntity   vater, 0, 90, 0
kind = CreatePivot(vater)
PositionEntity kind, 0, 0, 2
UpdateWorld
TFormPoint 0, 0, 0, kind, 0
If Abs(TFormedX#() + 2) < 0.001 And Abs(TFormedY#() - 5) < 0.001 Then Print "kind durch die kette" Else Print "FEHLER kind durch die kette"

; --- 5) Ohne UpdateWorld ---
;
; Im Original rechnet getWorldTform() verzoegert nach, sobald jemand liest -
; ein frisches PositionEntity wirkt also sofort. Bei uns schreibt sonst nur
; UpdateWorld die Weltmatrix (BUG-71), deshalb frischen die TForm-Befehle die
; Elternkette selbst auf. Ohne das rechnete diese Zusicherung mit der Lage
; von vorhin.
spaet = CreatePivot()
PositionEntity spaet, 7, 8, 9
TFormPoint 0, 0, 0, spaet, 0
If Abs(TFormedX#() - 7) < 0.001 And Abs(TFormedZ#() - 9) < 0.001 Then Print "ohne updateworld" Else Print "FEHLER ohne updateworld"

Print "fertig"
