;-----------------------------------------------------------
; Konfig laden
If FileType("blox.ini")=1
	infile=ReadFile("blox.ini")
	If infile
		While Not Eof(infile)
			commands$=ReadLine(infile)
			If Instr(commands$,"640")>0
				scr\w=640
				scr\h=480
			EndIf
			If Instr(commands$,"800")>0
				scr\w=800
				scr\h=600
			EndIf
			If Instr(commands$,"1024")>0
				scr\w=1024
				scr\h=768
			EndIf
			If Instr(commands$,"depth=")>0 Then scr\d=Int(Right(commands$,2))
			If Instr(commands$,"windowed")>0 Then scr\m=2
			If Instr(commands$,"nomirror")>0 Then nomirror=True
			If Instr(commands$,"showfps")>0 Then showfps=True
			If Instr(commands$,"autoadjust")>0 Then autoadjust=True
			If Instr(commands$,"limitframe")>0 Then limitframe=True
			If Instr(commands$,"mousespeed=")>0
				lang1=Len(commands$)
				lang2=Len("mousespeed=")
				mouse_speed#=Float(Right(commands$,lang1-lang2))
			EndIf
			If Instr(commands$,"musicvolume=")>0
				lang1=Len(commands$)
				lang2=Len("musicvolume=")
				music_vol#=Float(Right(commands$,lang1-lang2))
			EndIf
			If Instr(commands$,"soundvolume=")>0
				lang1=Len(commands$)
				lang2=Len("soundvolume=")
				sound_vol#=Float(Right(commands$,lang1-lang2))
			EndIf
			If Instr(commands$,"showfps") Then showfps=True
			If Instr(commands$,"fsaa") Then fsaa=True
		Wend
		If scr\w<640 Or scr\w>1024 Then scr\w=640
		If scr\h<480 Or scr\h>768 Then scr\h=480
		If scr\m<1 Or scr\m>2 Then scr\m=1
		If scr\d<16 Or scr\d>32 Then scr\d=16
		CloseFile infile
	EndIf
Else
	scr\w=640
	scr\h=480
	scr\d=16
EndIf

scr\title$="x-cellence blox'n'balls"