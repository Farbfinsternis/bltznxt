; blox-n-balls Multiplayer
; Version 0.4
;-----------------------------------------------------------

.multiplayer
	FlushMouse
	x=(scr\w/2)-320
	y=(scr\h/2)-240
	
	browser=LoadImage("dat\gfx\gfx\browser.png") : MaskImage browser,255,0,255
	mp_busy=LoadAnimImage("dat\gfx\gfx\busy.png",200,150,0,16)
	
	; // Status anzeigen	
	Cls
	DrawImage browser,x,y
	mp_Status("Laden...",x,y)
	DrawImage mp_busy,(scr\w/2)-64,(scr\h/2)-64
	Flip
	; //
	
	mp_newgame=LoadImage("dat\gfx\gfx\mp_newgame.bmp")
	mp_newlist=LoadImage("dat\gfx\gfx\mp_nlist.bmp")
	mp_connect=LoadImage("dat\gfx\gfx\mp_connect.bmp")
	mp_pointer=LoadImage("dat\gfx\gfx\pointer.bmp") : MaskImage mp_pointer,50,50,50
	mp_pcreategame=LoadImage("dat\gfx\gfx\p_creategame.bmp") : MaskImage mp_pcreategame,255,0,0
	mp_cancel=LoadImage("dat\gfx\gfx\b_p_cancel.bmp")
	mp_ok=LoadImage("dat\gfx\gfx\b_p_ok.bmp")
	mp_check=LoadImage("dat\gfx\gfx\mp_check.bmp")
	mp_error=LoadImage("dat\gfx\gfx\p_error.bmp")
	mp_agree=LoadImage("dat\gfx\gfx\b_p_agree.bmp")
	mp_ready=LoadImage("dat\gfx\gfx\b_ready.bmp")
	mp_gamechat=LoadImage("dat\gfx\gfx\gamechat.png") : MaskImage mp_gamechat,255,0,255
	mp_quit=LoadImage("dat\gfx\gfx\b_quit.bmp") : MaskImage mp_quit,255,0,255
	mp_user=LoadImage("dat\gfx\gfx\nickinput.png") : MaskImage mp_user,255,0,255
	mp_userlist=LoadImage("dat\gfx\gfx\userlist.png") : MaskImage mp_userlist,0,0,50
	mp_userlist2=LoadImage("dat\gfx\gfx\userlist2.png") : MaskImage mp_userlist2,0,0,50
	
	mpfont=LoadFont("Arial",16)
	SetFont mpfont
	
	detailfont=LoadFont("Arial",12,True)
	
	strh=StringHeight("X")
	mp_ctime=MilliSecs()
	mp_busy_frame=0
	
	exception=0
	Gosub username
	If exception>0 Then Goto exit_multiplayer
	
	mp_users.tGlobal=New tGlobal
	mp_users\username$=mp_nickname$
	
	; Userlist-Variablen
	If scr\w<1024
		ulx=-135
		uly=(scr\h-400)/2
	Else
		ulx=0
		uly=(scr\h-400)/2
	EndIf
	
	ul_move=0
	
	;................................................
	Repeat
		Cls
		If KeyHit(1) Then Exit
		DrawImage browser,x,y
				
		;...............................................
		; Gameliste anzeigen
		
		Color 255,255,255
		yy=y+60 : i=0
		dx=x+50 : dy=y+375
		For glist.tGList=Each tGlist
			If Len(glist\name$)>2
				If mx>x+60 And my>yy And mx<x+590 And my<yy+strh
					Text scr\w/2,y+450-strh,"Liste der gerade laufenden Spiele",1,0
					
					If i=mp_sel_game Then Color 255,0,0 Else Color 255,255,0
					If MouseHit(1)
						mp_sel_game=i
						mp_selected_name$=glist\name$
						mp_selected_ip$=glist\ip$
						mp_selected_desc$=glist\desc$
						mp_selected_iscore=glist\iscore
						mp_selected_pwd$=glist\pwd$
						FlushMouse
					EndIf
				Else
					If i=mp_sel_game Then Color 255,0,0 Else Color 255,255,255
				EndIf
				Text x+60,yy,glist\name$,0,0
				Text x+210,yy,glist\desc$,0,0
			
				If glist\pwd$<>""
					Text x+410,yy,"Ja",0,0
				Else
					Text x+410,yy,"Nein",0,0
				EndIf
				
				If glist\player>=2
					Text x+510,yy,"Spiel ist voll",0,0
				Else
					Text x+510,yy,"Host wartet",0,0
				EndIf
				
				; Details anzeigen
				If i=mp_sel_game
					Color 255,255,255
					SetFont detailfont
					; Anzahl der Spieler
					Select glist\player
						Case 0
							Text dx,dy,"Kein Spieler im Spiel, es wurde beendet",0,0
						Case 1
							Text dx,dy,"Es befindet sich "+glist\player+" Spieler im Spiel",0,0
						Case 2
							Text dx,dy,"Es befinden sich "+glist\player+" Spieler im Spiel",0,0
					End Select
					dy=dy+strh
					
					; Server IP
					If glist\player>0
						Text dx,dy,"Server IP: "+glist\ip$,0,0
					Else
						Text dx,dy,"Server IP: ---.---.---.---",0,0
					EndIf 
					dy=dy+strh
					
					; i.Score
					If glist\iscore
						Text dx,dy,"Das Spiel wird in der i.Score geführt",0,0
					Else
						Text dx,dy,"Das Spiel wird nicht in der i.Score geführt",0,0
					EndIf
					dy=dy+strh
					
					; Passwort
					If glist\pwd$<>""
						Text dx,dy,"Das Spiel ist privat",0,0
					Else
						Text dx,dy,"Das Spiel ist öffentlich",0,0
					EndIf
					dy=dy+strh
					SetFont mpfont
				EndIf
				; //
				
				yy=yy+strh : i=i+1
			EndIf
		Next
		
		;...............................................
		; Buttons checken
		mx=MouseX() : my=MouseY()
		
		; Neue Liste
		If mx>x+30 And my>y+450 And mx<x+160 And my<y+480
			DrawImage mp_newlist,x+30,y+450
			Text scr\w/2,y+450-strh,"Neue Spieleliste vom Server abrufen",1,0
			
			If MouseHit(1) 
				FlushMouse
				mp_busy_time=MilliSecs()
				result=mp_GetList(x,y)
			EndIf
		EndIf
		
		; Spiel erstellen
		If mx>x+255 And my>y+450 And mx<x+385 And my<y+480
			DrawImage mp_newgame,x+255,y+450
			Text scr\w/2,y+450-strh,"Ein neues Spiel erstellen",1,0
			
			If MouseHit(1)
				FlushMouse
				mp_CreateGame(x,y)
			EndIf
		End If
		
		; Verbinden
		If mx>x+480 And my>y+450 And mx<x+610 And my<y+480
			DrawImage mp_connect,x+480,y+450
			Text scr\w/2,y+450-strh,"Mit dem gerade gewählten Spiel verbinden",1,0
			
			If MouseHit(1)
				FlushMouse
				mp_ConnectGame(x,y)
			EndIf
		EndIf
				
		; Exit
		If mx>x+584 And my>y+4 And mx<x+606 And my<y+26
			DrawImage mp_quit,x+584,y+4
			Text scr\w/2,y+450-strh,"Den Multiplayermodus verlassen",1,0
			
			If MouseHit(1) : FlushMouse : Exit : EndIf
		EndIf
		
		; Userliste
		DrawImage mp_userlist,ulx,uly
		If mx>ulx+135 And my>uly And mx<ulx+160 And my<uly+100
			DrawImage mp_userlist2,ulx,uly
			
			If ulx>=0
				Text scr\w/2,y+450-strh,"Die Liste der User, die derzeit online sind, schliessen.",1,0
			Else
				Text scr\w/2,y+450-strh,"Die Liste der User, die derzeit online sind, öffnen.",1,0
			EndIf
			
			If MouseHit(1) And scr\w<1024 Then ul_move=1-ul_move
		EndIf
		If scr\w<1024
			If ul_move=0 And ulx>-135 Then ulx=ulx-10
			If ul_move=1 And ulx<0 Then ulx=ulx+10
		EndIf
		
		ty=uly+15
		For mp_users.tGlobal=Each tGlobal
			Text ulx+15,ty,mp_users\username$,0,0
			ty=ty+StringHeight(mp_users\username$)
		Next
		; //
				
		DrawImage mp_pointer,mx,my
		mp_Status("Bereit",x,y)
		
		; User begruessen
		user_welcome$="Herzlich willkommen "+mp_nickname$
		lang=StringWidth(user_welcome$)+4
		If scr\w<800
			Text scr\w/2,y+450-StringHeight("X"),user_welcome$,1,0
		Else
			Text scr\w/2,4,user_welcome$,1,0
		EndIf
		
		Flip
	Forever
		
	;................................................
	; Ressourcen freigeben
	.exit_multiplayer
	
	result=mp_SendName("/bnb/registernick.php",mp_nickname$,1,x,y)
	FlushMouse
	
	FreeImage browser
	FreeImage mp_newgame
	FreeImage mp_newlist
	FreeImage mp_connect
	FreeImage mp_pointer
	FreeImage mp_busy
	FreeImage mp_pcreategame
	FreeImage mp_cancel
	FreeImage mp_ok
	FreeImage mp_check
	FreeImage mp_error
	FreeImage mp_agree
	FreeImage mp_gamechat
	FreeImage mp_ready
	FreeImage mp_quit
	FreeImage mp_user
	FreeImage mp_userlist
	FreeImage mp_userlist2
	
	FreeFont mpfont
	SetFont arial
	
	mp_sel_game=-1
	mp_selected_name$=""
	mp_selected_ip$=""
	mp_selected_desc$=""
	mp_selected_iscore=""
	mp_selected_pwd$=""
	mp_ready_flag=False
	
	For mp_users.tGlobal=Each tGlobal
		If mp_users\username$=mp_nickname$ Then Delete mp_users
	Next
	FlushMouse
