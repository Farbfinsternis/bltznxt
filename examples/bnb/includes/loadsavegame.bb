; Load & Save Game
.loadgame
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
	
	For i=0 To 500
		games$(i)=""
		row(i)=0
	Next
	
	bg=LoadImage("dat\gfx\gfx\load_screen.jpg")				: MaskImage bg,255,0,255
	b_load=LoadImage("dat\gfx\gfx\b_load.bmp")				: MaskImage b_load,0,0,50
	b_canc=LoadImage("dat\gfx\gfx\b_loadsave_cancel.bmp")	: MaskImage b_canc,0,0,50
	
	; gesicherte Games suchen
	savedgames$="dat\sve"
	currdir$=CurrentDir()
	dir=ReadDir(savedgames$)
	ChangeDir(savedgames$)
	If dir
		flag=0
		i=flistgame
		Repeat
			file$=NextFile(dir)
			ft=FileType(file$)
			
			If ft=1
				If file$<>"." And file$<>".." And file$<>"" And Right(file$,4)=".sav" Then games$(i)=file$
				i=i+1
			EndIf
			If ft=0 Then flag=1
		Until flag=1
	EndIf
	ChangeDir(currdir$)
	
	;----------
	x=(scr\w/2)-320
	y=(scr\h/2)-240
	tx=x+20
	
	yy=y+62
	For i=flistgame To flistgame+9
		If i<500
			row(i)=yy
			yy=yy+35
		EndIf
	Next
	
	hitgame=-1	
	Repeat
		Cls
		mx=MouseX() : my=MouseY()
		If KeyHit(1) Then Exit
		
		DrawImage bg,x,y
		
		ty=y+60
		For i=flistgame To flistgame+9
			If i<500
				WriteBitmapFont(tx,ty+1,games$(i))
				ty=ty+35
			EndIf
		Next
		
		If hitgame>=0
			Color 255,255,0
			Rect x+20,row(hitgame)-4,600,18,0
		EndIf
		
		If mx>x+64 And my>y+425 And mx<x+194 And my<y+455
			DrawImage b_load,x+64,y+425
					
			If MouseDown(1)
				If hitgame>=0
					loadgamename$=games$(hitgame)
					If FileType("dat\sve\"+loadgamename$)=1
						infile=ReadFile("dat\sve\"+loadgamename$)
						If infile
							scorepoints=ReadInt(infile)
							levl=ReadInt(infile)
							blls=ReadInt(infile)
							dffc=ReadInt(infile)
							
							If demoflag=False
								level=levl Xor gamekey
								balls=blls Xor gamekey
								diffc=dffc Xor gamekey
							Else
								level=levl
								balls=blls
								diffc=dffc
							EndIf
							
							For i=0 To 99
								item$(i)=ReadLine(infile)
								item_num(i)=ReadInt(infile)
								If demoflag=False Then item_num(i)=item_num(i) Xor gamekey
								If item$(i)="" Then last_item=i-1
							Next
							If demoflag=False Then compare=ReadInt(infile)
							If demoflag=False And compare<>30031974 Then End
								
							curr_item=1
							CloseFile infile
							starttime=MilliSecs()

							For i=0 To 6
								If multiballs(i)<>0
									FreeEntity multiballs(i)
									multiballs(i)=0
								EndIf
							Next
							
							; cheaten verhindern
							If game_loaded=True
								scorepoints=loadedcredits
							Else
								loadedcredits=scorepoints
							EndIf
							
							game_loaded=True
							PositionEntity ball,EntityX(pdl),1,5
							Exit
						EndIf
					EndIf
				EndIf
			EndIf
		EndIf
		
		If MouseHit(1)
			For i=flistgame To flistgame+9
				If i<500
					If mx>x+20 And my>row(i) And mx<x+550 And my<row(i)+30 Then hitgame=i
				EndIf
			Next
		EndIf
		
		If mx>x+447 And my>y+425 And mx<x+577 And my<y+455
			DrawImage b_canc,x+447,y+425
			If MouseDown(1)
				While MouseDown(1) : Wend
				Exit
			EndIf
		EndIf
		
		Create2DSparks(1,mx+15,my+15,spark_blue)
		Create2DSparks(2,mx+15,my+15,spark_red)
		
		TurnEntity rad,0,0,-0.5
		TurnEntity rad2,0,0,-0.5
		PositionEntity mz3d,mx+10,my+10,1
		PositionEntity mzlight,mx+15,my+15,2
		UpdateSparks()

		UpdateWorld() : RenderWorld()
		
		Flip
	Forever
	
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
		
	FreeImage bg
	FreeImage b_load
	FreeImage b_canc
	starttime=MilliSecs()
Return

;-----------------------------------------------------------
.savegame
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
	
	For i=0 To 500
		games$(i)=""
		row(i)=0
	Next
	
	bg=LoadImage("dat\gfx\gfx\save_screen.jpg")				: MaskImage bg,255,0,255
	b_save=LoadImage("dat\gfx\gfx\b_savegame.bmp")			: MaskImage b_save,0,0,50
	b_canc=LoadImage("dat\gfx\gfx\b_loadsave_cancel.bmp")	: MaskImage b_canc,0,0,50

	; gesicherte Games suchen
	savedgames$="dat\sve"
	currdir$=CurrentDir()
	dir=ReadDir(savedgames$)
	ChangeDir(savedgames$)
	If dir
		flag=0
		i=flistgame
		Repeat
			file$=NextFile(dir)
			ft=FileType(file$)
			
			If ft=1
				If file$<>"." And file$<>".." And file$<>"" And Right(file$,4)=".sav" Then games$(i)=file$
				i=i+1
			EndIf
			If ft=0 Then flag=1
		Until flag=1
	EndIf
	ChangeDir(currdir$)
	
	;----------
	x=(scr\w/2)-320
	y=(scr\h/2)-240
	tx=x+20
	
	yy=y+60
	For i=flistgame To flistgame+9
		If i<500
			row(i)=yy
			yy=yy+35
		EndIf
	Next
	
	hitgame=-1
	
	Repeat
		Cls
		mx=MouseX() : my=MouseY()
		If KeyHit(1) Then Exit
		
		DrawImage bg,x,y
		
		ty=y+60
		For i=flistgame To flistgame+9
			If i<500
				WriteBitmapFont(tx,ty+1,games$(i))
				ty=ty+35
			EndIf
		Next
		
		hitgame=-1
		For i=flistgame To flistgame+9
			If i<500
				If MouseDown(1) And mx>x+20 And my>row(i) And mx<x+550 And my<row(i)+30
					hitgame=i
					; Savename eingeben
					cursor=0 : cursor_time=MilliSecs
					inname$=games$(hitgame)
					flag=0	
					Repeat
						Cls
						mx=MouseX() : my=MouseY()
						DrawImage door_left,0,0
						DrawImage door_right,scr\w-dr_w,0
						DrawImage bg,x,y
						
						If MouseHit(1)
							Exit
						EndIf
												
						ty=y+60
						For j=flistgame To flistgame+9
							If j<500
								If Not j=hitgame Then WriteBitmapFont(tx,ty+1,games$(j))
								ty=ty+35
							EndIf
						Next

						cx=(x+20)+(Len(inname$)*char_w)
						cy=row(hitgame)
						If MilliSecs()-cursor_time>=200 And cursor=1
							Color 255,255,255
							Rect cx,cy,char_w,char_h,1
						EndIf
						If MilliSecs()-cursor_time>=400
							cursor_time=MilliSecs()
							cursor=1-cursor
						EndIf
		
						key=GetKey()
						If key=27 Or key=13
							FlushKeys
							While MouseDown(1) : Wend
							flag=1
						EndIf
						
						If key>=32 And key<=255 And Len(inname$)<17
							inname$=inname$+Chr(key)
						EndIf
						If key=8 Then inname$=Left(inname$,Len(inname$)-1)
						WriteBitmapFont(x+20,row(hitgame)-1,inname$)
						
						Flip
					Until flag=1
					If hitgame>=0 Then games$(hitgame)=inname$
					FlushKeys
				EndIf
			EndIf	
		Next
				
		If mx>x+592 And my>y+41 And mx<x+627 And my<y+76
			If MouseDown(1)
				seconds=MilliSecs()
				While MilliSecs()-seconds<100 : Wend
				If flistgame-1>=0
					flistgame=flistgame-1
					yy=y+62
					For i=flistgame To flistgame+9
						If i<500
							row(i)=yy
							yy=yy+35
						EndIf
					Next
				EndIf
			EndIf
		EndIf
		
		If mx>x+592 And my>y+377 And mx<x+627 And my<y+416
			If MouseDown(1)
				seconds=MilliSecs()
				While MilliSecs()-seconds<100 : Wend
				If flistgame+1<491
					flistgame=flistgame+1
					yy=y+62
					For i=flistgame To flistgame+9
						If i<500
							row(i)=yy
							yy=yy+35
						EndIf
					Next
				EndIf
			EndIf
		EndIf
		
		If mx>x+64 And my>y+425 And mx<x+194 And my<y+455
			DrawImage b_save,x+64,y+425
			If MouseDown(1)
				FlushMouse
				If inname$<>""
					If Right(inname$,4)<>".sav" Then inname$=inname$+".sav"
						outfile=WriteFile("dat\sve\"+inname$)
						If outfile
							If demoflag=False
								scrp=scorepoints Xor gamekey
								levl=level Xor gamekey
								blls=balls Xor gamekey
								dffc=diffc Xor gamekey
							Else
								scrp=scorepoints
								levl=level
								blls=balls
								dffc=diffc
							EndIf
								
							WriteInt outfile,scrp
							WriteInt outfile,levl
							WriteInt outfile,blls
							WriteInt outfile,dffc
							For i=0 To 99
								WriteLine outfile,item$(i)
								If demoflag=False
									WriteInt outfile,item_num(i) Xor gamekey
								Else
									WriteInt outfile,item_num(i)
								EndIf
							Next
							If demoflag=False Then WriteInt outfile,30031974
							CloseFile outfile
							inname$=""
						EndIf
						Exit
					EndIf
				EndIf
			EndIf
		;EndIf
		
		If mx>x+447 And my>y+425 And mx<x+577 And my<y+455
			DrawImage b_canc,x+447,y+425
			If MouseDown(1)
				While MouseDown(1) : Wend
				Exit
			EndIf
		EndIf
		
		Create2DSparks(1,mx+15,my+15,spark_blue)
		Create2DSparks(2,mx+15,my+15,spark_red)
		
		TurnEntity rad,0,0,-0.5
		TurnEntity rad2,0,0,-0.5
		PositionEntity mz3d,mx+10,my+10,1
		PositionEntity mzlight,mx+15,my+15,2
		UpdateSparks()
		UpdateWorld() : RenderWorld

		Flip
	Forever
	
	While MouseDown(1) : Wend
	
	FreeImage bg
	FreeImage b_save
	FreeImage b_canc
	
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