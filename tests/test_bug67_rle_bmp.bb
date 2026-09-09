; BUG-67: BMP mit Lauflaengenkodierung (BI_RLE8 und BI_RLE4)
;
; stb_image liest BMP mit 1, 4, 8, 24 und 32 Bit je Bildpunkt, aber keine der
; beiden komprimierten Varianten. Das Original laedt seine Bilder ueber
; FreeImage 2.4.1, das sie kennt. Ohne den Nachweg gibt LoadImage 0 zurueck -
; und beim Laden eines Modells fallen alle Materialien mit fehlgeschlagener
; Textur zu einem einzigen Brush zusammen, der Fehler wird also an der
; Flaechenzahl sichtbar und nicht am Bild.
;
; Die beiden Testdateien entstehen mit scripts/make_rle_bmp_asset.py und zeigen
; dasselbe 8x4-Bild. Sie benutzen alle vier Kommandos des Formats; die schwarzen
; Felder stehen dort, wo Zeilenende und Sprung Bildpunkte ueberspringen - was
; uebersprungen wird, bleibt auf Palettenindex 0.
;
; Gedruckt wird die Farbe als Zahl, nicht ein Vergleich: schwarz 0, blau 255,
; gruen 65280, rot 16711680.

Graphics 320, 240, 32, 0

; ---- BI_RLE8 ----

Local img% = LoadImage("tests/assets/test_rle8.bmp")
If img = 0 Then Print "FEHLER rle8 nicht geladen" Else Print "rle8 geladen"
Print ImageWidth(img)
Print ImageHeight(img)

Local buf% = ImageBuffer(img)
LockBuffer buf

; oberste Zeile: ein Lauf ueber die ganze Breite, durchgehend blau
Print (ReadPixelFast(0, 0, buf) And $FFFFFF)
Print (ReadPixelFast(7, 0, buf) And $FFFFFF)

; zweite Zeile: Sprung um zwei, dann zwei gruene, dann Zeilenrest
Print (ReadPixelFast(0, 1, buf) And $FFFFFF)
Print (ReadPixelFast(2, 1, buf) And $FFFFFF)
Print (ReadPixelFast(4, 1, buf) And $FFFFFF)

; dritte Zeile: Rohdaten rot, gruen, blau
Print (ReadPixelFast(0, 2, buf) And $FFFFFF)
Print (ReadPixelFast(1, 2, buf) And $FFFFFF)
Print (ReadPixelFast(2, 2, buf) And $FFFFFF)

; unterste Zeile der Datei: vier rote, danach Zeilenende
Print (ReadPixelFast(3, 3, buf) And $FFFFFF)
Print (ReadPixelFast(4, 3, buf) And $FFFFFF)

UnlockBuffer buf
FreeImage img

; ---- BI_RLE4: dasselbe Bild, halb so viele Bit je Bildpunkt ----

Local img4% = LoadImage("tests/assets/test_rle4.bmp")
If img4 = 0 Then Print "FEHLER rle4 nicht geladen" Else Print "rle4 geladen"
Print ImageWidth(img4)
Print ImageHeight(img4)

Local buf4% = ImageBuffer(img4)
LockBuffer buf4
Print (ReadPixelFast(0, 0, buf4) And $FFFFFF)
Print (ReadPixelFast(0, 1, buf4) And $FFFFFF)
Print (ReadPixelFast(2, 1, buf4) And $FFFFFF)
Print (ReadPixelFast(1, 2, buf4) And $FFFFFF)
Print (ReadPixelFast(0, 3, buf4) And $FFFFFF)
UnlockBuffer buf4
FreeImage img4

; ---- unveraendert: eine unkomprimierte Datei geht weiter ueber stb_image ----

Local png% = LoadImage("tests/assets/test_grid.png")
If png = 0 Then Print "FEHLER png nicht geladen" Else Print "png geladen"

; ---- eine Datei, die es nicht gibt, bleibt 0 ----

Print LoadImage("tests/assets/gibtsnicht.bmp")

EndGraphics
Print "DONE"