Return

Function mp_GetList(x,y)
	FlushMouse
	mp_sel_game=-1
	senden$="?typ=2"		
	mp_online=OpenTCPStream(sendit\url$,sendit\port)
	If mp_online
		Cls
		DrawImage browser,x,y
		mp_Status("Verbindung zum Server hergestellt",x,y)
		DrawImage mp_busy,(scr\w/2)-64,(scr\h/2)-64,mp_busy_frame
		If MilliSecs()-mp_busy_time>66
			mp_busy_frame=mp_busy_frame+1
			If mp_busy_frame>15 Then mp_busy_frame=0
			mp_busy_time=MilliSecs()
		EndIf
		Flip
		
		success=True
		For glist.tGList=Each tGList : Delete glist : Next
		req$="http://"+sendit\url$+sendit\script$+senden$
		
		; Request an das Serverscript schicken
		WriteLine mp_online,"GET "+req$+" HTTP/1.1"
		WriteLine mp_online,"Host: "+sendit\host$
		WriteLine mp_online,"User-Agent: blox'n'balls" 
		WriteLine mp_online,"Accept: */*" 
		WriteLine mp_online,""	
		; Muell auslesen und verwerfen
		
		Cls
		DrawImage browser,x,y
		mp_Status("Empfange Spieleliste",x,y)
		
		DrawImage mp_busy,(scr\w/2)-64,(scr\h/2)-64,mp_busy_frame
		If MilliSecs()-mp_busy_time>66
			mp_busy_frame=mp_busy_frame+1
			If mp_busy_frame>15 Then mp_busy_frame=0
			mp_busy_time=MilliSecs()
		EndIf
		
		Flip

		trash$=""
		Repeat
			trash$=ReadLine(mp_online)
			
			If MilliSecs()-mp_busy_time>66
				mp_busy_frame=mp_busy_frame+1
				If mp_busy_frame>15 Then mp_busy_frame=0
				mp_busy_time=MilliSecs()
				Cls
				DrawImage browser,x,y
				mp_Status("Empfange Spieleliste",x,y)
				DrawImage mp_busy,(scr\w/2)-64,(scr\h/2)-64,mp_busy_frame
				Flip
			EndIf
			
		Until Left(trash$,5)="BEGIN"
		
		While 1
			mp_name$=ReadLine(mp_online) : If Left(mp_name$,3)="END" Then Exit
			If Eof(mp_online) Then Exit
			glist.tGList=New tGlist
			glist\name$=mp_name$
			glist\desc$=ReadLine(mp_online)
			glist\pwd$=ReadLine(mp_online)
			If glist\pwd$="none" Then glist\pwd$=""
			glist\player=Int(ReadLine(mp_online))
			glist\ip$=ReadLine(mp_online)
			glist\iscore=Int(ReadLine(mp_online))
			 
			If MilliSecs()-mp_busy_time>66
				mp_busy_frame=mp_busy_frame+1
				If mp_busy_frame>15 Then mp_busy_frame=0
				mp_busy_time=MilliSecs()
				Cls
				DrawImage browser,x,y
				mp_Status("Empfange Spieleliste",x,y)
				DrawImage mp_busy,(scr\w/2)-64,(scr\h/2)-64,mp_busy_frame
				Flip
			EndIf

		Wend
		CloseTCPStream mp_online
	Else
		success=False
	EndIf
	Return success
