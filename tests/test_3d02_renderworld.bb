; 3D-02: RenderWorld ohne Kamera, TrisRendered = 0
; Ohne Kamera zeichnet RenderWorld nichts, auch keinen Hintergrund (BUG-137);
; CameraClsColor auf Handle 0 ist wirkungslos. Reiner Uebersetzungstest.
Graphics3D 800,600,32,1
CameraClsColor 0, 10, 10, 60
UpdateWorld
RenderWorld
Print TrisRendered()
Flip
WaitKey
