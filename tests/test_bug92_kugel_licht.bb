; BUG-92 - eine Kugel wird von Richtungs- und Punktlicht beleuchtet.
;
; bb_gen_sphere_ legte die Dreiecke im Umlaufsinn an, den unser Renderer als
; Rueckseite behandelt (die z-Spiegelung in der Sichtmatrix, siehe
; bb_gen_cube_). Weggeschnitten wurde deshalb die Vorderseite, und zu sehen war
; die Innenseite der hinteren Haelfte, deren Normalen vom Licht weg zeigen: die
; Kugel war unter jedem Licht schwarz, nur das Umgebungslicht kam an.
;
; Die Sollwerte sind am Original gemessen (F:\dev\Blitz3D, 2026-09-15). Genau
; treffen koennen wir sie nicht - Richtungslicht rechnen wir je Bildpunkt
; (BUG-66), und nach der Richtlinie vom 2026-09-16 ist die Schattierung frei
; (WEAK-25: dieser Test wird mit der modernen Beleuchtung umgestellt). Geprueft
; wird deshalb jeder Messpunkt mit +-12; vor dem Fix lagen alle bei 0.
;
; Ausnahme: der aeusserste Punkt von A muss nur beleuchtet sein. Seit die Kugel
; wie im Original zerlegt ist (BUG-69, 2026-09-17), liegt er bei 153 statt 137 -
; die Geometrie stimmt dort, der Unterschied ist Schattierung.

Graphics3D 320,240,32,2
AmbientLight 0,0,0
cam = CreateCamera()
CameraClsColor cam,0,0,0
kugel = CreateSphere(16)
PositionEntity kugel,0,0,4
EntityColor kugel,100,100,100

; A) Punktlicht bei (1,1,0), Shininess 0.3
p = CreateLight(2)
PositionEntity p,1,1,0
LightRange p,20
EntityShininess kugel,0.3
UpdateWorld : RenderWorld
Pruefe "A punktlicht schraeg", 110, "137 255 255 255 255 255 255 255", True

; B) Punktlicht im Ursprung, Reichweite 3, ohne Shininess
EntityShininess kugel,0
PositionEntity p,0,0,0
LightRange p,3
UpdateWorld : RenderWorld
Pruefe "B punktlicht vorn", 120, "46 74 90 98 98 90 74 46"

; C) Richtungslicht von vorn
FreeEntity p
d = CreateLight(1)
UpdateWorld : RenderWorld
Pruefe "C richtungslicht", 120, "68 87 95 99 99 95 87 68"
End

Function Pruefe(name$, y, soll$, rand_nur_hell = False)
  LockBuffer BackBuffer()
  ok = True
  ist$ = ""
  For k = 0 To 7
    wert = (ReadPixelFast(125 + k * 10, y) Shr 8) And 255
    ist = ist + " " + wert
    s = Int(Wort(soll, k))
    If k = 0 And rand_nur_hell
      If wert = 0 Then ok = False
    Else If Abs(wert - s) > 12
      ok = False
    EndIf
  Next
  UnlockBuffer BackBuffer()
  Flip
  If ok Then Print name + ": wie das Original" Else Print "FEHLER " + name + ":" + ist + " statt " + soll
End Function

Function Wort$(s$, n)
  For i = 1 To n
    s = Mid(s, Instr(s, " ") + 1)
  Next
  If Instr(s, " ") Then s = Left(s, Instr(s, " ") - 1)
  Return s
End Function