End Function

;-----------------------------------------------------------
; Neues Spiel erstellen
Function mp_CreateGame(x,y)
	FlushMouse
	mp_game\ip$=mp_GetLocalIP$(1)
	Repeat
		Cls
		mx=MouseX() : my=MouseY()
		
		If KeyHit(1) Then Exit
		
		Cls
		DrawImage browser,x,y
		DrawImage mp_pcreategame,x+160,y+120
		
		;...............................................
		; Buttons checken
		; Spiel starten
		If mx>x+180 And my>y+330 And mx<x+280 And my<y+350
			DrawImage mp_ok,x+180,y+330
			If MouseHit(1) 
				result=mp_SendGame(x,y)
				If result Then Exit
			EndIf
		EndIf
		
		; Abbruch
		If mx>x+360 And my>y+330 And mx<x+455 And my<y+350
			DrawImage mp_cancel,x+360,y+330
			If MouseHit(1) Then Exit
		EndIf
		
		; Textfelder
		If MouseHit(1)
			; Name
			If mx>x+180 And my>y+165 And mx<x+460 And my<y+185
				mp_textfield=1
			EndIf
			; Beschreibung
			If mx>x+180 And my>y+215 And mx<x+460 And my<y+235
				mp_textfield=2
			EndIf
			; Passwort
			If mx>x+180 And my>y+300 And mx<x+460 And my<y+320
				mp_textfield=3
			EndIf
			
			; Checkbox i.Score
			If mx>x+440 And my>y+252 And mx<x+460 And my<y+272 Then mp_game\iscore=1-mp_game\iscore	
		EndIf

		;...............................................
		; eingegebenen Text und Check anzeigen
		Color 255,255,255
		If mp_game\iscore Then DrawImage mp_check,445,257
		Text x+182,y+167,mp_game\name$,0,0
		Text x+182,y+217,mp_game\desc$,0,0
		Text x+182,y+302,mp_game\pwd$,0,0
				
		;...............................................
		; Cursor zeigen
		Select mp_textfield
			Case 1
				cx=x+182 : cy=y+167
				cx=cx+StringWidth(mp_game\name$)
			Case 2
				cx=x+182 : cy=y+217
				cx=cx+StringWidth(mp_game\desc$)
			Case 3
				cx=x+182 : cy=y+302
				cx=cx+StringWidth(mp_game\pwd$)
		End Select

		If MilliSecs()-ctime>250
			mp_cflag=1-mp_cflag
			ctime=MilliSecs()
		EndIf
		If mp_cflag
			Color 255,180,0
			Rect cx,cy,StringWidth("X"),16,1
		EndIf
		
		;...............................................
		; Text annehmen
		key=GetKey()
		If key>31 And key<256
			Select mp_textfield
				Case 1 : mp_game\name$=mp_game\name$+Chr(key)
				Case 2 : mp_game\desc$=mp_game\desc$+Chr(key)
				Case 3 : mp_game\pwd$=mp_game\pwd$+Chr(key)
			End Select
		EndIf
		If key=KEY_ASCII_ENTER
			mp_textfield=mp_textfield+1
			If mp_textfield>3 Then mp_textfield=1
		EndIf
		
		If key=KEY_ASCII_BACKSPACE
			Select mp_textfield
				Case 1
					If Len(mp_game\name$)>0 Then mp_game\name$=Left(mp_game\name$,Len(mp_game\name$)-1)
				Case 2
					If Len(mp_game\desc$)>0 Then mp_game\desc$=Left(mp_game\desc$,Len(mp_game\desc$)-1)
				Case 3
					If Len(mp_game\pwd$)>0 Then mp_game\pwd$=Left(mp_game\pwd$,Len(mp_game\pwd$)-1)
			End Select
		EndIf
		
		If key=KEY_ASCII_TAB
			mp_textfield=mp_textfield+1
			If mp_textfield>3 Then mp_textfield=1
		EndIf
		
		;...............................................
		; Mauszeiger zeichnen
		DrawImage mp_pointer,mx,my
		Flip
	Forever
