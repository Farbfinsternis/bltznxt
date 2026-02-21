.config
	While MouseDown(1) : Wend
	FlushMouse
	; Blox-n-Balls Konfig
	;-----------------------------------------------------------
	; Images laden
	bg=LoadImage("dat\gfx\gfx\konfig.jpg") : MaskImage bg,255,0,255
	bl=LoadImage("dat\gfx\gfx\konfig_ball.bmp")
	ok=LoadImage("dat\gfx\gfx\ok.bmp") : MaskImage ok,50,50,50
	eo=LoadImage("dat\gfx\gfx\extraop.bmp") : MaskImage eo,50,50,50
	sn=LoadImage("dat\gfx\gfx\sound.jpg") : MaskImage sn,255,0,255
	sl=LoadImage("dat\gfx\gfx\slider.bmp") : MaskImage sl,50,50,50
	in=LoadImage("dat\gfx\gfx\input.jpg") : MaskImage in,255,0,255

	button_snd=LoadSound("dat\sfx\blockhit01.wav")
	end_snd=LoadSound("dat\sfx\lose.wav")
	save_snd=LoadSound("dat\sfx\shopentry.wav")

	;-----------------------------------------------------------
	; Hauptschleife

	aktcheck=1
	
	scx=(scr\w/2)-320
	scy=(scr\h/2)-240
	xx=scx+340 : yy=scy+240
	
	Repeat
		Cls
		mx=MouseX() : my=MouseY()
		If KeyHit(1) Then Exit
	
		DrawImage bg,scx,scy
		Color 255,255,255
	
		If mx>scx+169 And my>scy+121 And mx<scx+219 And my<scy+171
			DrawImage bl,scx+169,scy+121
			If MouseHit(1) Then Gosub sound
		EndIf
	
		If mx>scx+92 And my>scy+185 And mx<scx+142 And my<scy+235
			DrawImage bl,scx+92,scy+185
			If MouseHit(1) Then Gosub inputconf
		EndIf
	
		If mx>scx+110 And my>scy+330 And mx<scx+160 And my<scy+381
			DrawImage bl,scx+106,scy+329
			If MouseHit(1)
				Gosub saveconfig
				While MouseDown(1) : Wend
				Exit
			EndIf
		EndIf
	
		If mx>scx+433 And my>scy+330 And mx<scx+487 And my<scy+381
			DrawImage bl,scx+431,scy+329
			If MouseHit(1)
				PlaySound end_snd
				While MouseHit(1) : Wend
				Exit
			EndIf
		EndIf
	
		If MouseDown(1) And mx>scx+143 And my>scy+450 And mx<scx+246 And my<scy+466
			While MouseDown(1) : Wend
			If showfps=True Then showfps=False Else showfps=True
		EndIf
	
		If MouseDown(1) And mx>scx+264 And my>scy+450 And mx<scx+452 And my<scy+466
			If fsaa=True Then fsaa=False Else fsaa=True
			While MouseDown(1) : Wend
		EndIf
	
		Color 0,0,0
	
		If windowed=True Then Rect scx+29,scy+456,7,7,1
		If showfps=True Then Rect scx+148,scy+456,7,7,1
		If fsaa=True Then Rect scx+269,scy+456,7,7,1
	
		Text 0,scr\h-60,mx+" "+my,0,0
		
		Flip
	Forever

	FreeImage bg
	FreeImage bl
	FreeImage ok
	FreeImage eo
	FreeImage sn
	FreeImage sl
	FreeImage in

	FreeSound button_snd
	FreeSound end_snd
	FreeSound save_snd

Return

