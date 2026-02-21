; Globals
; System
Type tScreen
	Field w,h,d,m
	Field title$
End Type
Global scr.tScreen=New tScreen

Global in_window=False
Global specialfx=12											; Menge der Spezialeffekte
Global envmapping=True										; Kugeleffekt Flag
Global nomirror=False										; Spiegelflag
Global autoadjust=True										; stellt SpecialFX-Menge autom. ein
Global limitframe=False										; limitiert die FPS auf 60
Global mouse_speed#=0.2										; Mausempfindlichkeit
Global music_vol#=0.8										; Musikvolume
Global vol#=music_vol#										; Volume zur Laufzeit aendern
Global sound_vol#=1.0										; Soundvolume
Global levelend=False										; Levelende
Global game_loaded=False									; Spiel geladen
Global loadedcredits=0
Global starttimer
Global demoflag=True

Dim dend(4)

; CFG File fuer Updates schreiben
windir$=SystemProperty("windowsdir")
bnbcfg$="blox-n-balls.cfg"
If FileType(windir$+bnbcfg$)<>1
	outfile=WriteFile(windir$+bnbcfg$)
	WriteLine outfile,CurrentDir$()
EndIf

Include "includes\readconfig.bb"							; blox.ini lesen

;-----------------------------------------------------------
; Gfx-Mode setzen
Graphics3D scr\w,scr\h,scr\d,scr\m
SetBuffer BackBuffer()

AppTitle scr\title$
HidePointer

;-----------------------------------------------------------
; Intro anzeigen

;###########################################################
; Includes
;###########################################################
Include "includes\pak.bb"
Include "includes\init_bout.bb"			; Init-File einbinden
Include "includes\keys.bb"				; Tastaturkonstanten
Include "includes\multiplayer_global.bb"
Include "includes\getbanner.bb"

;-----------------------------------------------------------
SeedRnd MilliSecs()
frametime=CreateTimer(30)
t=MilliSecs()
pz=0
pf=0
fpscontrol=MilliSecs()

Global shotz=0
panel_x=(scr\w/2)-320
tvid=TotalVidMem()
avid=AvailVidMem()
rumble=0
rumble_time=0
kill_blox=0
speedadjust=MilliSecs()

Global multiplayer=False

;-----------------------------------------------------------
; Variablen fuer "Start"
Dim punkt_y(9)
Dim diff(3)

HideEntity ballcam

;// TEST
;Gosub mainmenu

If multiplayer=True
	Gosub start
Else
	Gosub start_old
EndIf

ShowEntity ballcam
HideEntity guicam
While MouseDown(1) : Wend
CreateMap(level)

;###########################################################
; Hauptschleife
;###########################################################
cbcx=scr\w
cbcw=0

Global performancecheck=MilliSecs()
MoveMouse scr\w,scr\h