End Function

;-----------------------------------------------------------
; Spielparameter an das Script schicken
Function mp_SendGame(x,y)
	; Pruefen ob alle Werte korrekt sind
	fehler=0
	success=True
	exit_creategame=False
	If Len(mp_game\name$)<3 Then fehler=1
	If Len(mp_game\desc$)<3 Then fehler=2
	
	If fehler=0
		pwd$=mp_game\pwd$
		If Len(pwd$)<2 Then pwd$="none"
		mp_game\pwd$=pwd$
		
		senden$="?typ=1"
		senden$=senden$+"&name="+mp_URLCode$(mp_game\name$)
		senden$=senden$+"&desc="+mp_URLCode$(mp_game\desc$)
		senden$=senden$+"&pwd="+mp_URLCode$(mp_game\pwd$)
		senden$=senden$+"&max_player="+mp_game\player
		senden$=senden$+"&ip="+mp_game\ip$
		senden$=senden$+"&iscore="+mp_game\iscore
		
		Cls
		DrawImage browser,x,y
		mp_Status("Verbindung zum Server herstellen",x,y)
		DrawImage mp_busy,(scr\w/2)-64,(scr\h/2)-64
		Flip

		mp_online=OpenTCPStream(sendit\url$,sendit\port)
		If mp_online
			
			success=True
			For glist.tGList=Each tGList : Delete glist : Next
			req$="http://"+sendit\url$+sendit\script$+senden$
			; Request an das Serverscript schicken
			WriteLine mp_online,"GET "+req$+" HTTP/1.1"
			WriteLine mp_online,"Host: "+sendit\host$
			WriteLine mp_online,"User-Agent: bnb" 
			WriteLine mp_online,"Accept: */*" 
			WriteLine mp_online,""	
			; Muell auslesen und verwerfen
			
			Cls
			DrawImage browser,x,y
			mp_Status("Sende Spieldaten",x,y)
			DrawImage mp_busy,(scr\w/2)-64,(scr\h/2)-64
			Flip

			trash$=""
			Repeat
				trash$=ReadLine(mp_online)
				If MilliSecs()-mp_busy_time>66
					mp_busy_frame=mp_busy_frame+1
					If mp_busy_frame>15 Then mp_busy_frame=0
					mp_busy_time=MilliSecs()
					Cls
					DrawImage browser,x,y
					mp_Status("Sende Spieldaten",x,y)
					DrawImage mp_busy,(scr\w/2)-64,(scr\h/2)-64,mp_busy_frame
					Flip
				EndIf
			Until Left(trash$,5)="BEGIN"
			
			While 1
				mp_name$=ReadLine(mp_online) : If Left(mp_name$,3)="END" Then Exit
				If Eof(mp_online) Then Exit
				glist.tGList=New tGlist
				glist\name$=mp_name$
				glist\desc$=ReadLine(mp_online)
				glist\pwd$=ReadLine(mp_online)
				glist\player=Str(ReadLine(mp_online))
				glist\ip$=ReadLine(mp_online)
				
				If MilliSecs()-mp_busy_time>66
					mp_busy_frame=mp_busy_frame+1
					If mp_busy_frame>15 Then mp_busy_frame=0
					mp_busy_time=MilliSecs()
					Cls
					DrawImage browser,x,y
					mp_Status("Sende Spieldaten",x,y)
					DrawImage mp_busy,(scr\w/2)-64,(scr\h/2)-64,mp_busy_frame
					Flip
				EndIf
				
			Wend
			mp_gamecreated=True
			CloseTCPStream mp_online
		Else
			success=False
		EndIf
	Else
		; Fehlerbehandlung
		success=False
		flag=0
		Repeat
			mx=MouseX() : my=MouseY()
			Cls
			DrawImage browser,x,y
			DrawImage mp_error,x+160,y+120
			
			; Button checken
			If mx>x+270 And my>y+330 And mx<x+370 And my<y+350
				DrawImage mp_agree,270,330
				If MouseHit(1) Then flag=1
			EndIf
			
			Color 255,255,255
			Select fehler
				Case 1
					Text scr\w/2,scr\h/2,"Der Name für das Spiel muß länger als 3 Zeichen sein!",1,1
				Case 2
					Text scr\w/2,scr\h/2,"Die Beschreibung muß länger als 3 Zeichen sein!",1,1
			End Select
			
			DrawImage mp_pointer,mx,my
			Flip
			
		Until flag=1
	EndIf
	
	If success
		mp_GetList(x,y)
		mp_Chat(x,y)
	EndIf
	
	Return success