;-----------------------------------------------------------
; Sound Prefs
.sound
	While MouseDown(1) : Wend
	PlaySound button_snd
	FlushMouse
	CreateButton(1,scx+170,scy+131,200,25)
	CreateButton(2,scx+170,scy+162,200,25)
	
	slw=16 : slh=26
	slider_sm_x#=Float(scx+310+(99*music_vol#))-10
	slider_ss_x#=Float(scx+310+(99*sound_vol#))-10
	slider_m_y#=scy+115+88
	slider_s_y#=scy+115+129
	
	Repeat
		Cls
		mx=MouseX() : my=MouseY()
		ms#=MouseXSpeed() : mspy#=MouseYSpeed()
		DrawImage bg,scx,scy
		If KeyHit(1) Then Exit
		
		slider_m_x#=Float(scx+310+(99*music_vol#))-10
		slider_s_x#=Float(scx+310+(99*sound_vol#))-10
				
		DrawImage sn,scx+155,scy+115
		
		If MouseHit(1) And mx>scx+310 And my>scy+206 And mx<scx+410 And my<scy+219 Then music_vol#=(Float(mx-scx)-310.0)/99.0
		If MouseHit(1) And mx>scx+310 And my>scy+248 And mx<scx+410 And my<scy+261 Then sound_vol#=(Float(mx-scx)-310.0)/99.0
						
		If MouseDown(1) And mx>slider_m_x And my>slider_m_y And mx<slider_m_x+slw And my<slider_m_y+slh
			music_vol=music_vol+(ms*0.01)
			If music_vol<0.0 Then music_vol=0.0
			If music_vol>1.0 Then music_vol=1.0
		EndIf
		If MouseDown(1) And mx>slider_s_x And my>slider_s_y And mx<slider_s_x+slw And my<slider_s_y+slh
			sound_vol=sound_vol+(ms*0.01)
			If sound_vol<0.0 Then sound_vol=0.0
			If sound_vol>1.0 Then sound_vol=1.0
		EndIf
		
		If GetMouse()=1 Then sel=CheckButton(mx,my)		
		Select sel
			Case 2
				If sound_vol#<0.2 Then sound_vol#=1.0 Else sound_vol#=0.0
			
			Case 1
				If music_vol#<0.2 Then music_vol#=1.0 Else music_vol#=0.0
		End Select
		sel=0
		
		If sound_vol#>0.0 Then DrawImage eo,scx+175,scy+167
		If music_vol#>0.0 Then DrawImage eo,scx+175,scy+136
		
		DrawImage sl,slider_m_x,slider_m_y
		DrawImage sl,slider_s_x,slider_s_y
				
		If mx>scx+168 And my>scy+319 And mx<scx+275 And my<scy+351
			DrawImage ok,scx+168,scy+318
			If MouseDown(1)
				PlaySound button_snd
				Exit
			EndIf
		EndIf
		DrawImage mz,mx,my
		Flip
	Forever
	While MouseDown(1) : Wend
Return

;-----------------------------------------------------------
; Eingabe
.inputconf
	PlaySound button_snd
	FlushMouse
	
	slw=16 : slh=26
	slider_m_x#=Float(scx+260+(99*(mouse_speed#*2)))-10
	slider_m_y#=scy+115+112
	
	Repeat
		Cls
		mx=MouseX() : my=MouseY()
		ms#=MouseXSpeed()
		If KeyHit(1) Then Exit
		DrawImage bg,scx,scy
		slider_m_x#=Float(scx+260+(99*(mouse_speed#*2)))-10
		
		If MouseHit(1) And mx>scx+260 And my>scy+224 And mx<scx+361 And my<scy+253 Then mouse_speed#=((Float(mx-scx)-260.0)/99.0)/2
		If MouseDown(1) And mx>slider_m_x And my>slider_m_y And mx<slider_m_x+slw And my<slider_m_y+slh
			mouse_speed#=mouse_speed#+(ms*0.01)
			If mouse_speed#<0.0 Then mouse_speed#=0.0
			If mouse_speed#>0.5 Then mouse_speed#=0.5
		EndIf

		DrawImage in,scx+155,scy+115
		DrawImage sl,slider_m_x,slider_m_y
		
		If mx>scx+168 And my>scy+319 And mx<scx+275 And my<scy+351
			DrawImage ok,scx+168,scy+318
			If MouseDown(1)
				PlaySound button_snd
				Exit
			EndIf
		EndIf
		DrawImage mz,mx,my
		Flip
	Forever
	While MouseDown(1) : Wend
Return

;-----------------------------------------------------------
; Konfig sichern
.saveconfig
	PlaySound save_snd
	outfile=WriteFile("blox.ini")
	If outfile
		WriteLine outfile,scr\w
		WriteLine outfile,"depth="+scr\d
		If limitframe=True Then WriteLine outfile,"limitframe"
		If autoadjust=True Then WriteLine outfile,"autoadjust"
		If nomirror=True Then WriteLine outfile,"nomirror"
		WriteLine outfile,"musicvolume="+music_vol#
		WriteLine outfile,"soundvolume="+sound_vol#
		WriteLine outfile,"mousespeed="+mouse_speed#
		If windowed=True Then WriteLine outfile,"windowed"
		If showfps=True Then WriteLine outfile,"showfps"
		If fsaa=True Then WriteLine outfile,"fsaa"
		CloseFile outfile
	Else
		RuntimeError "Fehler beim schreiben der Konfiguration"
	EndIf
	While MouseDown(1) : Wend
Return

;-----------------------------------------------------------
Function CreateButton(num,x,y,width,height)
	gad.tButton=New tButton
	gad\x=x
	gad\y=y
	gad\w=width
	gad\h=height
	gad\num=num
End Function

Function KillButton(num)
	For gad.tButton=Each tButton
		If gad\num=num
			Delete gad
			Exit
		EndIf
	Next
End Function

Function CheckButton(ix,iy)
	result=-1
	For gad.tButton=Each tButton
		x1=gad\x
		y1=gad\y
		x2=x1+gad\w
		y2=y1+gad\h
		If ix>x1 And iy>y1 And ix<x2 And iy<y2 Then result=gad\num
	Next
	Return result
End Function