; 3D-02: RenderWorld clears GL backbuffer, TrisRendered = 0
; Fenster zeigt dunkelblauen Hintergrund (GL clear), keine Entities noch.
Graphics3D 800,600,32,1
CameraClsColor 0, 10, 10, 60
UpdateWorld
RenderWorld
Print TrisRendered()
Flip
WaitKey
