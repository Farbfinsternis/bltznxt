; BUG-195 - LoadBSP, BSPLighting, BSPAmbientLight.
;
; Eine kleine Quake-3-Karte (build/bug195_bsp_20261026/mkbsp.py): Boden mit
; Lichtkarte, Wand mit Vertexfarben, Bezier-Patch, nodraw-Flaeche nur fuer die
; Kollision. Je Ansicht 25 Bildpunkte gegen die Werte von Blitz3D 11.8 (in den
; Data-Zeilen), je Kanal +-3 erlaubt - Interpolation rundet um 1 anders.
; Erwartung: Ausgabe von Blitz3D 11.8.
Graphics3D 320,240,0,2
SetBuffer BackBuffer()
cam=CreateCamera() : PositionEntity cam,0,60,-150 : CameraClsColor cam,0,0,40
Print "fehlt "+LoadBSP("tests\assets\gibtsnicht.bsp")
bsp=LoadBSP("tests\assets\bug195_bsp.bsp")
Print "bsp "+(bsp<>0)
Restore soll
Probe()
BSPLighting bsp,False : Probe()
BSPLighting bsp,True : BSPAmbientLight bsp,80,40,0 : Probe()
BSPAmbientLight bsp,0,0,0
TurnEntity cam,0,40,0 : Probe() : TurnEntity cam,0,-40,0
FreeEntity bsp
bsp=LoadBSP("tests\assets\bug195_bsp.bsp",0.5) : Probe()
EntityType bsp,2
s=CreateSphere() : ScaleEntity s,5,5,5 : EntityType s,1 : EntityRadius s,5
PositionEntity s,0,30,-60
Collisions 1,2,2,2
For i=1 To 20 : MoveEntity s,0,0,-5 : UpdateWorld : Next
Print "kugel z "+Int(EntityZ(s))+" kollisionen "+CountCollisions(s)
PositionEntity s,0,30,0
For i=1 To 40 : MoveEntity s,0,0,5 : UpdateWorld : Next
Print "kugel vor wand z "+Int(EntityZ(s))
Print "DONE"
End

Function Probe()
	Read n$
	Cls : RenderWorld
	ok=0
	For y=60 To 220 Step 40
		For x=20 To 300 Step 70
			Read soll
			c=ReadPixel(x,y) And $FFFFFF
			d=0
			For k=0 To 2
				a=(c Shr (k*8)) And 255 : b=(soll Shr (k*8)) And 255
				If Abs(a-b)>d Then d=Abs(a-b)
			Next
			If d<=3 Then ok=ok+1
		Next
	Next
	Print n$+": "+ok+"/25"
End Function

.soll
Data "normal", $000028, $000028, $000028, $000028, $000028, $485B36, $000028, $404D15, $000028, $000028, $877C1E, $000028, $5A3111, $000028, $000028, $0F2347, $54590E, $31312A, $0F1447, $542D0E, $455C0E, $0C1C47, $28312A, $45410E, $0C1347
Data "ohne lichtkarte", $000028, $000028, $000028, $000028, $000028, $5E6E9A, $000028, $404D15, $000028, $000028, $B0AA56, $000028, $5A3111, $000028, $000028, $2846C8, $E6D228, $878C78, $2846C8, $E6D228, $E6D228, $2846C8, $878C78, $E6D228, $2846C8
Data "ambient", $000028, $000028, $000028, $000028, $000028, $5E6C36, $000028, $636115, $000028, $000028, $B0971E, $000028, $7A4711, $000028, $000028, $1B2E47, $9C7A0E, $5B472A, $1B1F47, $9C4E0E, $8D7D0E, $192747, $53472A, $8D620E, $191E47
Data "links", $000028, $000028, $000028, $000028, $000028, $000028, $000028, $B0AF0F, $419B08, $142523, $000028, $000028, $233445, $1A191C, $8E5406, $000028, $416B0E, $4B600E, $212B38, $111847, $33650E, $0A1F47, $40570E, $333F21, $0D1947
Data "gamma 0.5", $000028, $000028, $000028, $000028, $000028, $52645B, $000028, $404D15, $000028, $000028, $9A9133, $000028, $5A3111, $000028, $000028, $183176, $8A8818, $515347, $182576, $8A6118, $7E8A18, $162C76, $4A5347, $7E7518, $162476
