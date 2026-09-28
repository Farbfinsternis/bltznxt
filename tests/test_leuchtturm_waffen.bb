; Leuchtturm Schritt 4 und 5 - Waffen, Zielscheiben und die Animationen der
; Waffenmodelle in der Platzhalter-Arena.
;
; Bindet die Spielmodule ein und feuert mit kuenstlicher Eingabe, 60 Takte
; je Sekunde, in derselben Reihenfolge wie das Spiel. Die Arena erzeugt
; samples/leuchtturm/werkzeug/arena.py, Waffen und Scheiben kenney.py.
; Laengen in Zentimetern, Zeiten in Takten.

Include "../samples/leuchtturm/karte.bb"
Include "../samples/leuchtturm/spieler.bb"
Include "../samples/leuchtturm/effekte.bb"
Include "../samples/leuchtturm/ziele.bb"
Include "../samples/leuchtturm/waffen.bb"
Include "../samples/leuchtturm/klang.bb"

Graphics3D 320,240,0,2
SetBuffer BackBuffer()
SeedRnd 4

Function C$(x#)
	Return Int(Floor(x * 100.0 + 0.5))
End Function

Function Wo$(p.Spieler)
	k = p\koerper
	Return "fuesse " + C(EntityX(k)) + "," + C(EntityY(k) - SP_HALB) + "," + C(EntityZ(k)) + " boden " + p\boden
End Function

Global hoechst#
; `takte` Takte mit fester Eingabe; springen nur im ersten Takt.
Function Lauf(p.Spieler, takte, abzug, springen = False)
	hoechst = -1000
	For t = 1 To takte
		Spieler_Takt(p, 0, 0, springen And t = 1)
		Raketen_Fliegen()
		Ziele_Takt()
		UpdateWorld
		Spieler_Nach(p)
		Raketen_Nach(p)
		Waffen_Takt(p, abzug)
		Effekte_Takt()
		y# = EntityY(p\koerper) - SP_HALB
		If y > hoechst Then hoechst = y
	Next
End Function

Function Raketen()
	n = 0
	For r.Rakete = Each Rakete
		n = n + 1
	Next
	Return n
End Function

; Ziel Nummer n (0 = das erste der Karte)
Function Scheibe.Ziel(n)
	z.Ziel = First Ziel
	For i = 1 To n
		z = After z
	Next
	Return z
End Function

; Stellen und schauen: Fuesse bei (x,y,z), Gierwinkel, Nickwinkel (+ = nach unten)
Function Hin(p.Spieler, x#, y#, z#, gier#, nick#)
	Spieler_Setzen(p, x, y, z, gier)
	Spieler_Beleben(p)          ; die eigenen Raketen schaden seit Schritt 7
	Spieler_Schauen(p, 0, nick)
	Lauf(p, 5, False)
End Function

; Warten, bis die Waffe wieder schiessen kann
Function Bereit(p.Spieler)
	Lauf(p, 1, False)
	While w_warte > 0 Or w_zustand <> 0
		Lauf(p, 1, False)
	Wend
End Function

; Waffe wechseln und warten, bis sie bereit ist
Function Nimm(p.Spieler, nr)
	Waffe_Waehlen(nr)
	Bereit(p)
End Function

Karte_Laden("samples/leuchtturm/daten/arena.glb")
Effekte_Laden()
Ziele_Laden("samples/leuchtturm/daten/ziel.glb")
p.Spieler = Spieler_Neu(EntityX(karte_spawn, True), EntityY(karte_spawn, True), EntityZ(karte_spawn, True), EntityYaw(karte_spawn, True))
Waffen_Laden(p, "samples/leuchtturm/daten")
; Dieser Test prueft die Waffen, nicht die Items: alle drei, volle Munition.
For i = 1 To W_ANZAHL
	w_besitz(i) = True : w_munition(i) = W_VOLL
Next

n = 0
For z.Ziel = Each Ziel
	n = n + 1
Next
Print "laden:     ziele " + n + " muendung " + (w_muendung(W_MG) <> w_modell(W_MG)) + " munition " + w_munition(1) + "/" + w_munition(2) + "/" + w_munition(3) + " waffe " + w_aktiv
z0.Ziel = Scheibe(0)
Print "ziel 0:    " + C(z0\x) + "," + C(z0\y) + "," + C(z0\z)

; 1) Heben: zu Beginn wird die Waffe 15 Takte gehoben, erst dann schiesst
;    sie - danach alle 6 Takte
Hin(p, 0, 0, -3, 0, 3.8)
Lauf(p, 9, True)
s1 = w_schuesse
Lauf(p, 1, True)
s2 = w_schuesse
Lauf(p, 50, True)
Print "1 heben:   nach 14 takten " + s1 + " schuesse, nach 15 " + s2 + ", nach 65 " + w_schuesse

; 2) MG aus 3 m auf die Scheibe in der Mitte: 6 Takte je Schuss
zl_treffer = 0 : w_schuesse = 0 : z0\leben = ZL_LEBEN
Lauf(p, 60, True)
Print "2 mg:      schuesse " + w_schuesse + " treffer " + zl_treffer + " leben " + z0\leben + " munition " + w_munition(W_MG)
Lauf(p, 60, True)
Print "2 mg:      leben " + z0\leben + " zerstoert " + zl_zerstoert + " weg " + (z0\weg > 0)

