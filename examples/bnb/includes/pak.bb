;Pak-Functions
; V1.03
;-----------------------------------------------------------
Type tPak
	Field filename$
	Field sizeof
End Type
Global pak.tPak
Global decrypt=False

;-----------------------------------------------------------
; PAK erstellen
Function PAKMake(dir$,outfile$,ekey)
	; Verzeichnis einlesen
	rdir=ReadDir(dir$)
	pakfile=WriteFile(outfile$)
	If rdir
		olddir$=CurrentDir()
		ChangeDir dir$
		Repeat
			file$=NextFile(rdir)
			typ=FileType(file$)
			If typ=1
				If file$<>"" And file$<>" " And file$<>"." And file$<>".."
					pak.tPak=New tPak
					pak\filename$=file$
					pak\sizeof=FileSize(file$)
				EndIf
			EndIf
		Until typ=0
		CloseDir rdir

		For pak.tPak=Each tPak
			f$=pak\filename$
			sz=pak\sizeof-1
			WriteLine pakfile,f$
			WriteInt pakfile,sz
			infile=ReadFile(pak\filename$)
			While Not Eof(infile)
				byte=ReadByte(infile)
				If ekey>0 Then byte=byte Xor ekey
				WriteByte pakfile,byte
			Wend
			CloseFile infile
		Next
		CloseDir rdir
		CloseFile pakfile
		ChangeDir olddir$
	EndIf
End Function

;-----------------------------------------------------------
; PAK entpacken
Function PAKDePak(pakfile$,key)
	infile=ReadFile(pakfile$)
	If infile
		temp$=SystemProperty("tempdir")
		CreateDir temp$+"tmp"
		temp$=temp$+"tmp\"
		While Not Eof(infile)
			pak.tPak=New tPak
			name$=ReadLine(infile)
			pak\filename$=name$
			fsize=ReadInt(infile)
			pak\sizeof=fsize
			outfile=WriteFile(temp$+name$)
			For j=0 To fsize
				byte=ReadByte(infile)
				If key>0 Then byte=byte Xor key
				WriteByte outfile,byte
			Next
			CloseFile outfile
		Wend
		CloseFile infile
	EndIf		
End Function

;-----------------------------------------------------------
; File aus dem Pak laden
Function PAKLoadFile$(file$)
	temp$=SystemProperty("tempdir")
	temp$=temp$+"tmp\"
	Return temp$+file$		
End Function

;-----------------------------------------------------------
; Key erzeugen
Function PAKCreateKey(seed)
	SeedRnd seed
	For i=0 To 255
		key=Rnd(0,255)
	Next
	Return key
End Function

;-----------------------------------------------------------
; PAK aus dem Speicher befreien
Function PAKFreePak()
	wintemp$=SystemProperty("tempdir")
	wintemp$=wintemp$+"tmp\"
	SetBuffer FrontBuffer()
	For pak.tPak=Each tPak
		filename$=pak\filename$
		DeleteFile wintemp$+filename$
		Delete pak
	Next
End Function