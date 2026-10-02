BLOCKWELT
=========

Eine Welt aus Blöcken im Stil von Minecraft - geschrieben als ganz
gewöhnliches Blitz3D-Programm und übersetzt mit BLTZNXT, einem modernen
Nachbau von Blitz3D: https://github.com/Farbfinsternis/bltznxt

Erste Fassung: Die Welt entsteht aus einer Saat in Chunks zu 16 x 128 x 16
Blöcken und wird um den Spieler herum nachgeladen - Hügel, Gebirge mit
Schnee, ein Ozean mit Stränden, Seen, kleine Tümpel, Höhlengänge und tiefe
Höhlenhallen, Kohle und Eisen im Stein, Wälder. Man läuft in der Ego-Sicht
hindurch, kann schwimmen und fliegen, Blöcke abbauen und setzen.
Gespeichert wird noch nichts.


Starten
-------

Im Ordner übersetzen und starten:

  blitzcc blockwelt.bb -o blockwelt
  blockwelt.exe              (Saat 2026)
  blockwelt.exe 1234         (eine andere Welt)
  blockwelt.exe 2026 -rundflug   (20 s Flug von selbst, misst die Bildzeiten)

Voraussetzungen: Windows 64 Bit, eine Grafikkarte mit OpenGL 3.3.


Steuerung
---------

Klick ins Fenster     Maus fangen (Tab gibt sie wieder frei)
Maus                  umsehen
W A S D / Pfeile      laufen
Strg                  rennen
Leertaste             springen, im Wasser schwimmen
F                     fliegen ein/aus (Leertaste hoch, Umschalt runter)
linke Maustaste       Block abbauen
rechte Maustaste      Block setzen
1 - 9 / Mausrad       Block wählen
F3                    Anzeige ein/aus
Esc                   beenden


Aufbau
------

blockwelt.bb    Hauptprogramm: Fenster, Eingabe, Hauptschleife
rauschen.bb     Perlin-Rauschen 2D/3D, fraktal
bloecke.bb      Blockarten und ihre selbst gemalten Texturen
welt.bb         Chunks, Zugriff, Erzeugung der Welt
netz.bb         Meshes aus den Blöcken: sichtbare Flächen, Schatten, Wasser
laden.bb        Nachladen und Vergessen von Chunks um den Spieler
spieler.bb      Bewegung und Kollision gegen das Blockraster, Blickstrahl

werkzeug/karte.bb   Draufsicht der Welt als Bild (zum Abstimmen des Geländes)
werkzeug/foto.bb    ein Bild von einer festen Stelle, misst Erzeugen und Bauen

Tests: tests/test_blockwelt_welt.bb (Erzeugung, Regeln für die Lage der
Blöcke) und tests/test_blockwelt_spieler.bb (Bewegung und Kollision).

PROTOKOLL.md hält fest, wo BLTZNXT beim Bau im Weg stand.


Herkunft
--------

Texturen und Welt erzeugt das Programm selbst; es gibt keine Dateien.

Verwendete Bibliotheken:
  SDL3, SDL3_ttf     zlib-Lizenz, www.libsdl.org
  FreeType           Portions of this software are copyright
                     (c) The FreeType Project (www.freetype.org).
                     All rights reserved.
