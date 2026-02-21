; SFX
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
				a#=a#-0.07
			Default
				a#=a#-0.02
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

;-----------------------------------------------------------
; Blitz erzeugen
Function CreateLightning(x#,y#,z#,spr_handle)
	; Zufallszielvektor
	ziel_x#=x+Rnd(x#-20,x#+20)
	ziel_y#=y#
	ziel_z#=z+Rnd(z#+20,z#+50)
	
	; Laenge des Blitzes berechnen
	x_lang#=x#+ziel_x#
	z_lang#=z#+ziel_z#
	laenge#=Sqr((x_lang#^2)+(z_lang#^2))
		
	sprite_count=laenge#
	
	For i=0 To sprite_count
		sfx_blitz.tBlitz=New tBlitz
		sfx_blitz\sfx_x#=x#
		sfx_blitz\sfx_y#=y#
		sfx_blitz\sfx_z#=z#
		sfx_blitz\zx=ziel_x
		sfx_blitz\zy=ziel_y
		sfx_blitz\zz=ziel_z
		sfx_blitz\alpha#=1.0
		sfx_blitz\lightning=CopyEntity(spr_handle)
		EntityAlpha sfx_blitz\lightning,1
		EntityRadius sfx_blitz\lightning,2
		PositionEntity sfx_blitz\lightning,x#,y#,z#
		ScaleSprite sfx_blitz\lightning,1,1
				
		; naechste Position berechnen
		x#=x#+(ziel_x#-x#)/sprite_count
		z#=z#+(ziel_z#-z#)/sprite_count
	Next
	
	For sfx_blitz.tBlitz=Each tBlitz
		x#=sfx_blitz\sfx_x#
		z#=sfx_blitz\sfx_z#
		
		Next	
End Function

;-------------------------------------------------
; Blitz aktualisieren
Function UpdateLightning()
	For sfx_blitz.tBlitz=Each tBlitz
		x#=sfx_blitz\sfx_x#
		y#=sfx_blitz\sfx_y#
		z#=sfx_blitz\sfx_z#
		a#=sfx_blitz\alpha#
		zx#=sfx_blitz\zx
		zy#=sfx_blitz\zy
		zz#=sfx_blitz\zz
		
		If a#<=0
			FreeEntity sfx_blitz\lightning
			Delete sfx_blitz
		Else
			en=LinePick(x,y,z,zx,zy,zz)
			If en
				s=0 : z=0
				While z<map_h
					If block_msh(s,z)=en; And block_typ(s,z)=1
						CreateScore(200,bx#,bz#)
						scorepoints=scorepoints+200
						Msg("200er Blitz")
						block_count=block_count-1
						FreeEntity block_msh(s,z)
						CreateSparks(specialfx+2,bx#,2,bz#,spark_red)
						block_msh(s,z)=0
						z=map_h
					EndIf
					s=s+1
					If s=map_w : s=0 : z=z+1 : EndIf
				Wend
			EndIf

			a#=a#-0.1
			sfx_blitz\alpha#=a#
			EntityAlpha sfx_blitz\lightning,a#
			ScaleSprite sfx_blitz\lightning,a#,a#
			
		EndIf
	Next
End Function