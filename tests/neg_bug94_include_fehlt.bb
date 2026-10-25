; BUG-94 - ein fehlendes Include hinter einem Doppelpunkt ist ein Fehler,
; gemeldet hinter dem schliessenden Anfuehrungszeichen wie im Original
; ("a.bb":1:27 bei "Print 1 : Include \"nix.bb\"", gemessen).
Print 1 : Include "bug94\gibtsnicht.bb"
