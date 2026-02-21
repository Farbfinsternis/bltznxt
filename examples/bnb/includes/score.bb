; Highscores
.scores
	PlaySound door_snd
	door_l_x=0 : door_r_x=scr\w-ImageWidth(door_right)
	Repeat
		Cls
		door_l_x=door_l_x-10
		door_r_x=door_r_x+10
		DrawImage door_left,door_l_x,0
		DrawImage door_right,door_r_x,0
		Flip
	Until door_l_x<=-dl_w
	Gosub closedoors
	
	For i=0 To 6
		If multiballs(i)>0 Then FreeEntity multiballs(i)
		multiballs(i)=0
	Next
	multiball=-1
	message$=""
	messagetime=0
	
	CameraViewport cam\cam,0,0,0,0
	CameraViewport guicam,0,0,scr\w,scr\h
	ShowEntity guicam
	HideEntity ballcam
	
	; bestehende Geometrie hiden
	HideEntity tbl01
	HideEntity tbl02
	HideEntity pdl
	HideEntity ball
	If nomirror=False Then HideEntity mirror
	
	sh=0 : zh=0
	While zh<map_w
		If block_msh(sh,zh)>0 Then HideEntity block_msh(sh,zh)
		sh=sh+1
		If sh=map_w : sh=0 : zh=zh+1 : EndIf
	Wend
	
	; noch stehende Sparks loeschen
	For spark.tSpark=Each tSpark
		FreeEntity spark\spark
		Delete spark
	Next
	For smk.tSmoke=Each tSmoke
		FreeEntity smk\smoke
		Delete smk
	Next
	For pnts.tScore3D=Each tScore3D
		FreeEntity pnts\pnt
		Delete pnts
	Next

	FlushKeys
	scorefile$=""
	Select diffc
		Case 0
			scorefile$="dat\bl0.tga"
		Case 1
			scorefile$="dat\bl1.tga"
		Case 2
			scorefile$="dat\bl3.tga"
	End Select
	
	; Scorefile lesen
	infile=ReadFile(scorefile$)
	If infile
		For i=0 To 9
			scorenames$(i)=ReadLine(infile)
			scorevalues(i)=ReadInt(infile)
			scorelevel(i)=ReadInt(infile)
		Next
		CloseFile infile
	Else
		For i=0 To 9
			scorenames$(i)=""
			scorevalues(i)=0
			scorelevel(i)=0
		Next
	EndIf
	
	; Images laden
	scoreres=LoadImage("dat\gfx\gfx\score_results.png")
	MaskImage scoreres,0,0,50
	
	;3D Bars
	scorebar=LoadImage("dat\gfx\gfx\scorebar.png")
	MaskImage scorebar,0,0,50
	
	x=(scr\w/2)-320
	y=(scr\h/2)-240
	
	yy=y+80
	For i=0 To 9
		scorebars_y(i)=yy
		yy=yy+32
	Next
		
	;----------
	; Scorebars
	x0=-640 : x1=-640 : x2=-640 : x3=-640 : x4=-640
	x5=-640 : x6=-640 : x7=-640 : x8=-640 : x9=-640
	flag=0
	Repeat
		Cls
		mx=MouseX()
		my=MouseY()
		
		DrawImage door_left,0,0
		DrawImage door_right,scr\w-dr_w,0
		
		DrawImage scorebar,x0,scorebars_y(0)
		DrawImage scorebar,x1,scorebars_y(1)
		DrawImage scorebar,x2,scorebars_y(2)
		DrawImage scorebar,x3,scorebars_y(3)
		DrawImage scorebar,x4,scorebars_y(4)
		DrawImage scorebar,x5,scorebars_y(5)
		DrawImage scorebar,x6,scorebars_y(6)
		DrawImage scorebar,x7,scorebars_y(7)
		DrawImage scorebar,x8,scorebars_y(8)
		DrawImage scorebar,x9,scorebars_y(9)
		
		If x0<x Then x0=x0+32
		If x0>-350 And x1<x Then x1=x1+32
		If x1>-350 And x2<x Then x2=x2+32
		If x2>-350 And x3<x Then x3=x3+32
		If x3>-350 And x4<x Then x4=x4+32
		If x4>-350 And x5<x Then x5=x5+32
		If x5>-350 And x6<x Then x6=x6+32
		If x6>-350 And x7<x Then x7=x7+32
		If x7>-350 And x8<x Then x8=x8+32
		If x8>-350 And x9<x Then x9=x9+32
		
		Create2DSparks(2,mx+15,my+15,spark_blue)
		Create2DSparks(2,mx+15,my+15,spark_red)
		
		TurnEntity rad,0,0,-0.5
		TurnEntity rad2,0,0,-0.5
		PositionEntity mz3d,mx+10,my+10,1
		PositionEntity mzlight,mx+15,my+15,2
		
		UpdateSparks()
		UpdateWorld() : RenderWorld
		
		Flip
	Until x9>=x-1
	
	If levelend=True
		;----------
		; Scoreresult
		For i=-64 To y+208 Step 32
			Cls
			mx=MouseX() : my=MouseY()
			DrawImage door_left,0,0
			DrawImage door_right,scr\w-dr_w,0
		
			For j=0 To 9
				DrawImage scorebar,x,scorebars_y(j)
			Next
			DrawImage scoreres,x,i
			
			Create2DSparks(2,mx+15,my+15,spark_blue)
			Create2DSparks(2,mx+15,my+15,spark_red)
		
			TurnEntity rad,0,0,-0.5
			TurnEntity rad2,0,0,-0.5
			PositionEntity mz3d,mx+10,my+10,1
			PositionEntity mzlight,mx+15,my+15,2
		
			UpdateSparks()
			UpdateWorld() : RenderWorld
			Flip
		Next
	
		; normale Credits
		seconds=MilliSecs()
		While MilliSecs()-seconds<1000
			Cls
			mx=MouseX() : my=MouseY()
			DrawImage door_left,0,0
			DrawImage door_right,scr\w-dr_w,0
		
			For j=0 To 9
				DrawImage scorebar,x,scorebars_y(j)
			Next
			DrawImage scoreres,x,y+208
			WriteBitmapFont(x+45,y+242,"SCORE: "+scorepoints+" CREDITS")
			
			Create2DSparks(2,mx+15,my+15,spark_blue)
			Create2DSparks(2,mx+15,my+15,spark_red)
		
			TurnEntity rad,0,0,-0.5
			TurnEntity rad2,0,0,-0.5
			PositionEntity mz3d,mx+10,my+10,1
			PositionEntity mzlight,mx+15,my+15,2
		
			UpdateSparks()
			UpdateWorld() : RenderWorld
			Flip
		Wend
	
		; Levelbonus 1000	
		seconds=MilliSecs()
		While MilliSecs()-seconds<1000
			Cls
			mx=MouseX() : my=MouseY()
			DrawImage door_left,0,0
			DrawImage door_right,scr\w-dr_w,0
		
			For j=0 To 9
				DrawImage scorebar,x,scorebars_y(j)
			Next
			DrawImage scoreres,x,y+208
			WriteBitmapFont(x+45,y+242,"+LEVELBONUS 1000 CREDITS")
			
			Create2DSparks(2,mx+15,my+15,spark_blue)
			Create2DSparks(2,mx+15,my+15,spark_red)
		
			TurnEntity rad,0,0,-0.5
			TurnEntity rad2,0,0,-0.5
			PositionEntity mz3d,mx+10,my+10,1
			PositionEntity mzlight,mx+15,my+15,2
		
			UpdateSparks()
			UpdateWorld() : RenderWorld
			Flip
		Wend
	
		; Schwierigkeitsgradbonus
		diff_bonus=(scrorepoints/2)*diffc
		seconds=MilliSecs()
		While MilliSecs()-seconds<1000
			Cls
			mx=MouseX() : my=MouseY()
			DrawImage door_left,0,0
			DrawImage door_right,scr\w-dr_w,0
		
			For j=0 To 9
				DrawImage scorebar,x,scorebars_y(j)
			Next
			DrawImage scoreres,x,y+208
			WriteBitmapFont(x+45,y+242,"+DIFFICULTY BONUS "+Str(diff_bonus)+" CREDITS")
			Create2DSparks(2,mx+15,my+15,spark_blue)
			Create2DSparks(2,mx+15,my+15,spark_red)
		
			TurnEntity rad,0,0,-0.5
			TurnEntity rad2,0,0,-0.5
			PositionEntity mz3d,mx+10,my+10,1
			PositionEntity mzlight,mx+15,my+15,2
		
			UpdateSparks()
			UpdateWorld() : RenderWorld
			Flip
		Wend
	
		; Zeitbonus
		If leveltime>0 Then s=(leveltime*500) Else s=0
		seconds=MilliSecs()
		While MilliSecs()-seconds<1000
			Cls
			mx=MouseX() : my=MouseY()
			DrawImage door_left,0,0
			DrawImage door_right,scr\w-dr_w,0
		
			For j=0 To 9
				DrawImage scorebar,x,scorebars_y(j)
			Next
			DrawImage scoreres,x,y+208
			WriteBitmapFont(x+45,y+242,"+TIME BONUS "+Str(s)+" CREDITS")
			
			Create2DSparks(2,mx+15,my+15,spark_blue)
			Create2DSparks(2,mx+15,my+15,spark_red)
		
			TurnEntity rad,0,0,-0.5
			TurnEntity rad2,0,0,-0.5
			PositionEntity mz3d,mx+10,my+10,1
			PositionEntity mzlight,mx+15,my+15,2
		
			UpdateSparks()
			UpdateWorld() : RenderWorld	
			Flip
		Wend

		; Overallscore	
		scorepoints=scorepoints+diff_bonus+1000+s
		seconds=MilliSecs()
		While MilliSecs()-seconds<1000
			Cls
			mx=MouseX() : my=MouseY()
			DrawImage door_left,0,0
			DrawImage door_right,scr\w-dr_w,0
		
			For j=0 To 9
				DrawImage scorebar,x,scorebars_y(j)
			Next
			DrawImage scoreres,x,y+208
			WriteBitmapFont(x+45,y+242,"Comp.Score: "+Str(scorepoints)+" CREDITS")
			
			Create2DSparks(2,mx+15,my+15,spark_blue)
			Create2DSparks(2,mx+15,my+15,spark_red)
		
			TurnEntity rad,0,0,-0.5
			TurnEntity rad2,0,0,-0.5
			PositionEntity mz3d,mx+10,my+10,1
			PositionEntity mzlight,mx+15,my+15,2
		
			UpdateSparks()
			UpdateWorld() : RenderWorld	
			Flip
		Wend
	
		If playername=""
			; Nameinput
			cursor=0 : cursor_time=MilliSecs
			inname$=""
			While Not KeyHit(28)
				Cls
				mx=MouseX() : my=MouseY()
				DrawImage door_left,0,0
				DrawImage door_right,scr\w-dr_w,0
		
				For j=0 To 9
					DrawImage scorebar,x,scorebars_y(j)
				Next
				DrawImage scoreres,x,y+208
		
				cx=(x+45)+(Len("YOUR NAME: ")*char_w)+(Len(inname$)*char_w)
				cy=y+242
				If MilliSecs()-cursor_time>=200 And cursor=1
					Color 255,255,255
					Rect cx,cy,char_w,char_h,1
				EndIf
				If MilliSecs()-cursor_time>=400
					cursor_time=MilliSecs()
					cursor=1-cursor
				EndIf
		
				key=GetKey()
				If key>=32 And key<=255 And Len(inname$)<17
				inname$=inname$+Chr(key)
				EndIf
				If key=8 Then inname$=Left(inname$,Len(inname$)-1)
				WriteBitmapFont(x+45,y+242,"YOUR NAME: "+inname$)
			
				Create2DSparks(2,mx+15,my+15,spark_blue)
				Create2DSparks(2,mx+15,my+15,spark_red)
		
				TurnEntity rad,0,0,-0.5
				TurnEntity rad2,0,0,-0.5
				PositionEntity mz3d,mx+10,my+10,1
				PositionEntity mzlight,mx+15,my+15,2
		
				UpdateSparks()
				UpdateWorld() : RenderWorld
				Flip
			Wend
			playername$=inname$
		Else
			inname$=playername$
		EndIf
	
		;----------
		; Score sortieren
		num=-1
		For i=0 To 9
			If scorevalues(i)<=scorepoints
				num=i
				Exit
			EndIf
		Next
		If num>=0
			For i=8 To num Step -1
				scorevalues(i+1)=scorevalues(i)
				scorenames$(i+1)=scorenames$(i)
				scorelevel(i+1)=scorelevel(i)
			Next
			scorevalues(num)=scorepoints
			scorenames$(num)=inname$
			scorelevel(num)=level
		EndIf
		
		;----------
		; Score sichern
		outfile=WriteFile(scorefile$)
		For i=0 To 9
			WriteLine outfile,scorenames$(i)
			WriteInt outfile,scorevalues(i)
			WriteInt outfile,scorelevel(i)
		Next
		CloseFile outfile
	EndIf
	
	;----------
	; Mainloop
	Repeat
		Cls
		mx=MouseX() : my=MouseY()
		If KeyHit(1) Then Exit
		If MouseDown(1) Then Exit
		
		DrawImage door_left,0,0
		DrawImage door_right,scr\w-dr_w,0
		
		For i=0 To 9
			DrawImage scorebar,x,scorebars_y(i)
		
			WriteBitmapFont(x+135,scorebars_y(i)+11,scorenames$(i))
			WriteBitmapFont(x+412,scorebars_y(i)+11,Str(scorevalues(i)))
			WriteBitmapFont(x+560,scorebars_y(i)+11,Str(scorelevel(i)))
		Next
		
		Create2DSparks(2,mx+15,my+15,spark_blue)
		Create2DSparks(2,mx+15,my+15,spark_red)
		
		TurnEntity rad,0,0,-0.5
		TurnEntity rad2,0,0,-0.5
		PositionEntity mz3d,mx+10,my+10,1
		PositionEntity mzlight,mx+15,my+15,2
		
		UpdateSparks()
		UpdateWorld() : RenderWorld
	
		Flip
	Forever
	
	;----------
	; FreeMem
	FreeImage scorebg
	FreeImage scoreres
	FreeImage scorebar

	levelend=False
	While MouseDown(1) : Wend
	FlushMouse
	
	PlaySound door_snd
	door_l_x=0 : door_r_x=scr\w-ImageWidth(door_right)
	Repeat
		Cls
		door_l_x=door_l_x-10
		door_r_x=door_r_x+10
		DrawImage door_left,door_l_x,0
		DrawImage door_right,door_r_x,0
		Flip
	Until door_l_x<=-dl_w
	Gosub closedoors
Return
	