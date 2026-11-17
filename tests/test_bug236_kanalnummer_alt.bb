; BUG-236 - eine alte Kanalnummer trifft keinen neuen Klang.
;
; Kanalnummern waren Platznummern: ein verklungener Klang gab seinen Platz
; frei, der naechste PlaySound bekam dieselbe Nummer, und ChannelPlaying,
; StopChannel, ChannelVolume auf die alte Nummer trafen den neuen Klang. Im
; Original vergibt gxAudio 4096 Kanalobjekte reihum (allocSoundChannel), und
; FMOD zaehlt in der Kanalnummer mit: die alte Nummer meldet 0.
wav$ = "tests\assets\klang_gleich.wav"
snd = LoadSound(wav)

alt = PlaySound(snd)
Print alt <> 0
Delay 1000
Print ChannelPlaying(alt)

LoopSound snd
neu = PlaySound(snd)
Print neu <> alt
Print ChannelPlaying(alt)
StopChannel alt              ; darf den neuen Klang nicht anhalten
PauseChannel alt
Print ChannelPlaying(neu)
StopChannel neu
Print ChannelPlaying(neu)

; Musik: nach dem Ende bleibt ihre Nummer bei 0, auch wenn ein Klang folgt
mus = PlayMusic(wav)
Delay 1000
fx = PlaySound(snd)
Print ChannelPlaying(mus)
StopChannel fx

; viele Klaenge hintereinander: jede Nummer ist neu
ok = 1
prev = 0
For i = 1 To 200
	c = PlaySound(snd)
	If c = prev Or c = 0 Then ok = 0
	StopChannel c
	If ChannelPlaying(c) Then ok = 0
	prev = c
Next
Print ok
Print "DONE"
