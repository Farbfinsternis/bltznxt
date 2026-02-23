; Milestone 34 — Sound Loading & Playback
; Headless verification: API compiles and handles missing file / no audio device.
; LoadSound returns 0 when audio device unavailable or file not found.
; PlaySound(0) returns 0 (no-op); StopChannel(0) is a no-op.

Local s% = LoadSound("beep.wav")
Print "LoadSound = " + Str(s)      ; no audio device or file → 0
Local ch% = PlaySound(s)
Print "PlaySound = " + Str(ch)     ; s=0 → ch=0
StopChannel ch
Print "StopChannel OK"
FreeSound s
Print "FreeSound OK"
Print "DONE"
