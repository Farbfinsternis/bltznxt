; Leuchtturm Schritt 7 - Items, Leben, Ruestung, Schaden und Tod.
;
; Bindet die Spielmodule ein und stellt den Spieler mit kuenstlicher Eingabe
; auf die Items der Platzhalter-Arena (werkzeug/arena.py), 60 Takte je
; Sekunde. Klang bleibt aus (kein Klang_Laden). Erwartet sind die Regeln
; von Quake III (items.bb, spieler.bb, waffen.bb).

Include "../samples/leuchtturm/karte.bb"
Include "../samples/leuchtturm/spieler.bb"
Include "../samples/leuchtturm/effekte.bb"
Include "../samples/leuchtturm/ziele.bb"
Include "../samples/leuchtturm/waffen.bb"
Include "../samples/leuchtturm/klang.bb"
Include "../samples/leuchtturm/items.bb"

Graphics3D 320,240,0,2
SetBuffer BackBuffer()
SeedRnd 7

Function Lauf(p.Spieler, takte, abzug = False, springen = False)
	For t = 1 To takte
		Spieler_Takt(p, 0, 0, springen And t = 1)
		Raketen_Fliegen()
		Ziele_Takt()
		UpdateWorld
		Spieler_Nach(p)
		Raketen_Nach(p)
		Waffen_Takt(p, abzug)
		Items_Takt(p)
		Effekte_Takt()
	Next
End Function

Function Item_Von.Item(name$)
	For m.Marke = Each Marke
		If m\name = name Then Exit
	Next
	For it.Item = Each Item
		If it\x = m\x And it\y = m\y And it\z = m\z Then Return it
	Next
	Return Null
End Function

; Auf ein Item stellen: die Fuesse einen halben Meter unter seiner Mitte
Function Auf(p.Spieler, name$)
	it.Item = Item_Von(name)
	Spieler_Setzen(p, it\x, it\y - 0.5, it\z, 0)
	Lauf(p, 2)
End Function

; Weit weg von allen Items, mitten in der Arena
Function Weg(p.Spieler)
	Spieler_Setzen(p, 5, 0, -5, 0)
	Lauf(p, 2)
End Function

Function Zustand$(p.Spieler)
	Return "leben " + p\leben + " ruestung " + p\panzer + " munition " + w_munition(1) + "/" + w_munition(2) + "/" + w_munition(3) + " besitz " + w_besitz(1) + w_besitz(2) + w_besitz(3)
End Function

Karte_Laden("samples/leuchtturm/daten/arena.glb")
Effekte_Laden()
Ziele_Laden("samples/leuchtturm/daten/ziel.glb")
p.Spieler = Spieler_Neu(EntityX(karte_spawn, True), EntityY(karte_spawn, True), EntityZ(karte_spawn, True), EntityYaw(karte_spawn, True))
Waffen_Laden(p, "samples/leuchtturm/daten")
Items_Laden("samples/leuchtturm/daten/items", "samples/leuchtturm/daten")

n = 0 : Dim arten(4)
For it.Item = Each Item
	n = n + 1 : arten(it\art) = arten(it\art) + 1
Next
Print "laden:      items " + n + " waffen " + arten(IT_WAFFE) + " munition " + arten(IT_MUNITION) + " ruestung " + arten(IT_RUESTUNG) + " gesundheit " + arten(IT_GESUNDHEIT)

; 1) Start: 125 Leben, das MG mit 100 Schuss; ueber 100 klingt es ab
Weg(p)
Print "1 start:    " + Zustand(p) + " waffe " + w_aktiv
Lauf(p, 178)
Print "1 abklingen nach 3 s: leben " + p\leben + " (je 60 Takte eins)"

