; BouncingSparks
; Erweiterung des blox'n'balls Partikelsystems
;-----------------------------------------------------------
;
; Type zum halten der Spark-Daten
Type tBSpark
	Field dx#,dy#,dz#
	Field spark
	Field alpha#
End Type
Global bspark.tBSpark

;-----------------------------------------------------------
; Funktionen
;-----------------------------------------------------------
;
; Sparks erzeugen
;
Function CreateBSparks(count,x#,y#,z#,sprite)
	For i=0 To count-1
		bspark.tBSpark=New tBSpark
		bspark\spark=CopyEntity(sprite)
		bspark\alpha=1.0
		bspark\dx#=Rnd(-0.2,0.2)
		bspark\dy#=Rnd(0.5,1.0)
		bspark\dz#=Rnd(-0.2,0.2)
		
		PositionEntity bspark\spark,x,y,z
		EntityAlpha bspark\spark,1
	Next
End Function

;-----------------------------------------------------------
; Sparks aktualisieren
;
Function UpdateBSparks()
	For bspark.tBSpark=Each tBSpark
		aa#=bspark\alpha#
		aa#=aa#-0.01
		If aa#<=0
			FreeEntity bspark\spark
			Delete bspark
		Else
			If EntityY(bspark\spark)<=0.0
				bspark\dy=-bspark\dy
				;bspark\dy=bspark\dy*0.62
			EndIf
			
			bspark\dy=bspark\dy-0.07			
			EntityAlpha bspark\spark,aa#
			MoveEntity bspark\spark,bspark\dx#,bspark\dy#,bspark\dz#
			ScaleSprite bspark\spark,aa*0.4,aa*0.4
			bspark\alpha#=aa#
		EndIf
	Next
End Function
