; BUG-44 - Signaturen, die gegen das laufende Original (blitzcc 1.108c, +k)
; richtiggestellt wurden. Geprueft wird hier, was ohne Grafik und ohne
; Joystick beobachtbar ist.

; --- Print ohne Argument gibt eine Leerzeile aus ---
; Im Original "Print [string$]". Wir haben das bis 2026-09-07 abgelehnt.
Print "vor der Leerzeile"
Print
Print "nach der Leerzeile"

; --- Floor und Ceil liefern float, nicht int ---
; Im Original "Floor# ( float# )" und "Ceil# ( float# )". Mit int als
; Rueckgabetyp wurde aus der folgenden Division eine Ganzzahldivision.
Print Floor(3.7)
Print Ceil(3.2)
; Die Teiler sind absichtlich Ganzzahlen: mit int als Rueckgabetyp waere
; das eine Ganzzahldivision (4/3 = 1 und 7/2 = 3), mit float nicht.
Local gedrittelt# = Ceil(3.2) / 3
Print gedrittelt
Local gehalbiert# = Floor(7.9) / 2
Print gehalbiert

; --- ReadBytes und WriteBytes: erst die Bank, dann die Datei ---
; Im Original "WriteBytes ( bank,file,offset,count )", und beide liefern die
; Zahl der uebertragenen Bytes. Bei uns standen bank und file vertauscht.
Local bank = CreateBank(8)
PokeInt bank, 0, 305419896
PokeInt bank, 4, 2018915346

Local aus = WriteFile("bug44_bytes.bin")
Local geschrieben = WriteBytes(bank, aus, 0, 8)
CloseFile aus
Print "geschrieben: " + Str(geschrieben)

Local leer = CreateBank(8)
Local ein = ReadFile("bug44_bytes.bin")
Local gelesen = ReadBytes(leer, ein, 0, 8)
CloseFile ein
Print "gelesen: " + Str(gelesen)
Print "wert 0: " + Str(PeekInt(leer, 0))
Print "wert 4: " + Str(PeekInt(leer, 4))

DeleteFile "bug44_bytes.bin"
Print "DONE"
