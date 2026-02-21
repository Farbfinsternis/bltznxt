; Onlinescore
SetFont isfnt

.iscore_sub
	x=(scr\w/2)-320
	y=(scr\h/2)-240
			
	exit_flag=0
	
	; iscore laden
	If diffc=0 Then ifile$="dat\sfx\il0.iff"
	If diffc=1 Then ifile$="dat\sfx\il1.iff"
	If diffc=2 Then ifile$="dat\sfx\il2.iff"

	infile=ReadFile(ifile$)
		
	If infile
		For i=0 To 9
			iscorename$(i)=ReadLine(infile)
			iscorescore(i)=ReadInt(infile)
			iscorelevel(i)=ReadInt(infile)
		Next
		CloseFile infile
	EndIf
	
	iscoredemo$="Das senden ist in der Demo nicht verfügbar!"
	isdw=StringWidth(iscoredemo$)
	isdh=StringHeight(iscoredemo$)
	
	sendoverlay=False
	
	; Banner herunterladen
	DrawImage iscreen,x,y
	Text scr\w/2,scr\h/2,"Bitte warten, Verbindung zum Server wird hergestellt!",1,1
	Flip
	banner=LoadWebImage("http://localhost/bnb/banner/banner0.jpg")
	
	Repeat
		Cls
		mx=MouseX() : my=MouseY()
		If KeyHit(1) Then Exit
				
		If KeyDown(42) And KeyHit(88) Then sendoverlay=True
		If KeyDown(54) And KeyHit(88) Then sendoverlay=True
		
		DrawImage iscreen,x,y
		If banner Then DrawImage banner,(scr\w/2)-(ImageWidth(banner)/2),0
		
		Color 255,255,255
		If mx>x+309 And my>y+423 And mx<x+392 And my<y+446
			; Score "easy" schicken
			If demoflag And sendoverlay=False
				Text scr\w-isdw-4,scr\h-isdh-4,iscoredemo$,0,0
			Else
				If MouseHit(1) Then iscore(0,1,x,y)
			EndIf
		EndIf
		If mx>x+414 And my>y+432 And mx<x+536 And my<y+446
			; Score "medium" schicken
			If demoflag And sendoverlay=False
				Text scr\w-isdw-4,scr\h-isdh-4,iscoredemo$,0,0
			Else
				If MouseHit(1) Then iscore(1,1,x,y)
			EndIf
		EndIf
		If mx>x+547 And my>y+423 And mx<x+638 And my<y+446
			; Score "hard" schicken
			If demoflag And sendoverlay=False
				Text scr\w-isdw-4,scr\h-isdh-4,iscoredemo$,0,0
			Else
				If MouseHit(1) Then iscore(2,1,x,y)
			EndIf
		EndIf
				
		If mx>x+309 And my>y+45 And mx<x+392 And my<y+70
			; Score "easy" holen
			If MouseHit(1) Then iscore(0,0,x,y)
		EndIf
		If mx>x+414 And my>y+45 And mx<x+536 And my<y+70
			; Score "medium" holen
			If MouseHit(1) Then iscore(1,0,x,y)
		EndIf
		If mx>x+547 And my>y+45 And mx<x+638 And my<y+70
			; Score "hard" holen
			If MouseHit(1) Then iscore(2,0,x,y)
		EndIf
		
		Color 255,255,255
		yy=y+108
		For k=0 To 9
			Text x+190,yy,iscorename$(k),0,0
			Text x+500,yy,iscorescore(k),0,0
			Text x+580,yy,iscorelevel(k),0,0
			yy=yy+30
		Next
				
		DrawImage imz,mx,my
		
		If MouseDown(1) And mx>x And my>y+450 And mx<x+150 And my<y+480 Then exit_flag=1
		
		Flip
	Until exit_flag=1
	
	FlushMouse
Return

