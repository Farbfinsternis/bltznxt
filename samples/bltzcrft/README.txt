BLTZCRFT
========

Eine Welt aus Blöcken im Stil von Minecraft - geschrieben als ganz
gewöhnliches Blitz3D-Programm und übersetzt mit BLTZNXT, einem modernen
Nachbau von Blitz3D: https://github.com/Farbfinsternis/bltznxt

Erste Fassung: Die Welt entsteht aus einer Saat in Chunks zu 16 x 256 x 16
Blöcken und wird um den Spieler herum nachgeladen - Hügel, Gebirge mit
Schnee, ein Ozean mit Stränden, Seen, kleine Tümpel, Kohle und Eisen im
Stein, Wälder, Wiesen mit hohem Gras und
Blumen (Löwenzahn, Mohn, Kornblume), Schilf am Ufer.

Unter dem Meeresspiegel (Höhe 112) liegen über 100 Blöcke Gestein voller
Höhlen, wie in Minecraft aus 3D-Rauschen: winzige Kriechgänge, Gänge von
eng bis breit, die an der Oberfläche als Eingänge enden, Käsehöhlen und in
manchen Gegenden riesige Gewölbe, Hunderte Blöcke weit und Dutzende hoch,
die sich über Kilometer ziehen. Höhlen sind dunkel wie in Minecraft ohne
Fackel - schon gut zehn Blöcke unter dem Gestein ahnt man nur noch Umrisse,
hell ist es nur, wo durch eine Öffnung Licht von oben fällt. Auch die Hand,
Bruchstücke und Mitspieler werden dort dunkel.

Dagegen helfen Fackeln. Man stellt sie aus einem Ast und einem Stück Kohle
her - Kohle bringt erst die Spitzhacke aus dem Kohleerz, man beginnt also
ohne Licht. Mit der rechten Maustaste steht eine Fackel aufrecht
auf dem Boden oder schräg an einer Wand, nie an der Decke. Ihr Licht breitet
sich wie in Minecraft aus - um Ecken herum, nicht durch Gestein, nach etwa
zehn Blöcken ist es verglüht - und scheint nachts so hell wie am Tag. Baut
man den Block ab, an dem sie hängt, fällt sie herunter ins Inventar. Auch
in der Hand leuchtet eine Fackel und wandert mit - im Dunkeln, am hellen Tag
im Freien spart sie sich das. Man läuft in der Ego-Sicht
hindurch, kann schwimmen und fliegen. Abgebaute Blöcke landen im Inventar
(36 Plätze, Stapel bis 64) und lassen sich wieder setzen.

Abbauen geht wie in Minecraft mit bloßen Händen: Die Hand schlägt auf den
Block ein, Bruchstücke springen ab, Risse wachsen, bis er bricht. Erde und
Sand dauern 0,75 s, Holz 3 s. Stein, Kohleerz, Eisenerz und Sandstein brechen
mit der Hand nur langsam (Stein 7,5 s) und hinterlassen nichts - dafür braucht
es eine Spitzhacke. Steinwerkzeuge (Spitzhacke, Axt, Schaufel) bauen ihre
Blöcke viermal so schnell ab und nutzen sich dabei ab, je Block um einen
Punkt von 100; kaputt wirken sie wie die bloße Hand. Mit der Steinspitzhacke
geben Stein und Sandstein ihren Block und Kohleerz Kohle, Eisenerz verlangt
eine bessere Spitzhacke. Laub gibt ab und zu einen Ast, Kies einen
Steinbrocken, und lose Steine an Stränden und auf Wiesen geben Steinbrocken -
daraus entstehen die ersten Werkzeuge. Pflanzen brechen sofort;
Blumen und Schilf kommen ins Inventar und lassen sich wieder pflanzen
(Blumen auf Gras oder Erde, Schilf auch auf Sand), hohes Gras gibt nichts
und macht einem gesetzten Block Platz. Baut man den Boden unter einer
Pflanze ab, bricht sie mit.

