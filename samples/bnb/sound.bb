; sound.bb - sound effects and music (the files of the original game).

Global sndBlock, sndBlockHit, sndPaddle, sndWall, sndBumper, sndBonus
Global sndLoseBall, sndLose, sndWin, sndDoor, sndBomb, sndShop
Global musicChannel, musicFile$ = ""

Function Sound_Load()
	sndBlock    = LoadSound("assets/sounds/blockhit01.wav")
	sndBlockHit = LoadSound("assets/sounds/punch1.wav")
	sndPaddle   = LoadSound("assets/sounds/paddlehit01.wav")
	sndWall     = LoadSound("assets/sounds/boing.wav")
	sndBumper   = LoadSound("assets/sounds/bumper.wav")
	sndBonus    = LoadSound("assets/sounds/bonuspoints.wav")
	sndLoseBall = LoadSound("assets/sounds/loseball.wav")
	sndLose     = LoadSound("assets/sounds/lose.wav")
	sndWin      = LoadSound("assets/sounds/win.wav")
	sndDoor     = LoadSound("assets/sounds/door.wav")
	sndBomb     = LoadSound("assets/sounds/bomb.wav")
	sndShop     = LoadSound("assets/sounds/shopentry.wav")
	If sndWall Then SoundVolume sndWall, 0.5
End Function

Function Sfx(snd)
	If snd = 0 Or optSound <= 0 Then Return
	ch = PlaySound(snd)
	ChannelVolume ch, optSound
End Function

; plays assets/music/<file>; the same file keeps running
Function Music_Play(file$)
	If file = musicFile And ChannelPlaying(musicChannel) Then Return
	Music_Stop()
	musicFile = file
	If file = "" Or optMusic <= 0 Then Return
	If FileType("assets/music/" + file) <> 1 Then Return
	musicChannel = PlayMusic("assets/music/" + file)
	ChannelVolume musicChannel, optMusic
End Function

Function Music_Stop()
	If musicChannel Then StopChannel musicChannel
	musicChannel = 0
End Function

; restarts the music when it has ended (loop) and follows the volume option
Function Music_Update()
	If musicFile = "" Then Return
	If optMusic <= 0
		Music_Stop()
	ElseIf Not ChannelPlaying(musicChannel)
		musicChannel = PlayMusic("assets/music/" + musicFile)
	EndIf
	If musicChannel Then ChannelVolume musicChannel, optMusic
End Function