End Function

;-----------------------------------------------------------
; Mit einem Spiel verbinden
Function mp_ConnectGame(x,y)
	; Fehlerbehandlung
	flag=0
	Repeat
		mx=MouseX() : my=MouseY()
		Cls
		DrawImage browser,x,y
		DrawImage mp_error,x+160,y+120
		
		; Button checken
		If mx>x+270 And my>y+330 And mx<x+370 And my<y+350
			DrawImage mp_agree,x+270,x+330
			If MouseHit(1) Then flag=1
		EndIf
		
		; Fehlerbehandlung	
		Color 255,255,255
		If mp_selected_name$=""
			Text scr\w/2,scr\h/2,"Sie haben kein Spiel gewählt!",1,1
		Else
			mp_Chat(x,y)
			flag=1
		EndIf
		DrawImage mp_pointer,mx,my
		Flip
			
	Until flag=1
End Function

;-----------------------------------------------------------
; Vorbereitung auf das Spiel
Function mp_Chat(x,y)
	FlushMouse
	flag=0
	Repeat
		mx=MouseX() : my=MouseY()
		If KeyHit(1) Then flag=1
		Cls
		DrawImage mp_gamechat,x,y
		
		;...............................................
		; Buttons checken
		;
		; Exitbutton
		If mx>x+584 And my>y+4 And mx<x+606 And my<y+26
			DrawImage mp_quit,x+584,y+4
			If MouseHit(1)
				FlushMouse
				
				; Spiel loeschen wenn es erstellt wurde
				If mp_gamecreated Then mp_DeleteGame(x,y)
				mp_gamecreated=False
				mp_sel_game=-1
				mp_selected_name$=""
				mp_selected_ip$=""
				mp_selected_desc$=""
				mp_selected_iscore=""
				mp_selected_pwd$=""
				mp_ready_flag=False
				
				mp_game\name$=""
				mp_game\desc$=""
				mp_game\pwd$=""
				mp_game\iscore=0
				mp_game\ip$=""
				mp_game\player=0

				flag=1
			EndIf
		EndIf
		
		; Bereit
		If mx>x+480 And my>y+450 And mx<x+610 And my<y+480
			Color 255,255,255
			Text x+480,y+430,"Noch nicht implementiert",0,0
			
			If MouseHit(1) 
				mp_ready_flag=1-mp_ready_flag
				FlushMouse
			EndIf
		EndIf

		If mp_ready_flag Then DrawImage mp_ready,x+480,y+450
		DrawImage mp_pointer,mx,my
		
		If mp_gamecreated And mp_gameconnected=False Then mp_Status("Warten auf zweiten Spieler",x,y)
		If Not mp_gamecreated Then mp_Status("Verbunden mit "+mp_selected_name$,x,y)
		
		; Details anzeigen
		dx=x+50 : dy=y+375
		strh=StringHeight("X")
		Color 255,255,255
		SetFont detailfont
		
		If mp_gamecreated=False
			Text dx,dy,"Sie befinden sich im Spiel "+mp_selected_name$,0,0 : dy=dy+strh
			Text dx,dy,"Server IP: "+mp_selected_ip$,0,0 : dy=dy+strh
			Text dx,dy,"Beschreibung: "+mp_selected_desc$,0,0 : dy=dy+strh
			If mp_selected_iscore
				Text dx,dy,"Dieses Spiel wird in der i.Score geführt",0,0
			Else
				Text dx,dy,"Dieses Spiel wird nicht in der i.Score geführt",0,0
			EndIf
			dy=dy+strh
		Else
			Text dx,dy,"Ihr Spiel heisst "+mp_game\name$,0,0 : dy=dy+strh
			If mp_game\iscore
				Text dx,dy,"Dieses Spiel wird in der i.Score geführt",0,0
			Else
				Text dx,dy,"Dieses Spiel wird nicht in der i.Score geführt",0,0
			EndIf
			dy=dy+strh
			Text dx,dy,"Ihre IP lautet "+mp_game\ip$,0,0 : dy=dy+strh
			If Not mp_game\pwd$="none"
				Text dx,dy,"Dieses Spiel ist privat",0,0
			Else
				Text dx,dy,"Dieses Spiel ist öffentlich",0,0
			EndIf
			dy=dy+strh
		EndIf

		SetFont mpfont
		; //
		
		If scr\w>640
			Text scr\w/2,StringHeight("X")+4,"Herzlich willkommen "+mp_nickname$,1,0
		Else
			Text scr\w/2,y+450-StringHeight("X"),"Herzlich willkommen "+mp_nickname$,1,0
		EndIf
		
		Flip
	Until flag=1
