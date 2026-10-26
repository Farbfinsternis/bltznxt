; BUG-195 - SetGamma, UpdateGamma, GammaRed/Green/Blue im Fenster.
; Das Original haelt je Kanal 256 Eintraege zu 16 Bit (Ziel*257, abgeschnitten,
; auf 0..65535 begrenzt, Index & 255); im Fenster beginnt die Tabelle mit 0..255
; und UpdateGamma tut nichts. Jedes Graphics setzt sie zurueck.
; Erwartung: Ausgabe von Blitz3D 11.8.
Graphics 200,150,0,2
Print GammaRed(0)+" "+GammaRed(100)+" "+GammaGreen(255)+" "+GammaBlue(128)
SetGamma 100,110,120, 50,60.5,70.9
Print GammaRed(100)+" "+GammaGreen(110)+" "+GammaBlue(120)
SetGamma 356,10,10, 1.5,300,-4
Print GammaRed(100)+" "+GammaGreen(10)+" "+GammaBlue(10)
UpdateGamma
UpdateGamma 1
Print GammaRed(100)+" "+GammaRed(-156)
Graphics 200,150,0,2
Print GammaRed(100)
Print "DONE"
