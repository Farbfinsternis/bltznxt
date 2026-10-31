; convert_bbl - converts the level files of the original blox-n-balls
; (binary .bbl: 14 x 14 Int, then the level time as Int and the music path
; as a line) into the text format of Blox'n'Balls (see assets/levels/README.txt).
;
; Usage: convert_bbl <folder with lvlNN.bbl> <output folder>
;        convert_bbl F:\dev\Blitz3D\Games\bnb\dat\lvl ..\assets\levels

Const OLD_W = 14
Const OLD_H = 14

Dim cell(OLD_W - 1, OLD_H - 1)

args$ = Trim(CommandLine())
src$ = Word(args, 1)
dst$ = Word(args, 2)
If src = "" Or dst = ""
	Print "Usage: convert_bbl <folder with lvlNN.bbl> <output folder>"
	End
EndIf

count = 0
For n = 1 To 99
	file$ = src + "\lvl" + Right("0" + n, 2) + ".bbl"
	If FileType(file) <> 1 Then Exit
	If ConvertLevel(file, dst + "\level" + Right("0" + n, 2) + ".txt", n) Then count = count + 1
Next
Print count + " level(s) converted."
End

Function ConvertLevel(file$, out$, n)
	f = ReadFile(file)
	If Not f Then Print "Cannot read " + file : Return False
	For z = 0 To OLD_H - 1
		For x = 0 To OLD_W - 1
			cell(x, z) = ReadInt(f) + 1            ; old type, 0 = empty
		Next
	Next
	time = 180 : music$ = ""
	If FileSize(file) > OLD_W * OLD_H * 4
		time = ReadInt(f)
		music = ReadLine(f)
	EndIf
	CloseFile f

	; strip the path of the old music file: dat\sfx\music01.mp3 -> music01.mp3
	For i = Len(music) To 1 Step -1
		If Mid(music, i, 1) = "\" Then music = Mid(music, i + 1) : Exit
	Next

	; drop empty rows at the bottom (the player side)
	rows = OLD_H
	While rows > 0
		used = False
		For x = 0 To OLD_W - 1
			If cell(x, rows - 1) Then used = True
		Next
		If used Then Exit
		rows = rows - 1
	Wend

	o = WriteFile(out)
	If Not o Then Print "Cannot write " + out : Return False
	WriteLine o, "; Blox'n'Balls level, converted from the original lvl" + Right("0" + n, 2) + ".bbl"
	WriteLine o, "name: Level " + n
	WriteLine o, "time: " + time
	WriteLine o, "speed: " + (8 + n)
	WriteLine o, "music: " + Lower(music)
	WriteLine o, "grid:"
	For z = 0 To rows - 1
		row$ = ""
		For x = 0 To OLD_W - 1
			row = row + OldCode(cell(x, z))
		Next
		WriteLine o, row
	Next
	CloseFile o
	Print file + " -> " + out + " (" + OLD_W + " x " + rows + ")"
	Return True
End Function

; old block type (1..24) -> character of the new format
Function OldCode$(t)
	Select t
	Case 0  : Return "."
	Case 1  : Return "1"     ; normal block
	Case 2  : Return "P"     ; points 50
	Case 3  : Return "Q"     ; points 75
	Case 4  : Return "R"     ; points 100
	Case 5  : Return "M"     ; multiball +1
	Case 6  : Return "N"     ; multiball +2
	Case 7  : Return "O"     ; multiball +3
	Case 8  : Return "E"     ; extra ball
	Case 9  : Return "A"     ; atomic ball
	Case 10 : Return "S"     ; speed ball
	Case 11 : Return "B"     ; new blocks
	Case 12 : Return "s"     ; small paddle
	Case 13 : Return "t"     ; tiny ball
	Case 15 : Return "o"     ; bumper
	Case 16 : Return "|"     ; obstacle, cylinder
	Case 17 : Return "#"     ; obstacle, block
	Case 18 : Return "/"     ; obstacle, 45 degrees
	Case 19 : Return "W"     ; wide paddle
	Case 20 : Return "L"     ; slow motion
	Case 21 : Return "G"     ; big ball
	Case 22 : Return "X"     ; score x2
	Case 23 : Return "T"     ; time +15
	Case 24 : Return "U"     ; time -15
	End Select
	Return "."                 ; 14 (unused in the original) and unknown types
End Function

; n-th word of s$, words separated by blanks
Function Word$(s$, n)
	s = Trim(s)
	For i = 1 To n - 1
		p = Instr(s, " ")
		If p = 0 Then Return ""
		s = Trim(Mid(s, p + 1))
	Next
	p = Instr(s, " ")
	If p Then Return Left(s, p - 1)
	Return s
End Function
