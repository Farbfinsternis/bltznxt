; BUG-159 - MoveMouse, MouseX/Y und MouseXSpeed/YSpeed wie im Original.
;
; Wie bbinput.cpp: die Geschwindigkeit ist der Abstand der Position zum letzten
; Aufruf bzw. zum letzten MoveMouse; MoveMouse gilt sofort. Nach Graphics steht
; die Maus auf 0,0, bis sie bewegt wird. Am Original gemessen (2026-09-18),
; im Fenster und im Vollbild gleich. Das skalierte Vollbild selbst prueft die
; Suite nicht; gemessen in build/mouse20260918/.

Graphics 640, 480, 0, 2
Print "start " + MouseX() + " " + MouseY() + " speed " + MouseXSpeed() + " " + MouseYSpeed()
MoveMouse 100, 50
Print "a " + MouseX() + " " + MouseY() + " speed " + MouseXSpeed() + " " + MouseYSpeed()
MoveMouse 300, 200
Delay 100
Print "b " + MouseX() + " " + MouseY() + " speed " + MouseXSpeed() + " " + MouseYSpeed()
MoveMouse 639, 479
Print "c " + MouseX() + " " + MouseY() + " speed " + MouseXSpeed() + " " + MouseYSpeed()
End
