; BUG-235 - PlayMusic spielt WAV/MP3/OGG einmal; BlitzPlus-Flags.
;
; Gemessen an Blitz3D 1.108 (Pegelmesser am Ausgabegeraet): ein Stream laeuft
; einmal, danach meldet ChannelPlaying 0. MIDI wiederholt hoerbar, meldet aber
; nach dem ersten Durchlauf 0. BlitzPlus (audio.cpp) kennt
; PlayMusic(file$[,flags]) mit 1 = Wiederholen (nur MIDI/MOD), 2 = pausiert.
; Bei uns wiederholte jeder Stream endlos, und ChannelPlaying blieb 1 - eine
; Wiedergabeliste kam nie zum naechsten Lied.
wav$ = "tests\assets\klang_gleich.wav"
mid$ = "tests\assets\bug216_ton.mid"

; Blitz3D: Stream einmal
ch = PlayMusic(wav)
Print ch <> 0
Print ChannelPlaying(ch)
Delay 1000
Print ChannelPlaying(ch)

; der beendete Musikkanal bleibt belegt: ein neuer Klang bekommt eine andere
; Nummer, ChannelPlaying auf die alte Nummer bleibt 0
snd = LoadSound(wav)
fx = PlaySound(snd)
Print fx <> ch
Print ChannelPlaying(ch)
StopChannel fx

; BlitzPlus: Flag 2 startet pausiert, ResumeChannel spielt einmal
ch = PlayMusic(wav, 2)
Delay 1000
Print ChannelPlaying(ch)
ResumeChannel ch
Delay 1000
Print ChannelPlaying(ch)

; BlitzPlus: Flag 1 wiederholt keinen Stream (playStream)
ch = PlayMusic(wav, 1)
Delay 1000
Print ChannelPlaying(ch)
StopMusic

; MIDI (ein Ton, etwa 2 s): Blitz3D meldet nach dem ersten Durchlauf 0
ch = PlayMusic(mid)
Delay 300
Print ChannelPlaying(ch)
Delay 2700
Print ChannelPlaying(ch)

; MIDI BlitzPlus: Flag 1 wiederholt und meldet 1, Flag 0 endet mit 0
ch = PlayMusic(mid, 1)
Delay 3000
Print ChannelPlaying(ch)
ch = PlayMusic(mid, 0)
Delay 3000
Print ChannelPlaying(ch)
StopMusic
Print "DONE"
