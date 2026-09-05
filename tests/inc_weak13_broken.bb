; Hilfsdatei fuer neg_weak13_include_file.bb — enthaelt absichtlich einen
; Fehler in Zeile 4, damit die Diagnose diese Datei nennen muss und nicht die
; einbindende.
Function Kaputt%(n%)
  Return GibtEsNichtB(n)
End Function
