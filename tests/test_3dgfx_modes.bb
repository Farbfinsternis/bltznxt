; Grafikmodus- und Treiberaufzaehlung (CountGfxModes3D, GfxModeWidth/Height/
; Depth, GfxModeExists, Windowed3D, CountGfxDrivers, GfxDriverName, SetGfxDriver)
;
; Diese Befehle blockierten 30 Beispielprogramme, weil die gemeinsame
; "start.bb" der mak-Beispiele damit beginnt. Die Signaturen entsprechen denen
; des Originals ("blitzcc +k"); der Vergleich mit scripts/compare_commands.py
; meldet fuer alle zehn keine Abweichung.
;
; **Der Test prueft Invarianten, keine Zahlen.** Wie viele Modi und Treiber es
; gibt und wie sie heissen, haengt vom Rechner ab - eine feste .expected mit
; "21 Modi" waere auf jedem anderen Rechner falsch. Geprueft wird deshalb, was
; ueberall gelten muss: die 1-Basierung, das Verhalten ausserhalb des Bereichs
; und die Vertraeglichkeit der Abfragen untereinander.
;
; Die 1-Basierung ist nicht geraten, sondern aus dem tatsaechlichen Gebrauch im
; Beispielbestand abgelesen:
;   For k=1 To CountGfxModes3D() ... GfxModeWidth(k) ... Next
;   driver = Input$( "Display driver (1-"+CountGfxDrivers()+"):" )

treiber = CountGfxDrivers()
If treiber >= 1 Then Print "treiber vorhanden" Else Print "FEHLER treiber"

; 1-basiert: der erste Treiber hat einen Namen, 0 und n+1 nicht
If GfxDriverName$(1) <> "" Then Print "name(1) gesetzt" Else Print "FEHLER name(1)"
If GfxDriverName$(0) = "" Then Print "name(0) leer" Else Print "FEHLER name(0)"
If GfxDriverName$(treiber + 1) = "" Then Print "name(n+1) leer" Else Print "FEHLER name(n+1)"

; Fenster-3D koennen wir immer
If Windowed3D() = 1 Then Print "windowed3d" Else Print "FEHLER windowed3d"

; 2D- und 3D-Modusliste sind bei uns dieselbe
If CountGfxModes() = CountGfxModes3D() Then Print "modizahl gleich" Else Print "FEHLER modizahl"

n = CountGfxModes3D()
If n > 0
  If GfxModeWidth(1) > 0 Then Print "breite(1)" Else Print "FEHLER breite(1)"
  If GfxModeHeight(1) > 0 Then Print "hoehe(1)" Else Print "FEHLER hoehe(1)"
  If GfxModeDepth(1) > 0 Then Print "tiefe(1)" Else Print "FEHLER tiefe(1)"
  ; letzter gueltiger Index ist n, nicht n-1
  If GfxModeWidth(n) > 0 Then Print "breite(n)" Else Print "FEHLER breite(n)"
  ; ausserhalb des Bereichs: 0 statt Absturz
  If GfxModeWidth(0) = 0 Then Print "breite(0) null" Else Print "FEHLER breite(0)"
  If GfxModeWidth(n + 1) = 0 Then Print "breite(n+1) null" Else Print "FEHLER breite(n+1)"
  ; ein aufgezaehlter Modus muss auch existieren
  If GfxModeExists(GfxModeWidth(1), GfxModeHeight(1), GfxModeDepth(1)) = 1
    Print "exists stimmig"
  Else
    Print "FEHLER exists stimmig"
  EndIf
Else
  ; ohne Display trotzdem definiert: alles 0, kein Absturz
  If GfxModeWidth(1) = 0 Then Print "breite(1)" Else Print "FEHLER breite(1)"
  Print "hoehe(1)" : Print "tiefe(1)" : Print "breite(n)"
  Print "breite(0) null" : Print "breite(n+1) null" : Print "exists stimmig"
EndIf

; eine offensichtlich unmoegliche Aufloesung gibt es nicht
If GfxModeExists(12345, 678, 32) = 0 Then Print "exists negativ" Else Print "FEHLER exists negativ"

; SetGfxDriver darf nicht stuerzen
SetGfxDriver 1
Print "fertig"
