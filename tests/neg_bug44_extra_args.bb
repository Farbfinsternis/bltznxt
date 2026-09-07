; BUG-44 - Formen, die unsere Runtime als Erweiterung angeboten hat und die es
; in Blitz3D nicht gibt. Entscheidung des Nutzers (2026-09-07): der Compiler
; muss sich verhalten wie das Original, auch wo eine Erweiterung nicht mit der
; Syntax kollidiert, sondern bloss zusaetzlich ist.

Graphics 320, 240
Local img = CreateImage(16, 16, 4)

; "Rnd# ( from#[,to#] )" - ohne Argument gibt es Rnd nicht
Local r# = Rnd()

; die folgenden Befehle kennen im Original keinen frame-Parameter:
; "ImageWidth ( image )", "MidHandle image", "MaskImage image,red,green,blue",
; "ScaleImage image,xscale#,yscale#", "RotateImage image,angle#",
; "HandleImage image,x,y", "ImageXHandle ( image )"
Print ImageWidth(img, 1)
Print ImageHeight(img, 1)
Print ImageXHandle(img, 1)
Print ImageYHandle(img, 1)
MidHandle img, 1
HandleImage img, 2, 3, 1
MaskImage img, 255, 0, 255, 1
ScaleImage img, 2.0, 2.0, 1
RotateImage img, 90.0, 1
