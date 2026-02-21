; SFX
;-----------------------------------------------------------
; Funken
Type tSpark
	Field spark
	Field x#,y#,z#
	Field alpha#
	Field dx#,dy#,dz#
	Field typ
End Type
Global spark.tSpark

;-----------------------------------------------------------
; Rauch
Type tSmoke
	Field smoke
	Field x#,y#,z#
	Field alpha#
	Field dx#,dz#
End Type
Global smk.tSmoke

;-----------------------------------------------------------
; 3D Score
Type tScore3D
	Field pnt
	Field x#,y#,z#
	Field alpha#
End Type
Global pnts.tScore3D

;-----------------------------------------------------------
; Funken erzeugen
Function CreateSparks(count,x#,y#,z#,spark_sprite)
	For i=1 To count
		spark.tSpark=New tSpark
		spark\spark=CopyEntity(spark_sprite)
		spark\dx=Rnd(-1,1)
		spark\dy=Rnd(0,1)
		spark\dz=Rnd(-1,1)
		spark\x=x
		spark\y=y
		spark\z=z
		spark\alpha=1
	Next
End Function

;-----------------------------------------------------------
; 2D Funken erzeugen
Function Create2DSparks(count,x,y,spark_sprite)
	For i=1 To count
		spark.tSpark=New tSpark
		spark\spark=CopyEntity(spark_sprite)
		
		ScaleSprite spark\spark,.02,.02
		SpriteViewMode spark\spark,2
		EntityParent spark\spark,overlay
		
		spark\dx=Rnd(0,4)
		spark\dy=Rnd(0,4)
		spark\dz=0
		spark\x=x
		spark\y=y
		spark\z=1
		spark\alpha=1
		spark\typ=1
	Next
End Function

;-----------------------------------------------------------
; senkrechte Funken erzeugen
Function CreateFireSparks(count,x#,y#,z#,spark_sprite)
	For i=1 To count
		spark.tSpark=New tSpark
		spark\spark=CopyEntity(spark_sprite)
		spark\dx=Rnd(-1,1)
		spark\dy=1
		spark\dz=Rnd(-1,1)
		spark\x=x+Rnd(-1,1)
		spark\y=y
		spark\z=z+Rnd(-1,1)
		spark\alpha=1
	Next
End Function

;-----------------------------------------------------------
; Funken aktualisieren
Function UpdateSparks()
	For spark.tSpark=Each tSpark
		dx#=spark\dx
		dy#=spark\dy
		dz#=spark\dz
		x#=spark\x
		y#=spark\y
		z#=spark\z
		ss=spark\spark
		a#=spark\alpha
		tt=spark\typ
		
		Select tt
			Case 1
				a#=a#-0.2
			Default
				a#=a#-0.2
		End Select 
		
		If a#<=0
			FreeEntity spark\spark
			Delete spark
		Else
			If spark\spark
				Select tt
					Case 1
						x=x+dx
						y=y+dy
					Default
						x=x+dx/16
						y=y+dy/16
						z=z+dz/16
				End Select
				PositionEntity spark\spark,x,y,z
				EntityAlpha spark\spark,a
				spark\x=x
				spark\y=y
				spark\z=z
				spark\alpha=a
			EndIf
		EndIf
	Next
End Function

;-----------------------------------------------------------
; Ballspur erzeugen
Function CreateSmoke()
	smk.tSmoke=New tSmoke
	If energie<=0 Then smk\smoke=CopyEntity(smoke) Else smk\smoke=CopyEntity(spark_red)
	smk\x=EntityX(ball)+Rnd(-0.5,0.5)
	smk\y=0.1+Rnd(-0.5,0.5)
	smk\z=EntityZ(ball)+Rnd(-0.2,0.2)
	smk\dx=dir_x#+Rnd(-0.2,0.2)
	smk\dz=dir_z#+Rnd(-0.2,0.2)
	smk\alpha=0.5
	PositionEntity smk\smoke,smk\x,smk\y,smk\z
	EntityAlpha smk\smoke,0.5
End Function

;-----------------------------------------------------------
; Ballspur aktualisieren
Function UpdateSmoke()
	For smk.tSmoke=Each tSmoke
		xx#=smk\x
		yy#=smk\y
		zz#=smk\z
		dx#=smk\dx
		dz#=smk\dz
		aa#=smk\alpha
		xx=xx+(-(dx/10))
		zz=zz+(-(dz/10))
		aa=aa-0.02
		If aa<=0
			FreeEntity smk\smoke
			Delete smk
		Else
			PositionEntity smk\smoke,xx,yy,zz
			EntityAlpha smk\smoke,aa
			smk\x=xx
			smk\z=zz
			smk\alpha=aa
		EndIf
	Next
End Function

;-----------------------------------------------------------
; Punktezahl erzeugen
Function CreateScore(p,x#,z#)
	pnts.tScore3D=New tScore3D
	Select p
		Case -10: pnts\pnt=CopyEntity(pnt10min)
		Case 1  : pnts\pnt=CopyEntity(pnt1)
		Case 2  : pnts\pnt=CopyEntity(pnt2)
		Case 10 : pnts\pnt=CopyEntity(pnt10)
		Case 15 : pnts\pnt=CopyEntity(pnt15)
		Case 20 : pnts\pnt=CopyEntity(pnt20)
		Case 30 : pnts\pnt=CopyEntity(pnt30)
		Case 50 : pnts\pnt=CopyEntity(pnt50)
		Case 75 : pnts\pnt=CopyEntity(pnt75)
		Case 100 : pnts\pnt=CopyEntity(pnt100)
		Case 150 : pnts\pnt=CopyEntity(pnt150)
		Case 200 : pnts\pnt=CopyEntity(pnt200)
		
		Case BONUS_EXTRABALL : pnts\pnt=CopyEntity(spr_extraball)
		Case BONUS_SPEEDBALL : pnts\pnt=CopyEntity(spr_speedball)
		Case BONUS_ATOMICBALL : pnts\pnt=CopyEntity(spr_atomicball)
		Case BONUS_MULTI1 : pnts\pnt=CopyEntity(multi1)
		Case BONUS_MULTI3 : pnts\pnt=CopyEntity(multi3)
		Case BONUS_MULTI5 : pnts\pnt=CopyEntity(multi5)
		Case BONUS_PDLOOPS : pnts\pnt=CopyEntity(pdloops)
		Case BONUS_TINYBALL : pnts\pnt=CopyEntity(tinyball)
		Case BONUS_NEWBLOX : pnts\pnt=CopyEntity(blockrespawn)
		Case BONUS_WPADDLE : pnts\pnt=CopyEntity(widepaddle)
		Case BONUS_SLOWDOWN : pnts\pnt=CopyEntity(slowdown)
		Case BONUS_BIGBALL : pnts\pnt=CopyEntity(bigball)
		Case BONUS_EXTRASCORE : pnts\pnt=CopyEntity(extrascore)
		Case BONUS_TIMEPLUS : pnts\pnt=CopyEntity(moretime)
		Case BONUS_TIMEMIN : pnts\pnt=CopyEntity(nomoretime)
	End Select
			
	pnts\x#=x
	pnts\y#=EntityY(ball)
	pnts\z#=z
	pnts\alpha=1.0
	PositionEntity pnts\pnt,pnts\x,pnts\y,pnts\z
	EntityAlpha pnts\pnt,pnts\alpha
End Function

;-----------------------------------------------------------
; Punktezahl anzeigen
Function UpdateScore()
	For pnts.tScore3D=Each tScore3D
		x#=pnts\x
		y#=pnts\y
		z#=pnts\z
		a#=pnts\alpha
	
		y#=y#+0.1
		a#=a#-0.004
		PositionEntity pnts\pnt,x,y,z
		EntityAlpha pnts\pnt,a
		pnts\y=y
		pnts\alpha=a
		If a<=0
			FreeEntity pnts\pnt
			Delete pnts
		EndIf
	Next
End Function

;-----------------------------------------------------------
; Message updaten
Function Msg(mesg$)
	messagetime=MilliSecs()
	message$=mesg$
End Function