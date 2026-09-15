; BUG-42: Restore liest nur einen Bezeichner. ".daten" ist dahinter die
; naechste Anweisung - eine zweite Definition der Marke weiter unten.
Restore .daten
Read a
.daten
Data 5