; 3) Die Scheibe kommt nach 5 s wieder
Lauf(p, 240, False)
w1 = (z0\weg > 0)
Lauf(p, 60, False)
Print "3 wieder:  nach 4 s weg " + w1 + ", nach 5 s weg " + (z0\weg > 0) + " leben " + z0\leben

; 4) Wechsel zur Railgun: im Takt des Wunsches beginnt das Senken, 12 Takte
;    spaeter ist die neue Waffe da, nach 15 weiteren ist sie bereit
Waffe_Waehlen(W_RAIL)
t = 0
While w_aktiv <> W_RAIL
	Lauf(p, 1, False) : t = t + 1
Wend
a = t
While w_zustand <> 0
	Lauf(p, 1, False) : t = t + 1
Wend
Print "4 wechsel: neue waffe im " + a + ". takt, bereit im " + t + ". takt, waffe " + w_aktiv

; 5) Railgun: ein Schuss zerstoert die Scheibe, der naechste erst nach 90 Takten
zl_zerstoert = 0 : w_schuesse = 0
Lauf(p, 90, True)
s1 = w_schuesse
Lauf(p, 1, True)
Print "5 rail:    zerstoert " + zl_zerstoert + " schuesse nach 90 takten " + s1 + ", nach 91 " + w_schuesse
; die zweite Kugel ging durch den leeren Platz an die Nordwand (z = 20)
Print "5 rail:    ende " + C(w_ex) + "," + C(w_ey) + "," + C(w_ez)
Lauf(p, 300, False)

; 6) Rakete an die Nordwand: 20 m in 28.9 m/s sind 0.69 s, 42 Takte
Nimm(p, W_RL)
Hin(p, 5, 0, 0, 0, 0)
w_explosionen = 0
Lauf(p, 1, True)
r1 = Raketen()
t = 1
While Raketen() > 0 And t < 200
	Lauf(p, 1, False)
	t = t + 1
Wend
Print "6 rakete:  fliegt " + r1 + " explodiert nach " + t + " takten, explosionen " + w_explosionen

; 7) Direkter Treffer auf die Scheibe in der Mitte: zerstoert
zl_zerstoert = 0
Hin(p, 0, 0, -6, 0, 2)
Bereit(p)
Lauf(p, 1, True)
Lauf(p, 30, False)
Print "7 direkt:  zerstoert " + zl_zerstoert + " raketen " + Raketen()
Lauf(p, 320, False)

; 8) Flaechenschaden: Explosion am Boden 1.5 m vor der Scheibe
zl_treffer = 0
Hin(p, 0, 0, -6, 0, 20)
Bereit(p)
Lauf(p, 1, True)
Lauf(p, 50, False)
Print "8 flaeche: treffer " + zl_treffer + " leben " + z0\leben
z0\leben = ZL_LEBEN

