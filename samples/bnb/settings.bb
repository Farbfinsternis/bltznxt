; settings.bb - options, high scores and the autosave.
; All three live next to the executable as plain text files.

Const CFG_FILE$   = "bloxnballs.cfg"
Const SCORE_FILE$ = "scores.dat"
Const SAVE_FILE$  = "save.dat"
Const SCORE_MAX   = 10

; options
Global optMusic# = 0.7          ; 0..1
Global optSound# = 0.9          ; 0..1
Global optMouse# = 1.0          ; mouse sensitivity, 0.25..3
Global optMirror = True         ; reflecting table floor
Global optWidth = 1280
Global optHeight = 720
Global optFullscreen = False
Global optLastName$ = ""

; high scores per skill level (0 easy, 1 normal, 2 hard)
Dim scoreName$(2, SCORE_MAX - 1)
Dim scoreValue(2, SCORE_MAX - 1)
Dim scoreLevel(2, SCORE_MAX - 1)

; autosave: the state at the start of the last level reached
Global saveValid = False
Global saveSkill, saveLevel, saveScore, saveLives

Function AppPath$(file$)
	Return SystemProperty("appdir") + file
End Function

Function Settings_Load()
	f = ReadFile(AppPath(CFG_FILE))
	If Not f Then Return
	While Not Eof(f)
		line$ = ReadLine(f)
		p = Instr(line, "=")
		If p
			key$ = Lower(Trim(Left(line, p - 1)))
			value$ = Trim(Mid(line, p + 1))
			Select key
			Case "music"      : optMusic = Clamp(Float(value), 0, 1)
			Case "sound"      : optSound = Clamp(Float(value), 0, 1)
			Case "mouse"      : optMouse = Clamp(Float(value), 0.25, 3)
			Case "mirror"     : optMirror = Int(value) <> 0
			Case "width"      : optWidth = Int(value)
			Case "height"     : optHeight = Int(value)
			Case "fullscreen" : optFullscreen = Int(value) <> 0
			Case "name"       : optLastName = Left(value, 12)
			End Select
		EndIf
	Wend
	CloseFile f
	If optWidth < 640 Or optHeight < 360 Then optWidth = 1280 : optHeight = 720
End Function

Function Settings_Save()
	f = WriteFile(AppPath(CFG_FILE))
	If Not f Then Return
	WriteLine f, "music=" + optMusic
	WriteLine f, "sound=" + optSound
	WriteLine f, "mouse=" + optMouse
	WriteLine f, "mirror=" + optMirror
	WriteLine f, "width=" + optWidth
	WriteLine f, "height=" + optHeight
	WriteLine f, "fullscreen=" + optFullscreen
	WriteLine f, "name=" + optLastName
	CloseFile f
End Function

; ---- high scores -----------------------------------------------------------

Function Scores_Load()
	For s = 0 To 2
		For i = 0 To SCORE_MAX - 1
			scoreName(s, i) = "---" : scoreValue(s, i) = 0 : scoreLevel(s, i) = 0
		Next
	Next
	f = ReadFile(AppPath(SCORE_FILE))
	If Not f Then Return
	While Not Eof(f)
		; skill;rank;name;score;level
		line$ = ReadLine(f)
		s = Int(Part$(line, 1)) : i = Int(Part$(line, 2))
		If s >= 0 And s <= 2 And i >= 0 And i < SCORE_MAX And Part$(line, 3) <> ""
			scoreName(s, i) = Part$(line, 3)
			scoreValue(s, i) = Int(Part$(line, 4))
			scoreLevel(s, i) = Int(Part$(line, 5))
		EndIf
	Wend
	CloseFile f
End Function

Function Scores_Save()
	f = WriteFile(AppPath(SCORE_FILE))
	If Not f Then Return
	For s = 0 To 2
		For i = 0 To SCORE_MAX - 1
			WriteLine f, s + ";" + i + ";" + scoreName(s, i) + ";" + scoreValue(s, i) + ";" + scoreLevel(s, i)
		Next
	Next
	CloseFile f
End Function

; rank (0..9) a score would get, or -1
Function Scores_Rank(skill, score)
	If score <= 0 Then Return -1
	For i = 0 To SCORE_MAX - 1
		If score > scoreValue(skill, i) Then Return i
	Next
	Return -1
End Function

Function Scores_Insert(skill, name$, score, level)
	r = Scores_Rank(skill, score)
	If r < 0 Then Return -1
	For i = SCORE_MAX - 1 To r + 1 Step -1
		scoreName(skill, i) = scoreName(skill, i - 1)
		scoreValue(skill, i) = scoreValue(skill, i - 1)
		scoreLevel(skill, i) = scoreLevel(skill, i - 1)
	Next
	scoreName(skill, r) = name : scoreValue(skill, r) = score : scoreLevel(skill, r) = level
	Scores_Save()
	Return r
End Function

; ---- autosave --------------------------------------------------------------

Function Save_Load()
	saveValid = False
	f = ReadFile(AppPath(SAVE_FILE))
	If Not f Then Return
	saveSkill = Int(ReadLine(f))
	saveLevel = Int(ReadLine(f))
	saveScore = Int(ReadLine(f))
	saveLives = Int(ReadLine(f))
	CloseFile f
	saveValid = saveLevel >= 1 And saveLives > 0 And saveSkill >= 0 And saveSkill <= 2
End Function

Function Save_Write(skill, level, score, lives)
	f = WriteFile(AppPath(SAVE_FILE))
	If Not f Then Return
	WriteLine f, skill
	WriteLine f, level
	WriteLine f, score
	WriteLine f, lives
	CloseFile f
	saveValid = True
	saveSkill = skill : saveLevel = level : saveScore = score : saveLives = lives
End Function

Function Save_Clear()
	If FileType(AppPath(SAVE_FILE)) = 1 Then DeleteFile AppPath(SAVE_FILE)
	saveValid = False
End Function

; ---- small helpers ---------------------------------------------------------

Function Clamp#(v#, lo#, hi#)
	If v < lo Then Return lo
	If v > hi Then Return hi
	Return v
End Function

; n-th field of a line separated by ";"
Function Part$(s$, n)
	For i = 1 To n - 1
		p = Instr(s, ";")
		If p = 0 Then Return ""
		s = Mid(s, p + 1)
	Next
	p = Instr(s, ";")
	If p Then Return Left(s, p - 1)
	Return s
End Function
