; level.bb - reads the level files (format: assets/levels/README.txt).

Const LEVEL_MAX_ROWS = 40

Global lvName$, lvTime, lvSpeed#, lvMusic$
Global lvCols, lvRows
Dim lvRow$(LEVEL_MAX_ROWS - 1)

Function Level_File$(n)
	Return "assets/levels/level" + Right("0" + n, 2) + ".txt"
End Function

; number of levels: level01, level02, ... without gaps
Function Level_Count()
	n = 0
	While FileType(Level_File(n + 1)) = 1
		n = n + 1
	Wend
	Return n
End Function

Function Level_Load(n)
	lvName = "Level " + n : lvTime = 180 : lvSpeed = 10 : lvMusic = ""
	lvCols = 0 : lvRows = 0
	f = ReadFile(Level_File(n))
	If Not f Then Return False
	inGrid = False
	While Not Eof(f)
		line$ = ReadLine(f)
		If Left(Trim(line), 1) = ";" Then line = ""
		If inGrid
			line = Trim(line)
			If line <> "" And lvRows < LEVEL_MAX_ROWS
				lvRow(lvRows) = line
				lvRows = lvRows + 1
				If Len(line) > lvCols Then lvCols = Len(line)
			EndIf
		Else
			p = Instr(line, ":")
			If p
				key$ = Lower(Trim(Left(line, p - 1)))
				value$ = Trim(Mid(line, p + 1))
				Select key
				Case "name"  : lvName = value
				Case "time"  : lvTime = Int(value)
				Case "speed" : lvSpeed = Float(value)
				Case "music" : lvMusic = value
				Case "grid"  : inGrid = True
				End Select
			EndIf
		EndIf
	Wend
	CloseFile f
	If lvTime < 10 Then lvTime = 10
	If lvSpeed < 4 Then lvSpeed = 4
	Return lvRows > 0
End Function

; cell code at column c, row r ("." outside the grid)
Function Level_Cell$(c, r)
	If r < 0 Or r >= lvRows Then Return "."
	If c < 0 Or c >= Len(lvRow(r)) Then Return "."
	Return Mid(lvRow(r), c + 1, 1)
End Function