; 9) Sprung ohne Rakete, zum Vergleich
Hin(p, 5, 0, -15, 0, 89)
Lauf(p, 90, False, True)
Print "9 sprung:  hoechstens " + C(hoechst)

; 10) Rocket-Jump: nach unten schauen, springen und im selben Takt feuern
Hin(p, 5, 0, -15, 0, 89)
Bereit(p)
w_explosionen = 0
Lauf(p, 1, True, True)
h# = hoechst
Lauf(p, 150, False)
If hoechst < h Then hoechst = h
Print "10 rj:     hoechstens " + C(hoechst) + " explosionen " + w_explosionen + " " + Wo(p)

; 11) Rakete aus dem Stand vor die Fuesse: hebt auch ohne Sprung ab
Hin(p, 5, 0, -15, 0, 89)
Bereit(p)
Lauf(p, 1, True)
Lauf(p, 150, False)
Print "11 stand:  hoechstens " + C(hoechst)

; 12) Mit der Nase an der Wand: die Rakete explodiert sofort, der Spieler
;     wird von der Wand weggestossen
Hin(p, 0, 0, 19.5, 0, 0)
Bereit(p)
w_explosionen = 0
Lauf(p, 1, True)
Print "12 wand:   raketen " + Raketen() + " explosionen " + w_explosionen + " vz " + C(p\vz) + " vy " + C(p\vy)

; 13) Leer: die Waffe wechselt von selbst - nach 30 Takten Warten und dem
;     Wechsel, 30 + 12 + 15 Takte
Bereit(p)
w_munition(W_RL) = 0
Lauf(p, 1, True)
Lauf(p, 55, False)
a = w_zustand
Lauf(p, 2, False)
Print "13 leer:   zustand nach 56 takten " + a + ", nach 58 " + w_zustand + ";   waffe " + w_aktiv + " munition " + w_munition(W_RL)

; 14) Animationen aus der glTF-Datei (Sequenz 0 feuern, 1 heben)
w_munition(W_RL) = W_VOLL
Nimm(p, W_MG)
mg = FindChild(w_modell(W_MG), "mg")
lf = FindChild(w_modell(W_MG), "lauf")
lz# = EntityZ(lf)
Print "14 bereit: seq " + AnimSeq(w_modell(W_MG)) + " animating " + Animating(w_modell(W_MG)) + " mg y " + C(EntityY(mg)) + " pitch " + Int(EntityPitch(mg))
; ein Schuss: die Laeufe stossen in 2 Takten 2 cm zurueck und sind nach 6 wieder vorn
Lauf(p, 1, True)
Lauf(p, 3, False)
Print "14 feuern: seq " + AnimSeq(w_modell(W_MG)) + " animating " + Animating(w_modell(W_MG)) + " lauf z " + C(EntityZ(lf) - lz) + " mg z " + C(EntityZ(mg) * 10)
Lauf(p, 3, False)
Print "14 feuern: animating " + Animating(w_modell(W_MG)) + " lauf z " + C(EntityZ(lf) - lz)
; Wechsel: "heben" rueckwaerts, 6 von 12 Takten gesenkt ist halb unten
Bereit(p)
Waffe_Waehlen(W_RAIL)
Lauf(p, 7, False)
Print "14 senken: seq " + AnimSeq(w_modell(W_MG)) + " mg y " + C(EntityY(mg)) + " pitch " + Int(EntityPitch(mg))
Bereit(p)
Print "14 heben:  seq " + AnimSeq(w_modell(W_RAIL)) + " rail y " + C(EntityY(FindChild(w_modell(W_RAIL), "rail")))
; Railgun: Rueckstoss, der Lauf faehrt zurueck und dreht sich
Lauf(p, 1, True)
Lauf(p, 2, False)
Print "14 rail:   rail z " + C(EntityZ(FindChild(w_modell(W_RAIL), "rail"))) + " lauf roll " + Int(EntityRoll(FindChild(w_modell(W_RAIL), "lauf")))
End
