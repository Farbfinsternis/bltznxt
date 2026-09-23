; 3D-10 - Aussehen: EntityAlpha, EntityColor, EntityShininess, EntityBlend,
; EntityFX, EntityAutoFade - und die Zeichenreihenfolge von EntityOrder.
;
; EntityAlpha steht in 35, EntityFX in 29, EntityColor in 25 der 130
; mitgelieferten Beispielprogramme.
;
; Alle Zahlen dahinter sind am laufenden Original nachgemessen, nicht dem
; Roadmap-Entwurf entnommen - der hatte bei EntityBlend die Modi vertauscht:
;
;   EntityBlend  1 = Alpha (Vorgabe), 2 = Multiply, 3 = Add
;   EntityFX     1 full-bright, 2 Vertexfarben, 4 flatshaded, 8 kein Nebel,
;                16 keine Rueckseitenentfernung, 32 Alphablending erzwingen
;   EntityColor  0-255, Gleitkomma, Vorgabe 255,255,255
;   EntityAlpha  0-1, Vorgabe 1; bei 0 wird gar nicht gezeichnet
;   EntityAutoFade  alpha = (far - Abstand) / (far - near), geklemmt
;
; **Was dieser Test zusichert und was nicht.** Er prueft, was die Sprache
; sehen kann: dass die Befehle angenommen werden, den Rest des Entity in Ruhe
; lassen, und dass ein nicht gezeichnetes Entity aus der Dreieckszahl
; verschwindet. Das *Aussehen* sichert er nicht zu - dafuer stehen die
; Messungen, die 34 von 34 Bildpunkten gegen das Original gehalten haben
; (siehe DEVLOG). Aus der Sprache heraus ist das Bild derzeit nicht lesbar,
; siehe BUG-63.

; Geschrieben ohne die uebliche Kurzform "UpdateWorld : RenderWorld": unser
; Parser liest einen Bezeichner vor einem Doppelpunkt als Sprungmarke und
; verschluckt den Befehl dabei stillschweigend - BUG-64, hier bewusst
; umgangen statt mitrepariert.

Graphics3D 320,240,32,2

cam = CreateCamera()
PositionEntity cam, 0, 0, -6
licht = CreateLight()
w = CreateCube()

UpdateWorld
RenderWorld
voll = TrisRendered()
If voll > 0 Then Print "grundbild" Else Print "FEHLER grundbild"

; --- 1) alle sechs Befehle laufen und lassen die Verwandtschaft in Ruhe
EntityColor     w, 255, 128, 0
EntityAlpha     w, 0.5
EntityShininess w, 0.75
EntityBlend     w, 3
EntityFX        w, 1 + 8
EntityAutoFade  w, 5, 10
If EntityClass$(w) = "Mesh" Then Print "klasse" Else Print "FEHLER klasse"
If EntityX(w) = 0 And EntityY(w) = 0 Then Print "position" Else Print "FEHLER position"

; --- 2) EntityColor nimmt Nachkommastellen an. Das Original fuehrt
;        "EntityColor entity,red#,green#,blue#" - mit Integer-Parametern
;        waere 30.5 stillschweigend zu 30 geworden (vgl. BUG-62).
EntityColor w, 30.5, 200.25, 0
Print "farbe gleitkomma"

; --- 3) ein durchscheinendes Entity laeuft durch den Renderer
UpdateWorld
RenderWorld
If TrisRendered() = voll Then Print "durchscheinend" Else Print "FEHLER durchscheinend"

; --- 4) Alpha 0 wird laut Doku gar nicht gezeichnet. Anders als HideEntity
;        bleibt das Entity dabei sichtbar im Sinne von ShowEntity/HideEntity.
EntityAutoFade w, 0, 0
EntityAlpha w, 0
UpdateWorld
RenderWorld
If TrisRendered() = 0 Then Print "alpha 0 nicht gezeichnet" Else Print "FEHLER alpha 0 nicht gezeichnet"

EntityAlpha w, 1
UpdateWorld
RenderWorld
If TrisRendered() = voll Then Print "alpha 1 wieder da" Else Print "FEHLER alpha 1 wieder da"

; --- 5) AutoFade: jenseits von far ist das Entity unsichtbar, diesseits von
;        near voll da. Die Kamera steht 6 Einheiten entfernt.
EntityAutoFade w, 1, 2
UpdateWorld
RenderWorld
If TrisRendered() = 0 Then Print "autofade fern" Else Print "FEHLER autofade fern"

EntityAutoFade w, 20, 40
UpdateWorld
RenderWorld
If TrisRendered() = voll Then Print "autofade nah" Else Print "FEHLER autofade nah"
EntityAutoFade w, 0, 0

; --- 6) EntityFX 16 schaltet die Rueckseitenentfernung ab, ohne die
;        Dreieckszahl zu aendern - gezeichnet wird beides Mal alles.
EntityFX w, 16
UpdateWorld
RenderWorld
If TrisRendered() = voll Then Print "fx 16" Else Print "FEHLER fx 16"
EntityFX w, 0

; --- 7) EntityOrder: die Reihenfolge aendert das Bild, nicht die Zahl der
;        Dreiecke. Geprueft wird, dass beide Richtungen durchlaufen.
z = CreateCube()
PositionEntity z, 0, 0, 2
UpdateWorld
RenderWorld
beide = TrisRendered()
If beide = voll * 2 Then Print "zwei wuerfel" Else Print "FEHLER zwei wuerfel"

EntityOrder z, -1
UpdateWorld
RenderWorld
If TrisRendered() = beide Then Print "order -1" Else Print "FEHLER order -1"
EntityOrder z, 1
UpdateWorld
RenderWorld
If TrisRendered() = beide Then Print "order +1" Else Print "FEHLER order +1"
EntityOrder z, 0

; --- 8) HideEntity nimmt das Entity ebenfalls aus dem Bild
HideEntity z
UpdateWorld
RenderWorld
If TrisRendered() = voll Then Print "versteckt" Else Print "FEHLER versteckt"
ShowEntity z

; --- 9) alle drei Blendmodi laufen durch
For b = 1 To 3
  EntityBlend w, b
  UpdateWorld
  RenderWorld
Next
If TrisRendered() = beide Then Print "blendmodi" Else Print "FEHLER blendmodi"

; Ungueltige Handles waren hier bis 2026-09-23 als folgenlos festgehalten -
; nie am Original gemessen. Seit BUG-170 beenden sie das Programm mit der
; Meldung des Debug-Modus; das pruefen die Tests test_bug170_*.

Print "fertig"