Repeat
	Cls
	msx=MouseXSpeed() : msy=MouseYSpeed()
	
	;..........................................................
	; Zeitmessungen
	; Bilder pro Sekunde messen
		
	If zeit-t=>1000 Then
		t=zeit
		fps=fpstmp
		fpstmp=0
		specialfx=fps/4
		speed#=(60.0*start_speed#)/fps
	End If
	fpstmp=fpstmp+1
	zeit=MilliSecs()
	
	;..........................................................
	; Tastatur abfragen
	;..........................................................
	
	; // TEST
	
	If KeyHit(59) Then level=6
	  
	; // Ende TEST
	
	; Ballcam
	If KeyHit(15) Then ballcam_mode=1-ballcam_mode
	
	; ESC gedrueckt :: zurueck zum Mainmenu
	If KeyHit(1)
		If ChannelPlaying(music_play) Then StopChannel music_play
		game_esc=True
		
		ballcam_x=scr\w
		ballcam_mode=0
		
		ShowEntity guicam
		HideEntity ballcam
		
		If multiplayer=True
			Gosub start
		Else
			Gosub start_old
		EndIf
		
		ShowEntity ballcam
		HideEntity guicam
	EndIf

	;..........................................................
	; Screenshot erstellen
	If KeyHit(88)
		SaveBuffer(FrontBuffer(),"shot0"+Str(shotz)+".bmp")
		shotz=shotz+1
	EndIf	
			
	;..........................................................
	; Extras durchschalten
	If KeyHit(200) Or MouseZSpeed()<0
		If curr_item-1>=0
			curr_item=curr_item-1
		Else
			curr_item=last_item
		EndIf
	EndIf
	
	If KeyHit(208) Or MouseZSpeed()>0
		If curr_item+1<=last_item
			curr_item=curr_item+1
		Else
			curr_item=0
		EndIf
	EndIf

	;..........................................................
	; Tisch anstossen
	If KeyHit(57)
		dir_x=dir_x+Float(Int(Rnd(-1,1)))
		dir_z=dir_z+Float(Int(Rnd(-1,1)))
		rumble=1
		rumble_time=zeit
	EndIf
	
	;..........................................................
	; Cheat
	If KeyDown(29) And KeyHit(49) Then block_count=0
		 	
	;..........................................................
	; Item ausloesen
	If KeyDown(29) Or KeyDown(157) Or MouseHit(2)
		Select item$(curr_item)
			Case "ATOMICBALL"
				kill_blox=1
				CreateSparks(specialfx*2,bx#,by#,bz#,spark_rainbow)
			Case "EXTRABALL"
				balls=balls+1
				CreateSparks(specialfx*2,bx#,by#,bz#,spark_rainbow)
				item$(curr_item)=""
				curr_item=curr_item-1
				If curr_item<0 Then curr_item=last_item
				last_item=last_item-1
			Case "TIME +15SECS"
				leveltime=leveltime+15
				CreateSparks(specialfx*2,bx#,by#,bz#,spark_rainbow)
				item$(curr_item)=""
				curr_item=curr_item-1
				If curr_item<0 Then curr_item=last_item
				last_item=last_item-1
			Case "WIDEPADDLE"
				If wpaddle_count<2
					ScaleMesh pdl,2,1,1
					wpaddle_count=wpaddle_count+1
					wpaddle_time=MilliSecs()
					CreateSparks(specialfx*2,bx#,by#,bz#,spark_rainbow)
					item$(curr_item)=""
					curr_item=curr_item-1
					If curr_item<0 Then curr_item=last_item
					last_item=last_item-1
				EndIf
			Case "SLOWMOTION"
				old_speed#=start_speed#
				start_speed#=start_speed#/2
				ball_slowdown=MilliSecs()
				CreateSparks(specialfx*2,bx#,by#,bz#,spark_rainbow)
				item$(curr_item)=""
				curr_item=curr_item-1
				If curr_item<0 Then curr_item=last_item
				last_item=last_item-1
		End Select
	EndIf
	
	;..........................................................
	; 2D Stuff :: Ebene 1
	;..........................................................
	DrawImage panel_img,panel_x,0								; Panel
	
	;..........................................................
	WriteBitmapFont(panel_x+558,30,sek$)						; verbleibende Sekunden
	
	;..........................................................
	; aktuelles Level 3-stellig anzeigen
	If lev$<100 Then lev$="0"+Str(level)
	If level<10 Then lev$="00"+Str(level)
	If level>=100 Then lev$=Str(level)
	WriteBitmapFont(panel_x+190,30,lev$)
	WriteBitmapFont(panel_x+25,30,Str(scorepoints))
	
	;..........................................................
	; verfuegbare Baelle ins Panel blitten
	If balls>1
		xx=panel_x+301
		For i=0 To balls-2
			DrawImage balls_img,xx,27
			xx=xx+24
		Next
	EndIf
	
	;..........................................................
	; aktuelle Message anzeigen	
	If zeit-messagetime<20000
		WriteBitmapFont(panel_x+20,72,message$)
	Else
		message$=""
		messagetime=0
	EndIf
	
	;..........................................................
	; aktuelles Extra anzeigen
	WriteBitmapFont(panel_x+350,72,item$(curr_item))
	
	;..........................................................
	; Bilder pro Sekunde anzeigen
	If showfps=True Then WriteBitmapFont(0,102,Str(fps)+"FPS")
		
	;..........................................................
	; 3D Stuff
	;..........................................................
	; Tisch bewegen wenn kein Multiball aktiv ist
	If multiball<=0
		RotateEntity cam\cam,cam\rx-(EntityZ(ball)*0.1),0,0
	Else	
		RotateEntity cam\cam,cam\rx,0,0
	EndIf
	
	If rumble=1
		If zeit-rumble_time>100
			rumble=0
			rumble_time=0
			RotateEntity cam\cam,cam\rx,0,0
		Else
			RotateEntity cam\cam,cam\rx,EntityYaw(cam\cam)+Rnd(-0.5,0.5),EntityRoll(cam\cam)+Rnd(-0.5,0.5)
		EndIf
	EndIf

	;..........................................................
	; Levelstart checken
	If levelstart=True
		ClearCollisions
		Collisions COLL_PADDLE,COLL_WALL,2,1
		
		dir_x=0 : dir_z=0
		
		MoveMouse scr\w/2,scr\h/2
		
		MoveEntity pdl,msx*mouse_speed#,0,0
		PositionEntity ball,EntityX(pdl),1.4,6
				
		If MouseHit(1)
			FlushMouse
			ShowEntity ball
			levelstart=False
			dir_z=2.0
			dir_x=0.0
			
			Collisions COLL_BALL,COLL_BLOCK,2,3
			Collisions COLL_BALL,COLL_WALL,2,3
			Collisions COLL_BALL,COLL_PADDLE,2,3
			Collisions COLL_BALL,COLL_GROUND,2,3
			Collisions COLL_BALL,COLL_BUMPER,2,3

			Collisions COLL_MBALL,COLL_BLOCK,2,3
			Collisions COLL_MBALL,COLL_WALL,2,3
			Collisions COLL_MBALL,COLL_PADDLE,2,3
			Collisions COLL_MBALL,COLL_GROUND,2,3
			Collisions COLL_MBALL,COLL_BUMPER,2,3
			Collisions COLL_BALL,COLL_MBALL,2,3
			Collisions COLL_MBALL,COLL_BALL,2,3
		EndIf
		FlushMouse
	EndIf
	
	;..........................................................
	; Verloren-Flag gesetzt?			
	If lose=True Then Gosub verloren
	
	;..........................................................
	; Alle Bloecke abgeraeumt?
	If block_count<=0
		; Stage complete
		level_end=MilliSecs()
		While MilliSecs()-level_end<2000
			Cls
			msx=MouseXSpeed()
			
			DrawImage panel_img,panel_x,0
			
			If lev$<100 Then lev$="0"+Str(level)
			If level<10 Then lev$="00"+Str(level)
			If level>=100 Then lev$=Str(level)
			WriteBitmapFont(panel_x+190,30,lev$)
			WriteBitmapFont(panel_x+25,30,Str(scorepoints))
			
			MoveEntity pdl,msx*mouse_speed#,0,0

			UpdateSparks()
			UpdateSmoke()
			UpdateScore()
			UpdateLightning()
						
			UpdateWorld() : RenderWorld()
			
			If leveltime>0
				DrawImage nextstage,(scr\w/2)-200,(scr\h/2)-32
			Else
				DrawImage timeover,(scr\w/2)-128,(scr\h/2)-16
			EndIf
			
			Flip
		Wend
		
		cbcx=scr\w
		cbcw=0
		
		energie=0
		chann=PlaySound(win_snd)
		ChannelVolume chann,sound_vol#
		
		levelend=True
		Gosub scores
		Gosub shop
		
		ShowEntity tbl01
		ShowEntity tbl02
		ShowEntity pdl
		ShowEntity ball
		sh=0 : zh=0
		While zh<map_w
			If block_msh(sh,zh)>0 Then ShowEntity block_msh(sh,zh)
			sh=sh+1
			If sh=map_w : sh=0 : zh=zh+1 : EndIf
		Wend
		
		If diffc=0 And balls<3 And level=1 Then balls=3
		If diffc=0 And balls<2 And level=2 Then balls=2
		
		levelstart=True
		level=level+1
		start_speed#=start_speed#+0.01
		
		z#=EntityZ(pdl) : d#=MeshDepth(pdl) : zd#=(z#+d#)
		
		If demoflag=True And level>5 Then Gosub demoend
		CreateMap(level)
		
		If MeshWidth(ball)>2 Then ScaleMesh ball,0.5,0.5,0.5

		Gosub start0
	EndIf
		
	;..........................................................	
	; Sekunden berechnen
	If zeit-starttime>=1000
		leveltime=leveltime-1
		show_sek=leveltime Mod 60
		If show_sek>=10
			sek$=Str(leveltime/60)+":"+Str(show_sek)
		Else
			sek$=Str(leveltime/60)+":0"+Str(show_sek)
		EndIf
		starttime=zeit
		; Zeit zuende?
		If leveltime<=0
			lose_blocks=block_count
			block_count=0
		EndIf
	EndIf
		
	;..........................................................
	If levelstart=False Then Gosub HandleBall
	Gosub handle_multiball
	Gosub handle_extras
	
	If EntityDistance(pdl,ball)<4 Then dir_x=dir_x+(-(msx*mouse_speed))
	
	;..........................................................
	; Ballcam zeigen
	If ballcam_mode=0
		If ballcam_x<=scr\w Then ballcam_x=ballcam_x+10
	EndIf
	If ballcam_mode=1
		If ballcam_x>=scr\w-144 Then ballcam_x=ballcam_x-10
	EndIf
	
	If ballcam_x>=scr\w
		HideEntity ballcam
	Else
		ShowEntity ballcam
	EndIf
	
	
	
	CameraViewport ballcam,ballcam_x+8,ballcam_y+24,scr\w-ballcam_x-8,128
	PositionEntity ballcam,bx,by,bz+0.5

		
	UpdateWorld() : RenderWorld()
	DrawImage bcamimg,ballcam_x,ballcam_y
	;..........................................................
	MoveMouse scr\w/2,scr\h/2
	If limitframe=True
		WaitTimer(frametime)
	EndIf
	
	
	;DrawImage demotxt,scr\w-100,104
	dty=scr\h-StringHeight("X")-4
	Text 4,dty,"DEMO",0,0

	Flip
Forever

;###########################################################
; Includes 2
;###########################################################
Include "includes\action.bb"			; Ballhandling
Include "includes\score.bb"				; Highscores
Include "includes\loadsavegame.bb"		; Laden/Sichern Requester
Include "includes\WriteBitmapFont.bb"	; Bitmaptext Funktion
Include "includes\sfx.bb"				; Sprite Effekte
Include "includes\bouncingsparks.bb"	; Physiksprites
Include "includes\iscore2.bb"

;-----------------------------------------------------------
; Map erzeugen
Function CreateMap(lvl_n)
	
	Cls
	DrawImage loadingpic,0,0
	Color 255,255,255
	Text (scr\w/2)+100,(scr\h/2)+10,"...Speicher leeren"
	Flip
	
	energie=0
	; Mapdaten loeschen
	block_count=0
	For x=0 To map_w
		For y=0 To map_h
			If block_msh(x,y)>0 Then FreeEntity block_msh(x,y)
			block_msh(x,y)=0								; Meshhandles
			block_col(x,y)=0								; Blockfarbe
			block_typ(x,y)=0								; normal oder Bonus
			block_alp(x,y)=0								; Blockalpha
			map(x,y)=-1
		Next
	Next
	
	Cls
	DrawImage loadingpic,0,0
	Color 255,255,255
	Text (scr\w/2)+100,(scr\h/2)+10,"...Leveldaten lesen"
	Flip
		
	; Mapdaten aus Levelfile lesen
	If lvl_n<10
		filename$="dat\lvl\lvl0"+Str(lvl_n)+".bbl"
	Else
		filename$="dat\lvl\lvl"+Str(lvl_n)+".bbl"
	EndIf
	fileexists=False
	If FileType(filename$)=1
		fileexists=True
		infile=OpenFile(filename$)
		If infile
			s=0 : z=0
			While z<map_h
				map(s,z)=ReadInt(infile)
				s=s+1
				If s=map_w : s=0 : z=z+1 : EndIf
			Wend
			fsize=FileSize(filename$)
			If fsize>784
				leveltime=ReadInt(infile)
				music$=ReadLine(infile)
			EndIf
			CloseFile infile
		EndIf
	Else
		lvl_n=0
	EndIf
		
	x1#=EntityX(block)
	y1#=EntityY(block)
	z1#=EntityZ(block)
	ww#=MeshWidth(block)
	dd#=MeshDepth(block)
	
	If fileexists=False Then mph=10 Else mph=14
	
	Cls
	DrawImage loadingpic,0,0
	Color 255,255,255
	Text (scr\w/2)+100,(scr\h/2)+10,"...Level erstellen"
	Flip
	
	ss=0 : zz=0
	While zz<mph
		If lvl_n>0
			bltype=map(ss,zz)
			If bltype>=0
				block_typ(ss,zz)=bltype+1
				btex=Rnd(0,max_tex)
			EndIf
		Else
			block_typ(ss,zz)=Rnd(0,3)
			btex=Rnd(0,max_tex)
		EndIf
		
		If block_typ(ss,zz)>0
			Select block_typ(ss,zz)
				Case 15
					block_msh(ss,zz)=CopyEntity(bumper)
					EntityTexture bumper,bumper_tex
					EntityType block_msh(ss,zz),COLL_BUMPER
					block_alp(ss,zz)=1
					block_x#(ss,zz)=x1-4
					block_y#(ss,zz)=y1
					block_z#(ss,zz)=z1+6
					PositionEntity block_msh(ss,zz),x1,y1,z1
					
				Case 16
					block_msh(ss,zz)=CopyEntity(blockerz)
					EntityTexture block_msh(ss,zz),blocker_tex
					EntityType block_msh(ss,zz),COLL_WALL
					block_alp(ss,zz)=1
					block_x#(ss,zz)=x1-4
					block_y#(ss,zz)=y1
					block_z#(ss,zz)=z1+6
					PositionEntity block_msh(ss,zz),x1,y1,z1
				
				Case 17
					block_msh(ss,zz)=CopyEntity(blocker)
					EntityTexture block_msh(ss,zz),blocker_tex
					EntityType block_msh(ss,zz),COLL_WALL
					block_alp(ss,zz)=1
					block_x#(ss,zz)=x1-4
					block_y#(ss,zz)=y1
					block_z#(ss,zz)=z1+6
					PositionEntity block_msh(ss,zz),x1,y1,z1
				
				Case 18
					block_msh(ss,zz)=CopyEntity(blocker45)
					EntityTexture block_msh(ss,zz),blocker_tex
					EntityType block_msh(ss,zz),COLL_WALL
					block_alp(ss,zz)=1
					block_x#(ss,zz)=x1-4
					block_y#(ss,zz)=y1
					block_z#(ss,zz)=z1+6
					PositionEntity block_msh(ss,zz),x1,y1,z1
				
				Default
					block_msh(ss,zz)=CopyEntity(block)
					EntityTexture block_msh(ss,zz),blocktex(btex)
					block_tex(ss,zz)=blocktex(btex)
					EntityType block_msh(ss,zz),COLL_BLOCK
					EntityPickMode block_msh(ss,zz),1
					
					block_count=block_count+1
					block_alp(ss,zz)=1
					block_x#(ss,zz)=x1-4
					block_y#(ss,zz)=y1
					block_z#(ss,zz)=z1+6
					PositionEntity block_msh(ss,zz),x1,y1,z1
			End Select
			If specialfx>4 Then CreateSparks(1,x1#-8,y1#+2,z1#+40,spark_blue)
			If specialfx>4 Then CreateSparks(1,x1#-8,y1#+2,z1#+40,spark_red)
		EndIf
		
		x1#=x1#+ww# : ss=ss+1
		If ss=map_w
			ss=0 : zz=zz+1
			x1#=EntityX(block)
			z1#=z1#-dd#
		EndIf
				
	Wend
	
	bc=block_count
	If FileType(music$)=1
		music_play=PlayMusic(music$)
		If music_vol#<=0 And ChannelPlaying(music_play)
			StopChannel music_play
		Else
			ChannelVolume music_play,music_vol#
			vol#=music_vol#
		EndIf
	EndIf
	MoveMouse scr\w/2,scr\h/2
	
	If level>1 Then start_speed=start_speed+0.025
	
	Select diffc
		Case 1 : leveltime=leveltime/1.5
		Case 2 : leveltime=leveltime/2
	End Select
	
End Function

;###########################################################
; Losescreen
;###########################################################
.verloren
	level=1
	levelstart=True
	scorepoints=0
	If bigball_time>0 ScaleEntity ball,0.5,0.5,0.5
	
	chann=PlaySound(lose_snd)
	ChannelVolume chann,sound_vol#
	
	FlushMouse
	lose_bmp=LoadImage("dat\gfx\gfx\lose.jpg")
	MaskImage lose_bmp,150,150,150
	Gosub closedoors
	
	Repeat
		Cls
		DrawImage door_left,0,0
		DrawImage door_right,scr\w-dr_w,0
		DrawImage lose_bmp,(scr\w/2)-320,(scr\h/2)-240
		Flip
	Until MouseHit(1)
	FreeImage lose_bmp
	lose=False
	
	PlaySound door_snd
	door_l_x=0 : door_r_x=scr\w-ImageWidth(door_right)
	Repeat
		Cls
		door_l_x=door_l_x-10
		door_r_x=door_r_x+10
		DrawImage door_left,door_l_x,0
		DrawImage door_right,door_r_x,0
		Flip
	Until door_r_x>=scr\w
	
	HideEntity ballcam
	ShowEntity guicam
	
	If multiplayer=True
		Gosub start
	Else
		Gosub start_old
	EndIf
	
	ShowEntity ballcam
Return	

;###########################################################
; Startmenu
;###########################################################
.start_old
	CameraViewport cam\cam,0,0,0,0
	CameraViewport guicam,0,0,scr\w,scr\h
	
	FlushKeys : FlushMouse
	If ChannelPlaying(music_play) Then StopChannel(music_play)
	bgchann=PlayMusic("dat\sfx\mainmusic.mp3")
	ChannelVolume bgchann,music_vol#
	
	While MouseDown(1) : Wend

	startscreen=LoadImage("dat\gfx\gfx\start2.bmp")
	credits=LoadImage("dat\gfx\gfx\credits.png")
	MaskImage credits,0,0,50
	MaskImage startscreen,0,0,50	
		
	akt_diff=0
	
	punkt_y(1)=140
	punkt_y(2)=175
	punkt_y(3)=210
	punkt_y(4)=245
	punkt_y(5)=280
	punkt_y(6)=315
	punkt_y(7)=350
	punkt_y(8)=385
	
	punkt=1
	apy=punkt_y(1)
	x=(scr\w/2)-320
	y=(scr\h/2)-240
	clicked=0
	
	; 3D Variablen
	; bestehende Geometrie hiden
	HideEntity tbl01
	HideEntity tbl02
	HideEntity pdl
	HideEntity ball
	If nomirror=False Then HideEntity mirror
	s=0 : z=0
	While z<map_w
		If block_msh(s,z)>0 Then HideEntity block_msh(s,z)
		s=s+1
		If s=map_w : s=0 : z=z+1 : EndIf
	Wend
	For i=0 To 6
		If multiballs(i)>0 Then HideEntity multiballs(i)
	Next
	
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
	For bspark.tBSpark=Each tBSpark
		FreeEntity bspark\spark
		Delete bspark
	Next
	
	Gosub closedoors
	
	For i=-480 To y Step 10
		Cls
		DrawImage door_left,0,0
		DrawImage door_right,scr\w-dr_w,0
		DrawImage startscreen,x,i
		Flip
	Next
	
	; 3D Selector erstellen
	guiball=CreateSphere()
	ScaleEntity guiball,0.03,0.03,0.03
	EntityParent guiball,overlay
	EntityTexture guiball,ball_tex,0,0
	EntityTexture guiball,envtex,0,1
	
	selector=LoadSprite("dat\gfx\gfx\smenu_select.tga",3)
	ScaleSprite selector,2.0,0.04
	;EntityAlpha selector,0.5
	EntityParent selector,overlay	
	;----------
	Repeat
		Cls
		
		If KeyHit(88)
			SaveBuffer(FrontBuffer(),"shot0"+Str(shotz)+".bmp")
			shotz=shotz+1
		EndIf
		
		punkt=-1
		If KeyHit(1)
			If game_esc=True
				If FileType(music$)=1
					music_play=PlayMusic(music$)
					If music_vol#<=0 And ChannelPlaying(music_play)
						StopChannel music_play
					Else
						ChannelVolume music_play,music_vol#
						vol#=music_vol#
					EndIf
				EndIf
				Exit
			Else
				ClearWorld()
				End
			EndIf
		EndIf
		
		mx=MouseX() : my=MouseY()
		msy=MouseZSpeed()
		
		DrawImage door_left,0,0
		DrawImage door_right,scr\w-dr_w,0
		
		DrawImage startscreen,x,y
		WriteBitmapFont(x+450,y+95,aktdiff$)
		If demoflag Then DrawImage dimg,5,5
		
		; Mauszeiger abfragen
		If mx>x+100 And my>y+25 And mx<x+450 And my<y+75	; New Game
			PositionEntity guiball,x+420,y+50,1
			Create2DSparks(2,x+420,y+50,spark_blue)
			PositionEntity selector,x,y+50,1
			
			If MouseHit(1)
				punkt=1
				clicked=1
			EndIf
		EndIf
		
		If mx>x+100 And my>y+75 And mx<x+450 And my<y+125	; Skill Level
			PositionEntity guiball,x+420,y+100,1
			Create2DSparks(2,x+420,y+100,spark_blue)
			PositionEntity selector,x,y+100,1
			
			If MouseHit(1)
				punkt=2
				clicked=1
				akt_diff=akt_diff+1
				If akt_diff>2 Then akt_diff=0
			EndIf
		EndIf
		
		If mx>x+100 And my>y+125 And mx<x+450 And my<y+175	; Load Game
			PositionEntity guiball,x+420,y+150,1
			Create2DSparks(2,x+420,y+150,spark_blue)
			PositionEntity selector,x,y+150,1
			
			If MouseHit(1)
				punkt=3
				clicked=1
			EndIf
		EndIf
		If mx>x+100 And my>y+175 And mx<x+450 And my<y+225	; Save Game
			PositionEntity guiball,x+420,y+200,1
			Create2DSparks(2,x+420,y+200,spark_blue)
			PositionEntity selector,x,y+200,1
			
			If MouseHit(1)
				punkt=4
				clicked=1
			EndIf
		EndIf
		If mx>x+100 And my>y+225 And mx<x+450 And my<y+275	; Highscore
			PositionEntity guiball,x+420,y+250,1
			Create2DSparks(2,x+420,y+250,spark_blue)
			PositionEntity selector,x,y+250,1
			
			If MouseHit(1)
				punkt=5
				clicked=1
			EndIf
		EndIf
		If mx>x+100 And my>y+275 And mx<x+450 And my<y+325	; i.SCORE
			PositionEntity guiball,x+420,y+300,1
			Create2DSparks(2,x+420,y+300,spark_blue)
			PositionEntity selector,x,y+300,1
			
			If MouseHit(1)
				HideEntity guiball
				Gosub iscore_sub
				ShowEntity guiball
			EndIf
		EndIf
		If mx>x+100 And my>y+325 And mx<x+450 And my<y+375	; Credits
			PositionEntity guiball,x+420,y+350,1
			Create2DSparks(2,x+420,y+350,spark_blue)
			PositionEntity selector,x,y+350,1
			
			If MouseHit(1)
				punkt=7
				clicked=1
			EndIf
		EndIf
		If mx>x+100 And my>y+375 And mx<x+450 And my<y+425	; Exit
			PositionEntity guiball,x+420,y+400,1
			Create2DSparks(2,x+420,y+400,spark_blue)
			PositionEntity selector,x,y+400,1
			
			If MouseHit(1)
				punkt=8
				clicked=1
			EndIf
		EndIf
		If mx>x+100 And my>y+425 And mx<x+450 And my<y+475	; Back
			PositionEntity guiball,x+420,y+450,1
			Create2DSparks(2,x+420,y+450,spark_blue)
			PositionEntity selector,x,y+450,1
			
			If MouseHit(1) And game_esc=True
				performancecheck=MilliSecs()
				Exit
			Else
				punkt=-1
			EndIf
		EndIf
		
		If MouseDown(1) And mx>x+130 And my>y+320 And mx<x+300 And my<y+340
					
		EndIf
		
		Select punkt
			Case 1
				
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
				
				If akt_diff=0
					speed#=0.3
					balls=5
					diffc=0
					gravity=1.0
				EndIf
				If akt_diff=1
					speed#=0.6
					balls=4
					diffc=1
					gravity=1.0
				EndIf
				If akt_diff=2
					speed#=1.0
					balls=3
					diffc=2
					gravity=1.5
				EndIf
				scorepoints=0
				levelstart=True
				MoveMouse scr\w/2,scr\h/2
				z#=EntityZ(pdl) : d#=MeshDepth(pdl) : zd#=(z#+d#)
				leveltime=180
				CreateMap(level)
				start_speed#=speed#
				scorepoints=0
				
				; alte Werte zuruecksetzen
				For i=0 To 6
					If multiballs(i)>0 Then FreeEntity multiballs(i)
					multiballs(i)=0
				Next
				If MeshWidth(ball)>2 Then ScaleMesh ball,0.5,0.5,0.5
				
				ShowEntity ball
				performancecheck=MilliSecs()
				FlushMouse
				Exit
			
			Case 3
				HideEntity guiball
				Gosub loadgame
				ShowEntity guiball
				If game_loaded=True
					game_loaded=False
					If diffc=0
						speed#=0.5
						gravity=1.0
					EndIf
					If diffc=1
						speed#=0.7
						gravity=1.0
					EndIf
					If diffc=2
						speed#=0.9
						gravity=1.5
					EndIf
					leveltime=180
					start_speed#=speed#
					multiball=-1
					performancecheck=MilliSecs()
					levelstart=True
					CreateMap(level)
					Exit
				EndIf
				
			Case 4
				HideEntity guiball
				Gosub savegame
				ShowEntity guiball
					
			Case 5
				levelend=False
				diffc=akt_diff
				CameraViewport cam\cam,0,0,0,0
				CameraViewport guicam,0,0,scr\w,scr\h
				HideEntity guiball
				Gosub scores
				ShowEntity guiball
				CameraViewport cam\cam,0,0,0,0
				CameraViewport guicam,0,0,scr\w,scr\h
			Case 6
		
					
			Case 7
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
				Cls
				Gosub closedoors
				While MouseDown(1) : Wend
				flag=0
				HideEntity guiball
				While flag=0
					Cls
					
					mx=MouseX() : my=MouseY()
					If KeyHit(KEY_ENTER) Or KeyHit(KEY_NUM_ENTER) Then flag=1
					If KeyHit(KEY_ESC) Then flag=1
					If MouseHit(1) Then flag=1
					If MouseHit(2) Then flag=1
					DrawImage door_left,0,0
					DrawImage door_right,scr\w-dr_w,0
					
					DrawImage credits,((scr\w/2)-320),((scr\h/2)-240)
					Create2DSparks(1,mx+15,my+15,spark_blue)
					Create2DSparks(2,mx+15,my+15,spark_red)
		
					TurnEntity rad,0,0,-0.5
					TurnEntity rad2,0,0,-0.5
					PositionEntity mz3d,mx+10,my+10,1
					PositionEntity mzlight,mx+15,my+15,2
	
					UpdateSparks()
					UpdateLightning()
					UpdateWorld() : RenderWorld
					Flip
				Wend
				ShowEntity guiball
				door_l_x=0 : door_r_x=scr\w-ImageWidth(door_right)
				PlaySound door_snd
				Repeat
					Cls
					door_l_x=door_l_x-10
					door_r_x=door_r_x+10
					DrawImage door_left,door_l_x,0
					DrawImage door_right,door_r_x,0
					Flip
				Until door_l_x<=-dl_w
				Cls
				Gosub closedoors

				While MouseDown(1) : Wend
	
			Case 8
				If demoflag=True Then Gosub demoend
				PAKFreePak()
				ClearWorld()
				
		End Select
		clicked=0
		punkt=-1
		FlushMouse
				
		Select akt_diff
			Case 0 : aktdiff$="LEICHT"
			Case 1 : aktdiff$="MITTEL"
			Case 2 : aktdiff$="SCHWER"
		End Select
				
		Create2DSparks(2,mx+15,my+15,spark_blue)
		Create2DSparks(2,mx+15,my+15,spark_red)
		
		Create2DSparks(2,EntityX(guiball),EntityY(guiball),spark_blue)
		
		TurnEntity rad,0,0,-0.5
		TurnEntity rad2,0,0,-0.5
		TurnEntity guiball,1,1,1
		PositionEntity mz3d,mx+20,my+20,1
		PositionEntity mzlight,mx+25,my+25,2
		
		UpdateSparks()
		UpdateLightning()
		UpdateWorld() : RenderWorld
		
		Flip
	Forever
	
	; 3D Variablen
	ShowEntity tbl01
	ShowEntity tbl02
	ShowEntity pdl
	ShowEntity ball
	If nomirror=False Then ShowEntity mirror
	s=0 : z=0
	While z<map_w
		If block_msh(s,z)>0 Then ShowEntity block_msh(s,z)
		s=s+1
		If s=map_w : s=0 : z=z+1 : EndIf
	Wend
	For i=0 To 6
		If multiballs(i)>0 Then ShowEntity multiballs(i)
	Next
	
	CameraViewport guicam,0,0,0,0
	CameraViewport cam\cam,0,0,scr\w,scr\h
	
	FreeImage startscreen
	FreeImage credits
	FreeEntity guiball
	FreeEntity selector
	
	For i=0 To 2 : FreeImage diff(i) : Next
	game_esc=False
	MoveMouse scr\w/2,scr\h/2
	While MouseDown(1) : Wend
	FlushMouse
	If ChannelPlaying(bgchann) Then StopChannel bgchann
	Gosub start0
Return

;###########################################################
; Shopscreen
;###########################################################
.shop
	FlushKeys
	If ChannelPlaying(music_play) Then StopChannel(music_play)
	chann=PlaySound(shopentry)
	ChannelVolume chann,sound_vol#
	
	leveltime=180
	bg=LoadImage("dat\gfx\gfx\shop.jpg") : MaskImage bg,255,0,255
	
	bnext=LoadImage("dat\gfx\gfx\button_nextlevel.bmp") : MaskImage bnext,0,0,50
	bbuy=LoadImage("dat\gfx\gfx\button_buy.bmp") : MaskImage bbuy,0,0,50
	xsel=LoadAnimImage("dat\gfx\gfx\xselect.bmp",32,32,0,16)
	xseltime=MilliSecs()
	xselframe=0
	
	tx1=-30 : ty1=-30
	meldung$=""
	meldung_zeit=0
	
	; Item-Maus Offsets
	If scr\w>640 Then offset=10
	If scr\w>800 Then offset=20
		
	Repeat
		Cls
		mx=MouseX() : my=MouseY()
		If KeyHit(1) Then Exit
		
		If KeyHit(88)
			SaveBuffer(FrontBuffer(),"shot0"+Str(shotz)+".bmp")
			shotz=shotz+1
		EndIf
		
		x=(scr\w/2)-320
		y=(scr\h/2)-240
		iy=y+108 : ix=x+380
		DrawImage door_left,0,0
		DrawImage door_right,scr\w-dr_w,0
		DrawImage bg,x,y
		
		If MilliSecs()-xseltime>62
			xselframe=xselframe+1
			If xselframe>15 Then xselframe=0
			xseltime=MilliSecs()
		EndIf
				
		WriteBitmapFont(x+133,y+66,Str(scorepoints))
		
		If MilliSecs()-meldung_zeit>=5000
			meldung$=""
			meldung_zeit=0
		Else
			lang=Len(meldung$)
			tx=(scr\w/2)-((lang*char_w)/2)
			WriteBitmapFont(tx,y+400,meldung$)
		EndIf
		
		If mx>x+370 And my>y+422 And mx<x+631 And my<y+449 Then DrawImage bnext,x+370,y+420
		If mx>x+10 And my>y+422 And mx<x+272 And my<y+449 Then DrawImage bbuy,x+10,y+420
		
		For shopentries.tShop=Each tShop
			DrawImage shopentries\img,shopentries\x,shopentries\y
		Next
		
		For i=0 To last_item
			WriteBitmapFont(ix,iy,item$(i))
			iy=iy+char_h+2
		Next
		
		If MouseHit(1)
			For shopentries.tShop=Each tShop
				x1=shopentries\x
				y1=shopentries\y
				x2=x1+shopentries\w
				y2=y1+shopentries\h
				n$=shopentries\name$
				pr=shopentries\price
				If mx>x1 And my>y1+offset And mx<x2 And my<y2+(offset*2)
					If scorepoints>pr
						willbuy=shopentries\num
						tx1=x+224 : ty1=y1
					Else
						meldung$="Nicht genug Credits"
						meldung_zeit=MilliSecs()
					EndIf
				EndIf
			Next
			
			If mx>x+370 And my>y+422 And mx<x+631 And my<y+449 Then Exit
			If mx>x+10 And my>y+422 And mx<x+272 And my<y+449
				For shopentries.tShop=Each tShop
					If shopentries\num=willbuy
						If scorepoints>shopentries\price
							scorepoints=scorepoints-shopentries\price
							item$(last_item)=shopentries\name$
							curr_item=last_item
							last_item=last_item+1
							PlaySound shopentry
						EndIf
					EndIf
				Next
			EndIf
		EndIf
		
		DrawImage xsel,tx1,ty1,xselframe

		Create2DSparks(1,mx+15,my+15,spark_blue)
		Create2DSparks(2,mx+15,my+15,spark_red)
		
		TurnEntity rad,0,0,-0.5
		TurnEntity rad2,0,0,-0.5
		PositionEntity mz3d,mx+20,my+20,1
		PositionEntity mzlight,mx+25,my+25,2
		UpdateSparks()
		UpdateLightning()
		UpdateWorld() : RenderWorld 
		
		Flip
	Forever
	While MouseDown(1) : Wend
	CameraViewport guicam,0,0,0,0
	CameraViewport cam\cam,0,0,scr\w,scr\h
	
	If nomirror=False Then ShowEntity mirror
	FreeImage bg
	FreeImage bnext
	FreeImage bbuy
	FreeImage xsel
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
	HideEntity guicam
Return

.closedoors
	;Tueren schliessen
	PlaySound door_snd
	flag=0
	xl=-ImageWidth(door_left)
	xr=scr\w
	Repeat
		Cls
		DrawImage door_left,xl,0
		DrawImage door_right,xr,0

		If xl<0
			xl=xl+10
		EndIf
		If xr>scr\w-dr_w
			xr=xr-10
		Else
			flag=1
		EndIf
		Flip
	Until flag=1
	spy=scr\h/8
	spy2=spy
	For i=0 To 8
		Create2DSparks(20,scr\w/2,spy2,smoke)
		spy2=spy2+spy
	Next
Return

.start0
	warmup=MilliSecs()
	Repeat
		Cls
		
		If zeit-t=>1000
			t=zeit
			fps=fpstmp
			fpstmp=0
		End If
		fpstmp=fpstmp+1
		zeit=MilliSecs()
		
		UpdateSparks()
		UpdateSmoke()
		UpdateScore()
		UpdateLightning()
			
		UpdateWorld : RenderWorld
	
		Cls
		DrawImage loadingpic,0,0
	
		speed#=(60.0*start_speed#)/fps
		specialfx=fps/4
		Flip
		If MilliSecs()-warmup>3000 Then Exit
	Forever
	
	MoveMouse scr\w/2,scr\h/2
	
	starttime=MilliSecs()-3000
Return

;-----------------------------------------------------------
; Demo Ende
.demoend

	dend(0)=LoadImage("dat\gfx\gfx\demo_screen01.jpg")
	dend(1)=LoadImage("dat\gfx\gfx\demo_screen02.jpg")
	dend(2)=LoadImage("dat\gfx\gfx\demo_screen03.jpg")
	dend(3)=LoadImage("dat\gfx\gfx\demo_screen04.jpg")
	
	If scr\w>640 Then xx=(scr\w-640)/2 Else xx=0
	If scr\h>480 Then yy=(scr\h-480)/2 Else yy=0
	
	dendtime=MilliSecs()
	dendscreen=0
	
	Repeat
		Cls
		If MilliSecs()-dendtime>3000
			dendscreen=dendscreen+1
			If dendscreen>3 Then dendscreen=0
			dendtime=MilliSecs()
		EndIf
		
		DrawImage dend(dendscreen),xx,yy
		
		Flip
	Until MouseDown(1)
	
	PAKFreePak()
	ClearWorld()

	End
Return

;-----------------------------------------------------------
; Startmenue
.start
	CameraViewport cam\cam,0,0,0,0
	CameraViewport guicam,0,0,scr\w,scr\h
	
	; 3D Variablen
	; bestehende Geometrie hiden
	HideEntity tbl01
	HideEntity tbl02
	HideEntity pdl
	HideEntity ball
	If nomirror=False Then HideEntity mirror
	s=0 : z=0
	While z<map_w
		If block_msh(s,z)>0 Then HideEntity block_msh(s,z)
		s=s+1
		If s=map_w : s=0 : z=z+1 : EndIf
	Wend
	For i=0 To 6
		If multiballs(i)>0 Then HideEntity multiballs(i)
	Next
	
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
	
	;................................................
	
	FlushKeys : FlushMouse
	If ChannelPlaying(music_play) Then StopChannel(music_play)
	bgchann=PlayMusic("dat\sfx\mainmusic.mp3")
	ChannelVolume bgchann,music_vol#
	
	While MouseDown(1) : Wend

	startscreen=LoadImage("dat\gfx\gfx\start.bmp")
	credits=LoadImage("dat\gfx\gfx\credits2.bmp")
	MaskImage credits,150,150,150
	MaskImage startscreen,0,0,50	
	selector=LoadImage("dat\gfx\gfx\selector.bmp")
	
	; Mouseover Grafiken laden
	b_newgame=LoadImage("dat\gfx\gfx\b_newgame.bmp")		: MaskImage b_newgame,0,0,50
	b_skill=LoadImage("dat\gfx\gfx\b_skill.bmp")			: MaskImage b_skill,0,0,50
	b_loadgame=LoadImage("dat\gfx\gfx\b_loadgame.bmp")		: MaskImage b_loadgame,0,0,50
	b_savegame=LoadImage("dat\gfx\gfx\b_savegame.bmp")		: MaskImage b_savegame,0,0,50
	b_highscore=LoadImage("dat\gfx\gfx\b_highscore.bmp")	: MaskImage b_highscore,0,0,50
	b_iscore=LoadImage("dat\gfx\gfx\b_iscore.bmp")			: MaskImage b_iscore,0,0,50
	b_multiplayer=LoadImage("dat\gfx\gfx\b_multiplayer.bmp"): MaskImage b_multiplayer,0,0,50
	b_exit=LoadImage("dat\gfx\gfx\b_exit.bmp")				: MaskImage b_exit,0,0,50
	b_continue=LoadImage("dat\gfx\gfx\b_continue.bmp")		: MaskImage b_continue,0,0,50
	infowin=LoadImage("dat\gfx\gfx\infowindow.bmp")			: MaskImage infowin,0,0,50
	
	Gosub closedoors
	
	;................................................
	x=(scr\w/2)-320
	y=(scr\h/2)-240
	
	For i=-480 To y Step 10
		Cls
		DrawImage door_left,0,0
		DrawImage door_right,scr\w-dr_w,0
		DrawImage startscreen,x,i
		Flip
	Next
	
	; Exitflags
	; 0 = nichts
	; 1 = beendet
	; 2 = continue
	; 3 = newgame
	
	exitflag=0
	akt_diff=0
	aktdiff$="easy"
		
	Repeat
		Cls
		mx=MouseX() : my=MouseY()
		If KeyHit(1) Then End
		
		DrawImage door_left,0,0
		DrawImage door_right,scr\w-dr_w,0
		
		DrawImage startscreen,x,y
		WriteBitmapFont(x+400,y+110,aktdiff$)
		
		;.................
		; New Game
		If mx>x+310 And my>y+65 And mx<x+460 And my<y+100
			DrawImage b_newgame,x+310,y+65
			
			; Infofenster
			DrawImage infowin,0,0
			Color 255,255,255
			Text 15,20,"Startet ein neues Spiel",0,0
			;//
			
			If MouseHit(1)
				FlushMouse
				Select akt_diff
					Case 0
						speed#=0.3
						balls=5
						diffc=0
						gravity=1.0
					Case 1
						speed#=0.6
						balls=4
						diffc=1
						gravity=1.2
					Case 2
						speed#=0.9
						balls=3
						diffc=2
						gravity=1.5
				End Select

				scorepoints=0
				levelstart=True
				MoveMouse scr\w/2,scr\h/2
				z#=EntityZ(pdl) : d#=MeshDepth(pdl) : zd#=(z#+d#)
				leveltime=180
				CreateMap(level)
				start_speed#=speed#
				scorepoints=0
				
				; alte Werte zuruecksetzen
				For i=0 To 6
					If multiballs(i)>0 Then FreeEntity multiballs(i)
					multiballs(i)=0
				Next
				If MeshWidth(ball)>2 Then ScaleMesh ball,0.5,0.5,0.5
				
				ShowEntity ball
				performancecheck=MilliSecs()
				FlushMouse
				exitflag=3
				Exit

			EndIf
		End If ;//
		
		; Skill
		If mx>x+310 And my>y+100 And mx<x+460 And my<y+140
			DrawImage b_skill,x+310,y+100
					
			; Infofenster
			DrawImage infowin,0,0
			Color 255,255,255
			Text 15,20,"Verändern Sie durch",0,0 
			Text 15,35,"klicken den Schwierig-",0,0
			Text 15,50,"keitsgrad",0,0
			;//

			If MouseHit(1)
				FlushMouse
				akt_diff=akt_diff+1
				If akt_diff>2 Then akt_diff=0
				Select akt_diff
					Case 0 : aktdiff$="easy"
					Case 1 : aktdiff$="medium"
					Case 2 : aktdiff$="hard"
				End Select
			EndIf
		End If ;//
		
		; Load
		If mx>x+310 And my>y+140 And mx<x+460 And my<y+180
			DrawImage b_loadgame,x+310,y+140
						
			; Infofenster
			DrawImage infowin,0,0
			Color 255,255,255
			Text 15,20,"Ein früheres Spiel",0,0
			Text 15,35,"weiterspielen",0,0
			;//
			FlushMouse
		EndIf ;//
		
		; Save
		If mx>x+310 And my>y+180 And mx<x+460 And my<y+220
			DrawImage b_savegame,x+310,y+180
			
			; Infofenster
			DrawImage infowin,0,0
			Color 255,255,255
			Text 15,20,"Das aktuelle Spiel",0,0
			Text 15,35,"sichern",0,0
			;//
			FlushMouse
		EndIf ;//
		
		; Highscore
		If mx>x+310 And my>y+220 And mx<x+460 And my<y+260
			DrawImage b_highscore,x+310,y+220
			
			; Infofenster
			DrawImage infowin,0,0
			Color 255,255,255
			Text 15,20,"Die Bestenliste",0,0
			;//
			FlushMouse
		EndIf ;//
		
		; i.Score
		If mx>x+310 And my>y+260 And mx<x+460 And my<y+300
			DrawImage b_iscore,x+310,y+260
			
			; Infofenster
			DrawImage infowin,0,0
			Color 255,255,255
			Text 15,20,"Die Highscore im Internet",0,0
			;//
			FlushMouse
		EndIf ;//
		
		; Multiplayer
		If mx>x+310 And my>y+300 And mx<x+460 And my<y+340
			DrawImage b_multiplayer,x+310,y+300
			
			; Infofenster
			DrawImage infowin,0,0
			Color 255,255,255
			Text 15,20,"Gegen andere Spieler",0,0
			Text 15,35,"aus der ganzen Welt",0,0
			Text 15,50,"spielen",0,0
			Text 15,75,"Noch nicht implementiert!",0,0
			;//
			If MouseHit(1) Then Gosub multiplayer
			FlushMouse
		End If ;//
		
		; Exit
		If mx>x+310 And my>y+340 And mx<x+460 And my<y+380
			DrawImage b_exit,x+310,y+340
			
			; Infofenster
			DrawImage infowin,0,0
			Color 255,255,255
			Text 15,20,"Beendet das Spiel",0,0
			;//

			If MouseHit(1)
				exitflag=1
				Exit
			EndIf
		EndIf ;//
		
		; Continue
		If mx>x+310 And my>y+380 And mx<x+460 And my<y+420
			DrawImage b_continue,x+310,y+380
			
			; Infofenster
			DrawImage infowin,0,0
			Color 255,255,255
			Text 15,20,"Das Spiel fortsetzen",0,0
			;//
			FlushMouse
		EndIf ;//
		
		;.................
		; 3D Mauszeiger	
		Create2DSparks(2,mx+15,my+15,spark_blue)
		Create2DSparks(2,mx+15,my+15,spark_red)
		
		TurnEntity rad,0,0,-0.5
		TurnEntity rad2,0,0,-0.5
		PositionEntity mz3d,mx+20,my+20,1
		PositionEntity mzlight,mx+25,my+25,2
		
		UpdateSparks()
		UpdateLightning()
		
		UpdateWorld() : RenderWorld
		
		If demoflag
			Color 255,255,255
			Text scr\w-StringWidth("Demo")-10,10,"Demo",0,0
		EndIf
		
		Flip
	Forever
	
	; Bitmaps freigeben
	FreeImage b_newgame
	FreeImage b_skill
	FreeImage b_loadgame
	FreeImage b_savegame
	FreeImage b_highscore
	FreeImage b_iscore
	FreeImage b_multiplayer
	FreeImage b_exit
	FreeImage b_continue
	FreeImage startscreen
	FreeImage credits
	FreeImage selector
	FreeImage infowin
	
	;..................
	; 3D Variablen
	ShowEntity tbl01
	ShowEntity tbl02
	ShowEntity pdl
	ShowEntity ball
	If nomirror=False Then ShowEntity mirror
	s=0 : z=0
	While z<map_w
		If block_msh(s,z)>0 Then ShowEntity block_msh(s,z)
		s=s+1
		If s=map_w : s=0 : z=z+1 : EndIf
	Wend
	For i=0 To 6
		If multiballs(i)>0 Then ShowEntity multiballs(i)
	Next
	
	CameraViewport guicam,0,0,0,0
	CameraViewport cam\cam,0,0,scr\w,scr\h
	
	;..................
	If exitflag=1 Then End
	
	game_esc=False
	MoveMouse scr\w/2,scr\h/2
	While MouseDown(1) : Wend
	;FlushMouse
	If ChannelPlaying(bgchann) Then StopChannel bgchann
	Gosub start0

Return

Include "includes\startmenu.bb"
Include "includes\multiplayer.bb"		; Multiplayermodul