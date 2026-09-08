; 3D-12 · Lichter — CreateLight, LightColor, LightRange, LightConeAngles,
; AmbientLight.
;
; CreateLight ist mit 39 betroffenen Beispieldateien der haeufigste fehlende
; Einzelbefehl gewesen. Alle Angaben stammen aus der mitgelieferten
; Blitz3D-Dokumentation (help/commands/3d_commands/), nicht aus Vermutungen:
;
;   CreateLight ( [type][,parent] )   1 = directional (Vorgabe), 2 = point,
;                                     3 = spot
;   LightColor light,red#,green#,blue#     0-255, negativ verdunkelt
;   LightRange light,range#                Vorgabe 1000.0
;   LightConeAngles light,inner#,outer#    Vorgabe 0,90
;   AmbientLight red#,green#,blue#         Vorgabe 127,127,127
;
; Zwei Dinge, die man ohne die Doku falsch gemacht haette: die Vorgabe ist
; **directional**, nicht point, und die Nummerierung beginnt bei 1 - der
; Shader zaehlt intern ab 0.
;
; **Was dieser Test zusichert und was nicht.** Geprueft werden die
; sprachsichtbaren Zusagen: Handles, Entity-Verhalten, Elternbindung,
; Positionsregel und dass ein beleuchtetes Bild ohne Absturz durchlaeuft.
; Das *Aussehen* wird nicht zugesichert - dafuer waere ein Bildvergleich mit
; dem Original noetig, und der ist hier nicht moeglich.

Graphics3D 320,240,32,2
AmbientLight 40, 40, 50.5

cam = CreateCamera()
PositionEntity cam, 0, 0, -6
wuerfel = CreateCube()

; --- 1) die drei Typen, jeder ein echtes Entity der Klasse "Light"
l1 = CreateLight()          ; Vorgabe: directional
l2 = CreateLight(2)         ; point
l3 = CreateLight(3)         ; spot

If l1 <> 0 And l2 <> 0 And l3 <> 0 Then Print "handles" Else Print "FEHLER handles"
If l1 <> l2 And l2 <> l3 Then Print "verschieden" Else Print "FEHLER verschieden"
If EntityClass$(l1) = "Light" Then Print "klasse" Else Print "FEHLER klasse"
If EntityClass$(l3) = "Light" Then Print "klasse spot" Else Print "FEHLER klasse spot"

; --- 2) Lichter sind gewoehnliche Entities: Position und Drehung wirken
PositionEntity l2, 3, 4, -2
If EntityX(l2) = 3 And EntityY(l2) = 4 Then Print "position" Else Print "FEHLER position"
RotateEntity l1, 45, 30, 0
If EntityPitch(l1) > 44 And EntityPitch(l1) < 46 Then Print "drehung" Else Print "FEHLER drehung"

; --- 3) Elternbindung. Laut Doku entsteht das Licht trotz Elternknoten bei
;        0,0,0 und nicht an der Position des Elternknotens.
piv = CreatePivot()
PositionEntity piv, 10, 10, 10
kind = CreateLight(2, piv)
If GetParent(kind) = piv Then Print "eltern" Else Print "FEHLER eltern"
If CountChildren(piv) = 1 Then Print "kinderzahl" Else Print "FEHLER kinderzahl"
If EntityX(kind) = 0 And EntityY(kind) = 0 Then Print "eltern-position" Else Print "FEHLER eltern-position"

; --- 4) die Einstellbefehle laufen und lassen das Entity unangetastet
LightColor l1, 255, 200, 150
LightColor l3, 0, 0, 255
LightRange l2, 20
LightConeAngles l3, 20, 60
If EntityX(l2) = 3 Then Print "unveraendert" Else Print "FEHLER unveraendert"

; negatives Licht ist laut Doku ausdruecklich erlaubt
LightColor l2, -255, -255, -255
Print "negativ ok"

; --- 5) ein beleuchtetes Bild laeuft durch
For i = 1 To 3
  UpdateWorld
  RenderWorld
  Flip
Next
If TrisRendered() > 0 Then Print "gerendert" Else Print "FEHLER gerendert"

Print "fertig"
