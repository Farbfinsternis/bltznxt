; BUG-208: Graphics-Modi wie im Original. Die Suite laeuft mit BLTZNXT_WINDOWED=1,
; deshalb hier nur die Fenstermodi; ein Modus, den das Original nicht kennt,
; bricht mit "Illegal Graphics mode" ab (gemessen 2026-10-27).
Graphics 320,240,0,2
Print "2 " + GraphicsWidth() + " " + GraphicsHeight()
Graphics 320,240,0,3
Print "3 " + GraphicsWidth() + " " + GraphicsHeight()
Graphics 320,240,0,6
Print "6 " + GraphicsWidth() + " " + GraphicsHeight()
Graphics 320,240,0,7
Print "7 " + GraphicsWidth() + " " + GraphicsHeight()
Graphics3D 200,150,0,3
Print "3d3 " + GraphicsWidth() + " " + GraphicsHeight()
Graphics3D 200,150,0,7
Print "3d7 " + GraphicsWidth() + " " + GraphicsHeight()
Graphics 100,100,0,4
Print "nicht erreicht"
End
