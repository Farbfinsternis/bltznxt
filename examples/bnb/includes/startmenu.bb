; Startmenu 3D
;-----------------------------------------------------------
.mainmenu
	CameraViewport cam\cam,0,0,0,0
	CameraViewport guicam,0,0,scr\w,scr\h
	
	FlushKeys : FlushMouse
	If ChannelPlaying(music_play) Then StopChannel(music_play)
	bgchann=PlayMusic("dat\sfx\mainmusic.mp3")
	ChannelVolume bgchann,music_vol#
	
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
	
	;..........................................................
	; Ressourcen laden
	
	monitor=LoadMesh("dat\gfx\msh\mon.3ds")
	mlight=CreateLight(2)
	monpiv=CreatePivot()
	
	PositionEntity monpiv,5,0,10
	
	PositionEntity monitor,3,-1,10
	PositionEntity mlight,0,6,0
	
	ScaleMesh monitor,0.2,0.2,0.2
	RotateEntity monitor,0,-90,0
	EntityParent monitor,monpiv
	
	inlight=CreateLight(2,monitor)
	LightColor inlight,0,255,0
	PositionEntity inlight,EntityX(monitor),EntityY(monitor)+1,EntityZ(monitor)
	
	Gosub rotateMonitor
	
	;..........................................................
	Repeat
		Cls
		If KeyHit(1) Then Exit
		
		DrawImage door_left,0,0
		DrawImage door_right,scr\w-dr_w,0
		
		LightMesh monitor,255,255,255,100,0,6,0
		UpdateWorld : RenderWorld
		
		Flip
	Forever
	End
Return

.rotateMonitor
	i=0 
	While i<60
		If KeyHit(1) Then i=61
		Cls
		
		DrawImage door_left,0,0
		DrawImage door_right,scr\w-dr_w,0
		
		TurnEntity monpiv,0,1,0
		
		i=i+1
		UpdateWorld : RenderWorld
		Flip
	Wend
	FlushKeys : FlushMouse
Return 