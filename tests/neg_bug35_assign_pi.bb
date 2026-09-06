; BUG-35, zweite Seite: eine Zuweisung an Pi. Vorher entstand daraus ein
; stummes "int var_pi = 0; var_pi = 3;", waehrend jede Lesestelle weiter das
; Builtin las - die Zuweisung verschwand spurlos. Pi ist ein reserviertes Wort,
; also ist das ein Syntaxfehler.
Pi = 3
Print Pi
