; Milestone 35 — Channel Control
; Headless verification: API compiles and handles ch=0 / no audio device gracefully.
; All channel functions are no-ops when ch=0 or no audio device is available.

Local s%  = LoadSound("beep.wav")   ; no audio → 0
Local ch% = PlaySound(s)            ; s=0      → 0

ChannelVolume ch, 0.5
Print "ChannelVolume OK"
ChannelPan ch, -0.5
Print "ChannelPan OK"
ChannelPitch ch, 440.0
Print "ChannelPitch OK"
PauseChannel ch
Print "PauseChannel OK"
ResumeChannel ch
Print "ResumeChannel OK"
Print "ChannelPlaying = " + Str(ChannelPlaying(ch))   ; ch=0 → 0

SoundVolume s, 0.8
Print "SoundVolume OK"
SoundPan s, 0.0
Print "SoundPan OK"
SoundPitch s, 44100.0
Print "SoundPitch OK"

StopChannel ch
Print "DONE"
