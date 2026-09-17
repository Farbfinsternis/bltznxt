; BUG-136 - ein versteckter Elternteil versteckt seine Kinder.
;
; Im Original bricht Entity::enumVisible an einem versteckten Entity ab, bevor
; es die Kinder durchlaeuft. Gemessen am 2026-09-17: das gilt fuer Meshes,
; Lichter und Kameras, ueber mehrere Ebenen, auch fuer ein Entity, das erst
; spaeter per EntityParent unter ein verstecktes Entity gehaengt wird. Das
; eigene Flag des Kindes bleibt dabei stehen: ShowEntity auf dem Kind allein
; zeigt es nicht, und ein selbst verstecktes Kind bleibt versteckt, wenn der
; Elternteil wieder gezeigt wird.
;
; Bei der Kamera wird nur geprueft, dass sie nicht mehr rendert; was ohne
; sichtbare Kamera im Backbuffer bleibt, prueft test_bug137_ohne_kamera.

Graphics3D 320, 240, 0, 2

Function Px$()
	RenderWorld
	Return ReadPixel(160, 120) And $FFFFFF
End Function

cam = CreateCamera()
CameraClsColor cam, 0, 0, 0
PositionEntity cam, 0, 0, -5

; --- Meshes
piv = CreatePivot()
k = CreateCube(piv) : EntityFX k, 1 : EntityColor k, 0, 0, 255
HideEntity piv
Print "M1 pivot versteckt: " + Px()
ShowEntity k
Print "M2 nur kind gezeigt: " + Px()
ShowEntity piv
Print "M3 pivot gezeigt: " + Px()
g = CreateCube(k) : EntityFX g, 1 : EntityColor g, 255, 0, 0
PositionEntity g, 0, 0, -1.5 : ScaleEntity g, 0.3, 0.3, 0.3
HideEntity piv
Print "M4 grosseltern versteckt: " + Px()
ShowEntity piv
Print "M5 wieder gezeigt: " + Px()
HideEntity k
Print "M6 mesh als elternteil versteckt: " + Px()
e = CreateCube() : EntityFX e, 1 : EntityColor e, 0, 255, 0
PositionEntity e, 0, 0, -1
Print "M7 neuer wuerfel: " + Px()
EntityParent e, k
Print "M8 unter verstecktes mesh gehaengt: " + Px()
EntityParent e, 0
Print "M9 wieder abgehaengt: " + Px()
FreeEntity e : FreeEntity piv

; --- ein selbst verstecktes Kind bleibt versteckt
piv = CreatePivot()
k = CreateCube(piv) : EntityFX k, 1 : EntityColor k, 0, 0, 255
HideEntity k
HideEntity piv
ShowEntity piv
Print "S1 kind blieb selbst versteckt: " + Px()
ShowEntity k
Print "S2 kind gezeigt: " + Px()

; --- TrisRendered zaehlt nur, was gezeichnet wird
k2 = CreateCube(piv) : EntityFX k2, 1
HideEntity piv
RenderWorld
Print "T1 tris, pivot versteckt: " + TrisRendered()
ShowEntity piv
RenderWorld
Print "T2 tris, alles sichtbar: " + TrisRendered()
FreeEntity piv

; --- Licht unter verstecktem Pivot
AmbientLight 0, 0, 0
c = CreateCube() : EntityColor c, 255, 255, 255
lp = CreatePivot()
l = CreateLight(1, lp)
Print "L1 licht unter sichtbarem pivot: " + Px()
HideEntity lp
Print "L2 pivot versteckt: " + Px()
ShowEntity lp
Print "L3 pivot gezeigt: " + Px()
FreeEntity lp : FreeEntity c

; --- Kamera unter verstecktem Pivot
FreeEntity cam
cp = CreatePivot()
cam = CreateCamera(cp)
CameraClsColor cam, 0, 0, 0
PositionEntity cam, 0, 0, -5
c = CreateCube() : EntityFX c, 1 : EntityColor c, 255, 128, 0
Print "K1 kamera unter sichtbarem pivot: " + Px()
HideEntity cp
ClsColor 0, 0, 255 : Cls
Print "K2 pivot versteckt, rendert nicht: " + (Px() <> "16744448")
ShowEntity cp
Print "K3 pivot gezeigt: " + Px()

Print "fertig"
End