Herstellen geht ohne Rezeptgitter, wie in Survival-Spielen (CRAFTING.md): Im
Inventar (E) steht rechts eine Liste der bekannten Rezepte. Ein Rezept wird
bekannt, sobald man jede seiner Zutaten einmal aufgehoben hat - Ast und
Steinbrocken zusammen ergeben die Steinwerkzeuge, ein Stamm Äste und Bretter,
Ast und Kohle Fackeln. Ein Klick zieht die Zutaten ab, nach ein paar Sekunden
liegt das Ergebnis im Inventar; bis zu acht Aufträge warten in einer Reihe
und laufen auch bei geschlossenem Inventar weiter (Balken neben der
Schnellleiste).

Aus vier Brettern und zwei Steinbrocken entsteht eine Werkbank. Steht man
höchstens vier Blöcke von ihr entfernt, kommen ihre Rezepte dazu (zuerst
Steinziegel aus Stein), und abgenutzte oder kaputte Werkzeuge lassen sich
dort reparieren: gegen die Hälfte ihrer Zutaten, mindestens eine je Art.
Rechtsklick auf die Werkbank öffnet das Inventar, mit Umschalt setzt man wie
sonst einen Block daran. Werkbank-Aufträge halten an, wenn man weggeht.

Gespeichert wird noch nichts.

Ein Tag dauert 20 Minuten: die Sonne zieht über den Himmel, abends färbt er
sich rot und die Welt wird dunkel, nachts stehen Mond (mit Phasen) und
Sterne am Himmel, und eine Wolkendecke zieht langsam nach Osten.

Mehrspieler im LAN: Mitspieler laufen als Klötzchenfigur mit Namen durch
dieselbe Welt, abgebaute und gesetzte Blöcke sehen alle, dazu ein Chat.
Sonst wirkt niemand auf den anderen.


Starten
-------

Im Ordner übersetzen und starten:

  blitzcc bltzcrft.bb -o bltzcrft
  bltzcrft.exe              (Saat 2026)
  bltzcrft.exe 1234         (eine andere Welt)
  bltzcrft.exe 2026 -rundflug   (20 s Flug von selbst, misst die Bildzeiten)
  bltzcrft.exe -zeit 18.5   (beginnt um 18:30 statt um 7 Uhr)
  bltzcrft.exe -zeit 18 -foto 90 10   (schaut nach Westen, speichert nach
                            8 s foto_<uhrzeit>.bmp und beendet sich)
  bltzcrft.exe -foto 0 50 -abbau   (baut dazu ab 4 s selbst ab)
  bltzcrft.exe -pos -2900 60 1500  (beginnt an dieser Stelle, hier in einem
                            großen Gewölbe; mit -foto ... -fackeln stellt es
                            ringsum Fackeln auf)
  bltzcrft.exe -werkzeuge -wahl 5 -foto 0 10   (mit Ästen, Brocken und
                            Steinwerkzeugen; -inventar öffnet das Inventar,
                            -herstellen stellt dazu drei Steinäxte her)

Voraussetzungen: Windows 64 Bit, eine Grafikkarte mit OpenGL 3.3.


Mehrspieler
-----------

Im Titelbild M drücken. BLTZCRFT sucht etwa eine Sekunde im LAN nach einem
laufenden Spiel und tritt bei - man bekommt die Welt des Hosts samt allen
Änderungen und steht vier Blöcke neben ihm. Findet es keins, hostet es
selbst, und die anderen treten dort bei. Geht der Host, übernimmt ein
anderer; das Spiel läuft weiter.

  bltzcrft.exe -mehrspieler             (gleich so, ohne Titelbild)
  bltzcrft.exe -mehrspieler -name Anna  (eigener Name, sonst "Spieler" + Zahl)
  bltzcrft.exe -join 192.168.0.5        (genau dort beitreten)
  bltzcrft.exe -test 14                 (Selbsttest ohne Maus und Tastatur,
                                         siehe bltzcrft.bb)

Beim ersten Hosten fragt die Windows-Firewall, ob BLTZCRFT Verbindungen
annehmen darf. Über das Internet geht es nur, wenn der Router den
UDP-Port 2350 weiterleitet; BLTZNXT bittet ihn darum (UPnP).


Steuerung
---------