End Function

;-----------------------------------------------------------
; Spiel loeschen
Function mp_DeleteGame(x,y)
	FlushMouse
	; Pruefen ob alle Werte korrekt sind
	senden$="?typ=3&name="+mp_URLCode(mp_game\name$)+"&ip="+mp_game\ip$
	mp_online=OpenTCPStream(sendit\url$,sendit\port)
	If mp_online
		Cls
		DrawImage browser,x,y
		mp_Status("Verbindung zum Server hergestellt",x,y)
		DrawImage mp_busy,(scr\w/2)-64,(scr\h/2)-64
		Flip
		
		success=True
		For glist.tGList=Each tGList : Delete glist : Next
		req$="http://"+sendit\url$+sendit\script$+senden$
		; Request an das Serverscript schicken
		WriteLine mp_online,"GET "+req$+" HTTP/1.1"
		WriteLine mp_online,"Host: "+sendit\host$
		WriteLine mp_online,"User-Agent: bnb" 
		WriteLine mp_online,"Accept: */*" 
		WriteLine mp_online,""	
				
		Cls
		DrawImage browser,x,y
		mp_Status("Sende Daten",x,y)
		DrawImage mp_busy,(scr\w/2)-64,(scr\h/2)-64
		Flip

		trash$=""
		While Not Eof(mp_online)
			trash$=ReadLine(mp_online)
			If MilliSecs()-mp_busy_time>66
				mp_busy_frame=mp_busy_frame+1
				If mp_busy_frame>15 Then mp_busy_frame=0
				mp_busy_time=MilliSecs()
				Cls
				DrawImage browser,x,y
				mp_Status("Lösche Spiel",x,y)
				DrawImage mp_busy,(scr\w/2)-64,(scr\h/2)-64,mp_busy_frame
				Flip
			EndIf
		Wend
		
		CloseTCPStream mp_online
	EndIf
