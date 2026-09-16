; BUG-113 - RSet kuerzt einen zu langen String von links und behaelt das Ende.
;
; Referenz: bbRSet in bbruntime/bbstring.cpp - bei Ueberlaenge
; "s->substr( s->size()-n )", sonst von links mit Leerzeichen auffuellen.
; LSet behaelt dagegen den Anfang. Alle Werte sind am laufenden Original
; gemessen (2026-09-16); die Klammern machen die Leerzeichen sichtbar.

Print "[" + RSet("abcdef", 3) + "]"
Print "[" + RSet("abc", 3) + "]"
Print "[" + RSet("ab", 5) + "]"
Print "[" + RSet("abc", 0) + "]"
Print "[" + RSet("", 2) + "]"

Print "[" + LSet("abcdef", 3) + "]"
Print "[" + LSet("abc", 0) + "]"
Print "[" + LSet("ab", 5) + "]"

; Rechtsbuendige Zahlenspalte: die Einerstelle bleibt stehen
Print "[" + RSet(Str(123456), 4) + "]"
