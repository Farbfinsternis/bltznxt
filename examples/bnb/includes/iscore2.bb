; i.Score
;-----------------------------------------------------------
.iscore_sub
	x=(scr\w/2)-320
	y=(scr\h/2)-240
		
	;................................................
	; Resourcen laden
	isfnt=LoadFont("Arial",18,1)
		
	background=LoadImage("dat\gfx\gfx\iscorescreen.jpg")
	pointer=LoadImage("dat\gfx\gfx\pointer.bmp")
	
	easy1=LoadImage("dat\gfx\gfx\is_easy_get.bmp")
	easy2=LoadImage("dat\gfx\gfx\is_easy_send.bmp")
	medium1=LoadImage("dat\gfx\gfx\is_medium_get.bmp")
	medium2=LoadImage("dat\gfx\gfx\is_medium_send.bmp")
	hard1=LoadImage("dat\gfx\gfx\is_hard_get.bmp")
	hard2=LoadImage("dat\gfx\gfx\is_hard_send.bmp")
	
	is_back=LoadImage("dat\gfx\gfx\is_mainmenu.bmp")
	
	;................................................
	; Images maskieren
	MaskImage pointer,50,50,50
	
	MaskImage easy1,0,0,50
	MaskImage easy2,0,0,50
	MaskImage medium1,0,0,50
	MaskImage medium2,0,0,50
	MaskImage hard1,0,0,50
	MaskImage hard2,0,0,50
	
	MaskImage is_back,0,0,50
	
	SetFont isfnt
	
	; Banner herunterladen
	DrawImage background,x,y
	Text scr\w/2,scr\h/2,"Bitte warten, Verbindung zum Server wird hergestellt!",1,1
	Flip
	banner=LoadWebImage("http://"+sendit\url$+"/bnb/banner/banner0.jpg")
	
	;................................................
	; Hauptschleife
	Repeat
		mx=MouseX() : my=MouseY()
		If KeyHit(1) Then Exit
		
		Cls
		DrawImage background,x,y
		If demoflag Then DrawImage dimg,5,5
		If banner Then DrawImage banner,x,y
		
		;.....................................
		; Buttons checken
		; easy senden
		If mx>x+535 And my>y+169 And mx<x+640 And my<y+199
			DrawImage easy2,x+535,y+169
			If MouseHit(1) Then is_iScore(1,0)
		EndIf
		
		; medium senden
		If mx>x+535 And my>y+200 And mx<x+640 And my<y+230
			DrawImage medium2,x+535,y+200
			If MouseHit(1) Then is_iScore(1,1)
		EndIf
		
		; hard senden
		If mx>x+535 And my>y+231 And mx<x+640 And my<y+261
			DrawImage hard2,x+535,y+231
			If MouseHit(1) Then is_iScore(1,2)
		EndIf
		
		; easy holen
		If mx>x+535 And my>y+314 And mx<x+640 And my<y+344
			DrawImage easy1,x+535,y+314
			If MouseHit(1) Then is_iScore(0,0)
		EndIf
		
		; medium holen
		If mx>x+535 And my>y+346 And mx<x+640 And my<y+376
			DrawImage medium1,x+535,y+346
			If MouseHit(1) Then is_iScore(0,1)
		EndIf
		
		; hard holen
		If mx>x+535 And my>y+378 And mx<x+640 And my<y+408
			DrawImage hard1,x+535,y+378
			If MouseHit(1) Then is_iScore(0,2)
		EndIf
		
		; Hauptmenue
		If mx>x+10 And my>y+440 And mx<x+140 And my<y+470
			DrawImage is_back,x+10,y+440
			If MouseHit(1) Then Exit
		EndIf
		
		;...............................................
		; Liste schreiben
		x1=50 : x2=210 : x3=360 : x4=435 : y1=145
		For i=0 To 9
			If local_score(i)>0
				Text x+x1,y+y1,local_name$(i),0,1
				Text x+x2,y+y1,local_score(i),0,1
				Text x+x3,y+y1,local_level(i),0,1
				Select local_skill(i)
					Case 0 : Text x+x4,y+y1,"Leicht",0,1
					Case 1 : Text x+x4,y+y1,"Mittel",0,1
					Case 2 : Text x+x4,y+y1,"Schwer",0,1
				End Select
			EndIf
			y1=y1+30
		Next
		
		DrawImage pointer,mx,my
		Flip	
	Forever
	
	;...................................................
	; Resourcen freigeben
	FreeImage background
	FreeImage pointer
	
	FreeImage easy1
	FreeImage easy2
	FreeImage medium1
	FreeImage medium2
	FreeImage hard1
	FreeImage hard2
	
	FreeImage is_back
	
	FreeFont isfnt
	
	SetFont arial