End Function

;-----------------------------------------------------------
; Nick am Server registrieren
Function mp_SendName(script$,name$,typ,x,y)
	senden$="?nick="+name$+"&typ="+typ
	
	register_nick$=""
	
	; // Status anzeigen	
	Cls
	DrawImage browser,x,y
	mp_Status("Verbindung zum Server herstellen",x,y)
	DrawImage mp_busy,(scr\w/2)-64,(scr\h/2)-64
	Flip
	; //
	
	mp_online=OpenTCPStream(sendit\url$,sendit\port)
	If mp_online	
		req$="http://"+sendit\url$+script$+senden$
		
		; Request an das Serverscript schicken
		WriteLine mp_online,"GET "+req$+" HTTP/1.1"
		WriteLine mp_online,"Host: "+sendit\host$
		WriteLine mp_online,"User-Agent: bnb" 
		WriteLine mp_online,"Accept: */*" 
		WriteLine mp_online,""
		
		; Muell auslesen
		
		; // Status anzeigen	
		Cls
		DrawImage browser,x,y
		mp_Status("Anmeldung am Server",x,y)
		DrawImage mp_busy,(scr\w/2)-64,(scr\h/2)-64
		Flip
		; //
	
		Repeat
			trash$=ReadLine(mp_online)
		Until Left(trash$,5)="BEGIN"
		If typ=0		
			While Not Eof(mp_online)
				
				; // Status anzeigen	
				Cls
				DrawImage browser,x,y
				mp_Status("Anmeldung läuft...",x,y)
				DrawImage mp_busy,(scr\w/2)-64,(scr\h/2)-64
				Flip
				; //
	
				in$=ReadLine(mp_online)
				If in$="FEHLER" Then result=1
				If in$="END" And register_nick$="" Then result=1
				If in$="OK" : result=0 : Exit : EndIf
			Wend
		EndIf
		
		Repeat
			trash$=ReadLine(mp_online)
		Until Left(trash$,3)="END"
				
		CloseTCPStream mp_online	
	Else
		result=2
	EndIf
	FlushMouse
	Return result
End Function

;-----------------------------------------------------------
; Status anzeigen
Function mp_Status(mpstatus$,x,y)
	Color 255,255,255
	Text x+340,y+15,mpstatus$,0,1
End Function

;-------------------------------------------------
; IP-String in Integer umwandeln
Function mp_IntIP(IP$)
	a1=mp_ValString(Left(IP$,Instr(IP$,".")-1)):IP$=Right(IP$,Len(IP$)-Instr(IP$,"."))
	a2=mp_ValString(Left(IP$,Instr(IP$,".")-1)):IP$=Right(IP$,Len(IP$)-Instr(IP$,"."))
	a3=mp_ValString(Left(IP$,Instr(IP$,".")-1)):IP$=Right(IP$,Len(IP$)-Instr(IP$,"."))
	a4=mp_ValString(IP$)
	Return (a1 Shl 24)+(a2 Shl 16)+(a3 Shl 8 )+a4
End Function