; 2) Raketenwerfer auf Plattform A: Waffe, 10 Raketen, gleich gewaehlt
Auf(p, "weapon_rl")
it.Item = Item_Von("weapon_rl")
Print "2 waffe:    " + Zustand(p) + " wunsch " + w_wunsch + " meldung " + it_meldung + " weg noch " + it\weg + " takte"
; nach 5 s ist sie wieder da - wer draufsteht, bekommt einen Schuss dazu
Lauf(p, it\weg)
Print "2 wieder:   nach 5 s munition " + w_munition(W_RL) + " weg " + it\weg
Lauf(p, 1)
Print "2 wieder:   einen takt spaeter munition " + w_munition(W_RL) + " weg noch " + it\weg + " takte"

; 3) Munition fuer die Railgun, ohne die Waffe zu haben
Auf(p, "ammo_rail")
Print "3 munition: " + Zustand(p)
Waffe_Waehlen(W_RAIL)
Print "3 waehlen:  railgun waehlbar ohne die waffe " + (w_wunsch = W_RAIL)

; 4) Gesundheit: die kleine bleibt liegen, solange man 100 oder mehr hat
p\leben = 100
Auf(p, "health_25")
it = Item_Von("health_25")
Print "4 voll:     leben " + p\leben + " liegt noch " + (it\weg <= 0)
Weg(p)
p\leben = 60
Auf(p, "health_25")
Print "4 genommen: leben " + p\leben + " weg noch " + it\weg + " takte"
Weg(p)
p\leben = 90
Auf(p, "health_50")
Print "4 gross:    leben " + p\leben + " (hoechstens 100)"

; 5) Die grosse Gesundheit oben auf der Saeule: bis 200
p\leben = 150
Auf(p, "health_100")
Print "5 mega:     leben " + p\leben

; 6) Ruestung: 50, 100, dann 25 - hoechstens 200
Auf(p, "armor_50")
Print "6 ruestung: " + p\panzer
Auf(p, "armor_100")
Print "6 ruestung: " + p\panzer
Auf(p, "armor_25")
Print "6 ruestung: " + p\panzer + " meldung " + it_meldung
p\panzer = 190
it = Item_Von("armor_25") : it\weg = 0 : ShowEntity it\ent
Auf(p, "armor_25")
Print "6 voll:     " + p\panzer

; 7) Schaden: die Ruestung faengt zwei Drittel ab (aufgerundet)
Weg(p)
p\leben = 100 : p\panzer = 50
Spieler_Schaden(p, 30)
Print "7 schaden 30:   leben " + p\leben + " ruestung " + p\panzer + " wunden " + p\wunden
p\panzer = 5
Spieler_Schaden(p, 30)
Print "7 wenig panzer: leben " + p\leben + " ruestung " + p\panzer

; 8) Die eigene Rakete vor die Fuesse: halber Schaden, voller Stoss
p\leben = 100 : p\panzer = 0
w_munition(W_RL) = 10 : w_besitz(W_RL) = True
Waffe_Waehlen(W_RL)
Spieler_Setzen(p, 5, 0, -5, 0)
Spieler_Schauen(p, 0, 89 - p\nick)
Lauf(p, 60)
Lauf(p, 1, True)
Lauf(p, 20)
Print "8 eigen:    leben " + p\leben + " (Quake: halbiert) vy " + Int(p\vy)
Lauf(p, 120)
Spieler_Schauen(p, 0, -p\nick)

; 9) Tod: bei 0 Leben; dann nimmt man nichts mehr auf
p\leben = 10 : p\panzer = 0
Spieler_Schaden(p, 20)
Print "9 tot:      " + p\tot + " leben " + p\leben
it = Item_Von("health_50") : it\weg = 0 : ShowEntity it\ent
Auf(p, "health_50")
Print "9 tot:      nimmt nichts, leben " + p\leben + " liegt noch " + (it\weg <= 0)
Spieler_Schaden(p, 50)
Print "9 tot:      kein weiterer schaden, leben " + p\leben

; 10) Neu erscheinen: Startwerte, nur das MG
Spieler_Beleben(p)
Waffen_Neu(p)
Print "10 neu:     " + Zustand(p) + " tot " + p\tot + " waffe " + w_aktiv
End