Return

;-----------------------------------------------------------
; Funktionen
;-----------------------------------------------------------
;
; i.Score senden / empfangen
; mode=0 -> empfangen, mode=1 -> senden
;
Function is_iScore(mode,skill)
	FlushMouse
	; Spielstände laden
	Select skill
		Case 0 : scorefile$="dat\bl0.tga"
		Case 1 : scorefile$="dat\bl1.tga"
		Case 2 : scorefile$="dat\bl3.tga"
	End Select
	
	scoredata=ReadFile(scorefile$)
	If scoredata
		For i=0 To 9
			local_name$(i)=ReadLine(scoredata)
			local_score(i)=ReadInt(scoredata)
			local_level(i)=ReadInt(scoredata)
			local_skill(i)=skill
		Next
		CloseFile scoredata
	Else
		; Wenn das Scorefile nicht vorhanden ist
		; wird eine leere Score erstellt
		For i=0 To 9
			local_name$(i)=""
			local_score(i)=0
			local_level(i)=0
			local_skill(i)=skill
		Next
	EndIf
	
	;................................................
	
	script$="/bnb/getscore.php"
	senden$="?"
	
	; wenn gelesen werden soll, ein leeres Parameterfeld
	; an das Script schicken
	If mode=0
		For i=0 To 9
			senden$=senden$+"name"+Str(i)+"=nobody"
			senden$=senden$+"&iscore"+Str(i)+"=0"
			senden$=senden$+"&skill"+Str(i)+"="+Str(skill)
			senden$=senden$+"&level"+Str(i)+"=0"
			If i<9 Then senden$=senden$+"&"
		Next
	EndIf
	If mode=1
		For i=0 To 9
			senden$=senden$+"name"+Str(i)+"="+local_name$(i)
			senden$=senden$+"&iscore"+Str(i)+"="+local_score(i)
			senden$=senden$+"&skill"+Str(i)+"="+skill
			senden$=senden$+"&level"+Str(i)+"="+local_level(i)
			If i<9 Then senden$=senden$+"&"
		Next
	EndIf
	
	;................................................
	; Score verschicken
	is_online=OpenTCPStream(sendit\url$,sendit\port)
	If is_online
		success=True
		req$="http://"+sendit\url$+script$+senden$
		
		DebugLog req$
		
		; Request an das Serverscript schicken
		WriteLine is_online,"GET "+req$+" HTTP/1.1"
		WriteLine is_online,"Host: "+sendit\host$
		WriteLine is_online,"User-Agent: bnb" 
		WriteLine is_online,"Accept: */*" 
		WriteLine is_online,""	
		
		; Daten empfangen
		Repeat
			trash$=ReadLine(is_online)
			DebugLog trash$
		Until Left(trash$,5)="BEGIN"
		
		For i=0 To 10
			local_name$(i)=ReadLine(is_online)
			local_score(i)=Int(ReadLine(is_online))
			local_skill(i)=Int(ReadLine(is_online))
			local_level(i)=Int(ReadLine(is_online))
		Next
		
		CloseTCPStream is_online
	EndIf
End Function