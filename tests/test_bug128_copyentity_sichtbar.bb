; BUG-128 - eine Kopie ist immer sichtbar.
;
; Gemessen am Original (2026-09-17): CopyEntity liefert eine sichtbare Kopie,
; auch wenn die Vorlage versteckt ist, und jedes mitkopierte Kind ist ebenfalls
; sichtbar, auch wenn es in der Vorlage versteckt war. Der veroeffentlichte
; Quelltext (Entity::Entity(const Entity&)) uebernimmt _visible, das laufende
; Blitz3D 11.8 tut es nicht. Das Muster "Vorlage laden, verstecken, Kopien
; zeigen" (Jet Tails, lodBalls) haengt daran.
;
; Nicht hier: ein versteckter Elternteil versteckt seine Kinder (BUG-136).
; Jeder Fall hat eine eigene Farbe; geprueft wird die Bildmitte.

Graphics3D 320,240,0,2
cam = CreateCamera()
CameraClsColor cam,0,0,0
PositionEntity cam,0,0,-5

Function Px$()
	RenderWorld
	Return ReadPixel(160,120) And $FFFFFF
End Function

Function Wuerfel(r,g,b,parent=0)
	c = CreateCube(parent)
	EntityFX c,1
	EntityColor c,r,g,b
	Return c
End Function

; 1) versteckter Wuerfel, Kopie ohne Parent
c = Wuerfel(0,255,0) : HideEntity c
d = CopyEntity(c)
Print "1 kopie versteckter wuerfel: " + Px()
FreeEntity d : FreeEntity c

; 2) versteckter Wuerfel, Kopie mit sichtbarem Pivot als Parent
c = Wuerfel(0,255,0) : HideEntity c
piv = CreatePivot()
d = CopyEntity(c, piv)
Print "2 kopie mit parent: " + Px()
FreeEntity piv : FreeEntity c

; 3) sichtbarer Parent-Pivot, verstecktes Wuerfel-Kind; Kopie des Pivots
piv = CreatePivot()
c = Wuerfel(255,0,0,piv) : HideEntity c
d = CopyEntity(piv)
Print "3 kopie pivot, kind versteckt: " + Px()
FreeEntity d : FreeEntity piv

; 5) versteckter Wuerfel mit verstecktem Kind-Wuerfel (kleiner, davor)
c = Wuerfel(255,255,0)
k = Wuerfel(255,0,255,c) : ScaleEntity k,0.5,0.5,0.5 : PositionEntity k,0,0,-1.5
HideEntity k : HideEntity c
d = CopyEntity(c)
Print "5 kopie, beide versteckt: " + Px()
FreeEntity d : FreeEntity c

; 6) sichtbarer Wuerfel mit verstecktem Kind davor
c = Wuerfel(255,255,0)
k = Wuerfel(255,0,255,c) : ScaleEntity k,0.5,0.5,0.5 : PositionEntity k,0,0,-1.5
HideEntity k
Print "6a original, kind versteckt: " + Px()
HideEntity c
d = CopyEntity(c)
Print "6b kopie: " + Px()
FreeEntity d : FreeEntity c

; 7) Kopie eines sichtbaren Wuerfels, danach Original verstecken
c = Wuerfel(0,255,255)
d = CopyEntity(c)
HideEntity c
Print "7 kopie bleibt sichtbar: " + Px()
FreeEntity d : FreeEntity c

; 8) Kopie einer Kopie eines versteckten Wuerfels, die Kopie versteckt
c = Wuerfel(128,128,128) : HideEntity c
d = CopyEntity(c) : HideEntity d
e = CopyEntity(d)
Print "8 kopie einer versteckten kopie: " + Px()
FreeEntity e : FreeEntity d : FreeEntity c

End
