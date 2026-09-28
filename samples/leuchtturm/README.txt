LEUCHTTURM
==========

Ein kleiner Arena-Shooter im Stil von Quake III - geschrieben als ganz
gewöhnliches Blitz3D-Programm und übersetzt mit BLTZNXT, einem modernen
Nachbau von Blitz3D: https://github.com/Farbfinsternis/bltznxt

Eine Arena, drei Waffen (MG, Raketenwerfer, Railgun), Munition, Rüstung und
Gesundheit, Zielscheiben, 3D-Klang.


Starten
-------

leuchtturm.exe starten. Die exe ist nicht signiert; Windows meldet sich
deshalb beim ersten Start ("Der Computer wurde durch Windows geschützt"):
"Weitere Informationen" -> "Trotzdem ausführen".

Voraussetzungen: Windows 64 Bit, eine Grafikkarte mit OpenGL 3.3.


Steuerung
---------

Klick ins Fenster     Maus fangen (Tab gibt sie wieder frei)
Maus                  umsehen
W A S D / Pfeile      laufen
Leertaste             springen
linke Maustaste       feuern
1 2 3 / Mausrad       Waffe wählen
F1                    Anzeige ein/aus
Esc                   beenden

Rocket-Jump: nach unten schauen, springen und im selben Moment feuern.
Die große Gesundheit liegt oben auf der Säule.


Herkunft
--------

Modelle der Waffen, Zielscheiben und Items: Kenney (www.kenney.nl),
Creative Commons Zero (CC0) - Blaster Kit, Mini Dungeon, Platformer Kit.
Arena und Klänge: Platzhalter, vom Programm bzw. per Skript erzeugt.

Verwendete Bibliotheken:
  SDL3, SDL3_ttf     zlib-Lizenz, www.libsdl.org
  FreeType           Portions of this software are copyright
                     (c) The FreeType Project (www.freetype.org).
                     All rights reserved.
  HarfBuzz           MIT-Lizenz, github.com/harfbuzz/harfbuzz
  plutosvg           MIT-Lizenz, github.com/sammycage/plutosvg

Die Lizenztexte stehen in LIZENZEN.txt.
