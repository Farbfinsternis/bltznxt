; BUG-151 - jedes Include gilt relativ zum Verzeichnis des Hauptprogramms,
; auch aus einer Datei in einem Unterordner heraus.
;
; Gemessen am Original (2026-09-17): blitzcc wechselt vor dem Parsen in das
; Verzeichnis der Hauptdatei. "inc_bug151\a.bb" bindet deshalb
; "inc_bug151\b.bb" ein, nicht "b.bb". Das Spiel blox-n-balls nutzt genau
; diese Form (includes\action.bb -> "includes\multiball.bb").
; Gegenstueck: neg_bug151_include_relativ.bb.

Include "inc_bug151\a.bb"
Print "Haupt"