Function iscore(skill,get_send,x1,y1)
	
	Color 255,255,255
	Cls
	DrawImage iscreen,x1,y1
	DrawImage istatus,x1+110,y1+210
	Text x1+190,scr\h/2,"connect to server...",0,1
	Flip
	
	online=OpenTCPStream(sendit\url$,sendit\port)
	If online
		; Scores lesen und zu sendende Messages zusammenstellen
		j=skill
		
		Cls
		DrawImage iscreen,x1,y1
		DrawImage istatus,x1+110,y1+210
		Text x+190,scr\h/2,"connected...",0,1
		Flip
		
		If j=0 Then scorefile$="dat\bl0.tga"
		If j=1 Then scorefile$="dat\bl1.tga"
		If j=2 Then scorefile$="dat\bl3.tga"
		
		If get_send=1
			Cls
			DrawImage iscreen,x1,y1
			DrawImage istatus,x1+110,y1+210
			Text x1+190,scr\h/2,"read local score...",0,1
			Flip
		EndIf
		
		infile=ReadFile(scorefile$)
		If infile
			For l=0 To 9
				scorenames$(l)=ReadLine(infile)
				scorevalues(l)=ReadInt(infile)
				scorelevel(l)=ReadInt(infile)
			Next
			CloseFile infile
		Else
			For l=0 To 9
				scorenames$(l)="nobody"
				scorevalues(l)=0
				scorelevel(l)=1
			Next
		EndIf
		send$="?"
		For i=0 To 9
			isname$=scorenames$(i)	: If isname$="" Then isname$="nobody"
			isscore=scorevalues(i)	: If isscore<0 Then isscore=0
			islevel=scorelevel(i)		: If islevel<=0 Then islevel=1
			send$=send$+"name"+i+"="+isname$+"&iscore"+i+"="+isscore+"&skill"+i+"="+j+"&level"+i+"="+islevel+"&"
		Next
		send$=Left(send$,Len(send$)-1)
		
		;If get_send=0 Then send$="?in=nobody&0|0|1|nobody|0|0|1|nobody|0|0|1|nobody|0|0|1|nobody|0|0|1|nobody|0|0|1|nobody|0|0|1|nobody|0|0|1|nobody|0|0|1|nobody|0|0|1"
		
		req$="http://"+sendit\url$+"/bnb/getscore.php"+send$
				
		Cls
		Text x1+190,scr\h/2,"send request...",0,1
		DrawImage iscreen,x1,y1
		DrawImage istatus,x1+110,y1+210
		Flip
		
		; Request an das Serverscript schicken
		WriteLine online,"GET "+req$+" HTTP/1.1"
		WriteLine online,"Host: "+sendit\host$
		WriteLine online,"User-Agent: blox'n'balls" 
		WriteLine online,"Accept: */*" 
		WriteLine online,""
		
		Cls
		DrawImage iscreen,x1,y1
		DrawImage istatus,x1+110,y1+210
		Text x1+190,scr\h/2,"incoming data...",0,1
		Flip
		
		; Muell auslesen und verwerfen
		Repeat
			trash$=ReadLine(online)
		Until Left(trash$,5)="BEGIN"
				
		; Muell auslesen und verwerfen
		k=0
		While Not Eof(online)
			isname$=ReadLine(online)
			If Left(isname$,3)="END" Then Exit
			isscore=ReadLine(online)
			isskill=ReadLine(online)
			islevel=ReadLine(online)
			iscorename$(k)=isname$
			iscorescore(k)=Int(isscore)
			iscoreskill(k)=Int(isskill)
			iscorelevel(k)=Int(islevel)
			k=k+1
		Wend
		
		; iscore sichern
		If skill=0 Then ofile$="dat\sfx\il0.iff"
		If skill=1 Then ofile$="dat\sfx\il1.iff"
		If skill=2 Then ofile$="dat\sfx\il2.iff"
		
		outfile=WriteFile(ofile$)
		
		If outfile
			For i=0 To 9
				WriteLine outfile,iscorename$(i)
				WriteInt outfile,iscorescore(i)
				WriteInt outfile,iscorelevel(i)
			Next
			CloseFile outfile
		EndIf
		
		CloseTCPStream online
		
		Cls
		DrawImage iscreen,x1,y1
		DrawImage istatus,x1+110,y1+210
		Text x1+190,scr\h/2,"ready...",0,1
		Flip
		
	Else
		; Es konnte keine Verbindung zum Server hergestellt werden
	EndIf 
	FlushMouse
End Function