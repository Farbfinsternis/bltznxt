; Milestone 36 — Music & CD
; Headless verification: API compiles and handles missing file / no audio device.
; PlayMusic returns 0 when file not found or no audio device is available.
; PlayCDTrack is a no-op stub (logs a warning to stderr).

Local ch% = PlayMusic("music.ogg")   ; no audio / no file → 0
Print "PlayMusic = " + Str(ch)
Print "MusicPlaying = " + Str(MusicPlaying())   ; ch=0 → 0
StopMusic
Print "StopMusic OK"
PlayCDTrack 1                        ; → warning on stderr, no crash
Print "PlayCDTrack OK"
Print "DONE"
