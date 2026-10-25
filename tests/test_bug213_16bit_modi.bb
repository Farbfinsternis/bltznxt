; BUG-213 - jede Aufloesung gibt es mit 32 und mit 16 Bit.
;
; Gemessen an Blitz3D 11.8 (build/bug213_20261025/modes): die Liste fuehrt
; erst alle Aufloesungen mit 32 Bit, dann dieselben in derselben Reihenfolge
; mit 16 Bit; vorn steht der Desktopmodus. GfxModeExists(640,480,16) ist 1,
; mit 24 oder 0 Bit gibt es nichts. (Windows meldet dem Original die 16-Bit-
; Modi nur beim ersten Start einer neuen Exe; auf Systemen von damals immer.)
; Bei uns kannte die Liste nur 32 Bit, und BBLauncher/SkyPlateau brachen mit
; "Please select another..." ab, weil sie vor dem Start 640x480x16 pruefen.
; Geprueft wird nur, was auf jedem Rechner gilt - die Aufloesungen selbst
; haengen am Bildschirm.
n = CountGfxModes()
If n > 0 And n Mod 2 = 0 Then Print "zahl gerade" Else Print "FEHLER zahl " + n
If CountGfxModes3D() = n Then Print "3d gleich" Else Print "FEHLER 3d"
half = n / 2
ok32 = 1 : ok16 = 1 : gleich = 1
For k = 1 To half
	If GfxModeDepth(k) <> 32 Then ok32 = 0
	If GfxModeDepth(k + half) <> 16 Then ok16 = 0
	If GfxModeWidth(k) <> GfxModeWidth(k + half) Or GfxModeHeight(k) <> GfxModeHeight(k + half) Then gleich = 0
Next
If ok32 Then Print "erste haelfte 32" Else Print "FEHLER 32"
If ok16 Then Print "zweite haelfte 16" Else Print "FEHLER 16"
If gleich Then Print "gleiche aufloesungen" Else Print "FEHLER aufloesungen"
w = GfxModeWidth(1) : h = GfxModeHeight(1)
Print GfxModeExists(w, h, 32)
Print GfxModeExists(w, h, 16)
Print GfxMode3DExists(w, h, 16)
Print GfxModeExists(w, h, 24)
Print GfxModeExists(w, h, 0)
Print GfxModeExists(640, 480, 16) = GfxModeExists(640, 480, 32)
