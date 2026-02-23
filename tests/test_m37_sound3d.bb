; Milestone 37 — 3D Sound
; Headless verification: all API functions compile, guards handle invalid handles
; gracefully, and listener/channel state setters don't crash.
;
; Load3DSound returns 0 when the file doesn't exist or no audio device is present.
; All other calls with handle=0 or ch=0 are safe no-ops.

; Load3DSound — wraps LoadSound; returns 0 in headless/no-device mode
Local s% = Load3DSound("boom.wav")
Print "Load3DSound = " + Str(s)     ; 0 (headless / no file)

; SoundRange — guard: snd=0 < 1, safe no-op
SoundRange s, 1.0, 10.0
Print "SoundRange OK"

; WaitSound — ch=0 → ChannelPlaying(0)=0 → returns immediately
WaitSound 0
Print "WaitSound OK"

; Channel3DPosition / Channel3DVelocity — ch=0 guard
Channel3DPosition 0, 1.0, 0.0, 5.0
Channel3DVelocity 0, 0.0, 0.0, 0.0
Print "Channel3D OK"

; Listener API — no guards needed (writes to global floats)
ListenerPosition 10.0, 0.0, 5.0
ListenerOrientation 0.0, 0.0, -1.0, 0.0, 1.0, 0.0
ListenerVelocity 1.0, 0.0, 0.0
Print "Listener OK"

Print "DONE"