;-------------------------------------------------
; String in numerischen Wert umrechnen
Function mp_ValString(strg$) 
	ac=0
	If Left(Trim(strg$),1)="-" : sign=-1 : Else : sign=1 : EndIf
	For i=1 To Len(strg$)
		b=Asc(Mid$(strg$,i,1))
		If b>47 And b<58
			hl = ac : hl=hl Shl 1 : ac=ac Shl 3 : ac=ac+hl + b - 48
		EndIf
		If b=46 Then Exit
	Next
	ac=ac*sign
	Return ac
End Function

;-----------------------------------------------------------
; lokale IP ermitteln
Function mp_GetLocalIP$(typ)
	ip_count=CountHostIPs(GetEnv("localhost"))
	If typ=0
		Return HostIP(1)
	Else
    	Return DottedIP(HostIP(1))
	EndIf
End Function

;-----------------------------------------------------------
; ASCII Codes durch URL-Codes ersetzen
Function mp_URLCode$(repl$)
	repl$=Replace(repl$,"%","%25")
	repl$=Replace(repl$,".","%2e")
	repl$=Replace(repl$,"?","%3e")
	repl$=Replace(repl$,"^","%5e")
	repl$=Replace(repl$,"~","%7e")
	repl$=Replace(repl$,"+","%2b")
	repl$=Replace(repl$,",","%2c")
	repl$=Replace(repl$,"/","%2f")
	repl$=Replace(repl$,":","%3a")
	repl$=Replace(repl$,";","%3b")
	repl$=Replace(repl$,"<","%3c")
	repl$=Replace(repl$,"=","%3d")
	repl$=Replace(repl$,">","%3d")
	repl$=Replace(repl$,"[","%5b")
	repl$=Replace(repl$,"\","%5c")
	repl$=Replace(repl$,"]","%5d")
	repl$=Replace(repl$,"{","%7b")
	repl$=Replace(repl$,"|","%7c")
	repl$=Replace(repl$,"}","%7d")
	repl$=Replace(repl$,Chr(9),"%09")
	repl$=Replace(repl$,Chr(32),"%20")
	repl$=Replace(repl$,"!","%21")
	repl$=Replace(repl$,Chr(34),"%22")
	repl$=Replace(repl$,"#","%23")
	repl$=Replace(repl$,"$","%24")
	repl$=Replace(repl$,"&","%26")
	repl$=Replace(repl$,"'","%27")
	repl$=Replace(repl$,"(","%28")
	repl$=Replace(repl$,")","%29")
	repl$=Replace(repl$,"@","%40")
	Return repl$
End Function

;-----------------------------------------------------------
; Benutzernamen eingeben
.username
	cx=x+190
	cy=y+180
	cx1=cx
	
	error$=""
	errortime=MilliSecs()
	
	Repeat
		Cls
		If KeyHit(1) Then Exit
		DrawImage browser,x,y
		DrawImage mp_user,x+150,y+150
		
		; Cursor anzeigen
		cx1=cx+StringWidth(mp_nickname$)
		If MilliSecs()-ctime>250
			mp_cflag=1-mp_cflag
			ctime=MilliSecs()
		EndIf
		If mp_cflag
			Color 255,180,0
			Rect cx1,cy,StringWidth("X"),16,1
		EndIf
		
		Color 255,255,255
		Text cx,cy,mp_nickname$,0,0
		
		; Text eingeben
		key=GetKey()
		If key>31 And key<255 Then mp_nickname$=mp_nickname$+Chr(key)
		
		If key=KEY_ASCII_BACKSPACE
			If Len(mp_nickname$)>0 Then mp_nickname$=Left(mp_nickname$,Len(mp_nickname$)-1)
		EndIf
		
		;...............................................
		; Am Server anmelden
		If key=KEY_ASCII_ENTER And Len(mp_nickname$)>=3
			result=mp_SendName("/bnb/registernick.php",mp_nickname$,0,x,y)
			Select result
				Case 1
					error$="Dieser Name ist bereits angemeldet"
				Case 2
					error$="Es konnte keine Verbindung zum Server hergestellt werden"
				Default
					Exit
			End Select
			If result>0
				errortime=MilliSecs()
				While MilliSecs()-errortime<5000
					Cls
					If KeyHit(1) Then Exit
					Text scr\w/2,scr\h/2,error$,1,1
					Flip
				Wend
				exception=1
				Exit
			EndIf	
		EndIf
		
		If key=KEY_ASCII_ENTER And Len(mp_nickname$)<3
			error$="Der Name muß mdst. 3 Zeichen lang sein!"
			errortime=MilliSecs()
		EndIf
		
		If MilliSecs()-errortime<5000
			Text scr\w/2,y+230,error$,1,0
		EndIf
		
		Flip
	Forever
Return