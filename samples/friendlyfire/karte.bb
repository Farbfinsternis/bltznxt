; Friendly Fire - die Karte.
;
; Die Karte ist eine .glb aus Blender. Was das Spiel ueber sie wissen muss,
; steht in den Objektnamen (FRIENDLY_FIRE.md, "Die Karte in Blender"):
;
;   ...-col   unsichtbare Kollisionsgeometrie
;   spawn     leeres Objekt: Startpunkt und Blickrichtung
;   ziel      leeres Objekt: dort schwebt eine Zielscheibe
;
; Die anderen leeren Objekte landen als Marke (Name, Ort, Gierwinkel) in
; einer Liste; die Teile des Spiels, die sie brauchen, gehen sie durch.
;
; Blender haengt an Kopien ".001", ".002" an; gelesen wird nur der Teil vor
; dem Punkt. Hat eine Karte gar keine "-col"-Objekte, kollidiert die
; sichtbare Geometrie - so laesst sich ein schneller Export sofort
; ausprobieren.

Const TYP_SPIELER = 1
Const TYP_WELT    = 2
Const TYP_RAKETE  = 3
Const TYP_ZIEL    = 4

Type Marke
	Field name$
	Field x#, y#, z#, gier#
End Type

Global karte_wurzel
Global karte_spawn          ; Entity des Startpunkts, 0 = keiner
Global karte_col_zahl

; Der Name ohne Blenders ".001" und klein geschrieben.
Function Karte_Name$(e)
	n$ = EntityName(e)
	p = Instr(n, ".")
	If p Then n = Left(n, p - 1)
	Return Lower(n)
End Function

Function Karte_Laden(datei$)
	karte_wurzel = LoadAnimMesh(datei)
	If karte_wurzel = 0 Then RuntimeError "Karte nicht lesbar: " + datei
	karte_spawn = 0
	karte_col_zahl = 0
	Delete Each Marke
	Karte_Knoten(karte_wurzel)
	If karte_col_zahl = 0 Then Karte_Alles_Kollidiert(karte_wurzel)
	Return karte_wurzel
End Function

Function Karte_Knoten(e)
	n$ = Karte_Name(e)
	If Right(n, 4) = "-col"
		; Unsichtbar, aber nicht versteckt: HideEntity nimmt es auch aus der
		; Kollision.
		EntityType e, TYP_WELT
		EntityPickMode e, 2         ; fuer die Bodenpruefung des Spielers
		EntityAlpha e, 0
		karte_col_zahl = karte_col_zahl + 1
	ElseIf n = "spawn"
		If karte_spawn = 0 Then karte_spawn = e
	ElseIf e <> karte_wurzel And CountSurfaces(e) = 0 And n <> ""
		m.Marke = New Marke
		m\name = n
		m\x = EntityX(e, True) : m\y = EntityY(e, True) : m\z = EntityZ(e, True)
		m\gier = EntityYaw(e, True)
	EndIf
	For i = 1 To CountChildren(e)
		Karte_Knoten(GetChild(e, i))
	Next
End Function

Function Karte_Alles_Kollidiert(e)
	If EntityClass(e) = "Mesh"
		If CountSurfaces(e) > 0 Then EntityType e, TYP_WELT : EntityPickMode e, 2
	EndIf
	For i = 1 To CountChildren(e)
		Karte_Alles_Kollidiert(GetChild(e, i))
	Next
End Function
