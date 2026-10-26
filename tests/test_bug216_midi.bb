; BUG-216 - PlayMusic spielt MIDI-Dateien.
;
; Das Original spielt sie ueber FMOD; bei uns lehnte SDL_LoadWAV die Datei ab
; ("Could not find RIFF or WAVE identifiers"), und wing_ring blieb stumm.
; Jetzt laeuft MIDI unter Windows ueber den MCI-Sequencer. Gemessen an Blitz3D
; 11.8 mit tune1.mid aus Games/wing_ring (build/bug216_20261026/snd): dieselbe
; Ausgabe; die Datei hier ist ein einzelner Ton.
ch = PlayMusic("tests\assets\bug216_ton.mid")
Print ch <> 0
Delay 300
Print ChannelPlaying(ch)
StopChannel ch
Delay 100
Print ChannelPlaying(ch)