Klick ins Fenster     Maus fangen (Tab gibt sie wieder frei)
Maus                  umsehen
W A S D / Pfeile      laufen
Strg                  rennen
Leertaste             springen, im Wasser schwimmen
F                     fliegen ein/aus (Leertaste hoch, Umschalt runter)
linke Maustaste       halten: Block abbauen (Risse zeigen den Fortschritt,
                      Erde 0,75 s, Holz 3 s, Stein 7,5 s ohne Ertrag,
                      Grundgestein bricht nie)
rechte Maustaste      Block aus der Schnellleiste setzen, Fackeln an Boden
                      oder Wand; auf eine Werkbank: ihre Rezepte (Umschalt: setzen)
1 - 9 / Mausrad       Platz der Schnellleiste wählen
E                     Inventar öffnen/schließen; darin: Linksklick Stapel
                      nehmen/ablegen/tauschen, Rechtsklick halber Stapel
                      bzw. ein Stück, Umschalt+Klick zwischen Leiste und Vorrat
                      rechts die Rezepte: Klick stellt her, Umschalt+Klick so
                      oft wie möglich, Rechtsklick auf einen Auftrag bricht ab
F1                    Hilfe: alle Tasten im Spiel
F3                    Anzeige ein/aus (mit Uhrzeit)
F4                    Zeitraffer: ein Tag in 20 Sekunden (nur allein)
T                     Chat im Mehrspieler (Enter schickt, Esc verwirft)
Esc                  beenden (bei offenem Inventar oder Hilfe: schließen)


Aufbau
------

bltzcrft.bb     Hauptprogramm: Fenster, Eingabe, Hauptschleife
rauschen.bb     Perlin-Rauschen 2D/3D, fraktal
bloecke.bb      Blockarten (auch Pflanzen) und ihre selbst gemalten Texturen
welt.bb         Chunks, Zugriff, Erzeugung der Welt, Liste der Änderungen
netz.bb         Meshes aus den Blöcken: sichtbare Flächen, Schatten, Wasser,
                Pflanzen als gekreuzte Flächen
laden.bb        Nachladen und Vergessen von Chunks um den Spieler
spieler.bb      Bewegung und Kollision gegen das Blockraster, Blickstrahl
inventar.bb     Inventar: Stapel, Klicks, Haltbarkeit, zur Laufzeit gemalte Symbole
dinge.bb        Gegenstände (Ast, Steinbrocken, Kohle, Werkzeuge) und ihre Wirkung
rezepte.bb      Rezepte, Freischalten beim Aufheben, Herstellen über die Zeit
abbau.bb        Abbauen über die Zeit, zur Laufzeit gemalte Risse
mehrspieler.bb  Mehrspieler: Beitritt, Figuren, Blöcke, Chat (Net-Befehle)
himmel.bb       Tag und Nacht: Himmelskuppel, Sonne, Mond, Sterne, Wolken
hand.bb         die eigene Hand (oder der gewählte Block) und ihr Schlag
partikel.bb     Bruchstücke beim Abbauen

daten/logo.png  das Logo für Titel- und Ladebild

werkzeug/karte.bb   Draufsicht der Welt als Bild (zum Abstimmen des Geländes)
werkzeug/foto.bb    ein Bild von einer festen Stelle, misst Erzeugen und Bauen
werkzeug/schnitt.bb Schnitte durch die Welt als Bild, ohne Fenster (zum
                    Abstimmen der Höhlen): schnitt.exe saat x z
werkzeug/video.bb   Kameraflug für das Werbevideo, Bild für Bild als BMP
werkzeug/video_schnitt.py  setzt die Bilder in Blender zum MP4 zusammen,
                    mit Logo-Einblendung

Tests: tests/test_bltzcrft_welt.bb (Erzeugung, Regeln für die Lage der
Blöcke) und tests/test_bltzcrft_spieler.bb (Bewegung und Kollision).

PROTOKOLL.md hält fest, wo BLTZNXT beim Bau im Weg stand.


Herkunft
--------

Texturen und Welt erzeugt das Programm selbst. Einzige Datei ist das Logo
(daten/logo.png), es erscheint im Titel- und Ladebild.

Verwendete Bibliotheken:
  SDL3, SDL3_ttf     zlib-Lizenz, www.libsdl.org
  FreeType           Portions of this software are copyright
                     (c) The FreeType Project (www.freetype.org).
                     All rights reserved.
