#!/usr/bin/env python3
"""Platzhalter-Arena fuer das Leuchtturm-Spiel (Schritt 3: Bewegung und Kollision).

Bis es eine Karte aus Blender gibt, steht hier eine kleine Arena, gebaut nach
denselben Regeln (LEUCHTTURM.md, "Die Karte in Blender"): eine .glb, alles
Wissen steckt in den Objektnamen.

    arena       sichtbare Geometrie, kollidiert nicht
    arena-col   dieselben Koerper als Kollisionsgeometrie (unsichtbar)
    spawn       leeres Objekt: Startpunkt, Blick nach Blitz +z
    ziel        leere Objekte: dort schweben Zielscheiben (Schritt 4)
    weapon_... ammo_... armor_... health_...
                leere Objekte: Items (Schritt 7), Name nach LEUCHTTURM.md

Masse in Metern, 1 Einheit = 1 m. Angaben unten in Blitz-Koordinaten (y oben,
z nach vorn); geschrieben wird glTF (z gespiegelt), der Lader spiegelt zurueck.

    Boden        40 x 40, Oberkante y = 0
    Waende       ringsum, 6 hoch
    Plattform A  x 8..14, z 8..14, Oberkante 2 - erreichbar ueber die Rampe
    Rampe        x 2..8 (y 0 -> 2), z 9..13 - 18 Grad
    Plattform B  x -14..-8, z 8..14, Oberkante 1.5 - ueber die Treppe
    Treppe       6 Stufen zu 0.25 m, je 0.5 m tief, x -5 .. -8
    Block        x -10..-7, z -10..-7, 1 m hoch - zum Draufspringen
    Saeule       x -1..1, z -1..1 um (0, -8) verschoben, 6 hoch
    spawn        (0, 0, -15)
    ziel         Mitte, ueber Plattform A und B, ueber dem Block, rechts,
                 und hoch vor der Nordwand
    Items        Raketenwerfer auf Plattform A, Railgun auf Plattform B,
                 Munition in drei Ecken, Ruestung auf dem Block, rechts
                 hinten und vor der Nordwand, Gesundheit links und rechts -
                 und die grosse Gesundheit oben auf der Saeule, nur mit
                 einem Rocket-Jump zu erreichen

Aufruf aus dem Projektwurzelverzeichnis:

    python samples/leuchtturm/werkzeug/arena.py
"""

import os

from glb import box, mesh, wedge, write

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "..", "daten", "arena.glb")


# Koerper: (Flaechen, Farbe)
GRAU, BRAUN, BLAU, GRUEN, ORANGE, ROT, GELB = (
    (0.55, 0.55, 0.55), (0.45, 0.33, 0.22), (0.25, 0.35, 0.70), (0.30, 0.60, 0.30),
    (0.85, 0.50, 0.15), (0.70, 0.20, 0.20), (0.80, 0.75, 0.25))
SOLIDS = [
    (box(-20, -1, -20, 20, 0, 20), GRAU),                          # Boden
    (box(-21, 0, 20, 21, 6, 21), BRAUN), (box(-21, 0, -21, 21, 6, -20), BRAUN),
    (box(20, 0, -20, 21, 6, 20), BRAUN), (box(-21, 0, -20, -20, 6, 20), BRAUN),
    (box(8, 0, 8, 14, 2, 14), BLAU),                               # Plattform A
    (wedge(2, 8, 9, 13, 2), GRUEN),                                # Rampe
    (box(-14, 0, 8, -8, 1.5, 14), BLAU),                           # Plattform B
    (box(-10, 0, -10, -7, 1, -7), GELB),                           # Block
    (box(-1, 0, -9, 1, 6, -7), ROT),                               # Saeule
]
for i in range(6):                                                  # Treppe
    SOLIDS.append((box(-5 - 0.5 * (i + 1), 0, 9, -5 - 0.5 * i, 0.25 * (i + 1), 13), ORANGE))

# Zielscheiben (Blitz-Koordinaten, Mitte der Scheibe)
ZIELE = [(0, 1.4, 0), (11, 3.4, 11), (-11, 2.9, 11), (-8.5, 2.4, -8.5), (12, 1.4, -10), (0, 4.5, 17)]

# Items (Blitz-Koordinaten, Mitte des Items, etwa einen halben Meter ueber dem Boden)
ITEMS = [
    ("weapon_rl", (11, 2.5, 10)), ("weapon_rail", (-11, 2.0, 10)),
    ("ammo_mg", (-17, 0.5, -17)), ("ammo_rl", (17, 0.5, -5)), ("ammo_rail", (-17, 0.5, 2)),
    ("armor_25", (-8.5, 1.5, -8.5)), ("armor_50", (17, 0.5, 17)), ("armor_100", (0, 0.5, 18)),
    ("health_25", (-6, 0.5, -17)), ("health_50", (17, 0.5, 4)), ("health_100", (0, 6.5, -8)),
]


def main():
    nodes = [
        {"name": "arena", "mesh": 0},
        {"name": "arena-col", "mesh": 1},
        {"name": "spawn", "translation": [0, 0, 15]},
    ]
    for i, (x, y, z) in enumerate(ZIELE):
        name = "ziel" if i == 0 else "ziel.%03d" % i               # wie Blenders Kopien
        nodes.append({"name": name, "translation": [x, y, -z]})
    for name, (x, y, z) in ITEMS:
        nodes.append({"name": name, "translation": [x, y, -z]})
    write(OUT, nodes, [mesh(SOLIDS, True), mesh(SOLIDS, False)],
          "BLTZNXT samples/leuchtturm/werkzeug/arena.py")
    print("%s: %d Dreiecke" % (os.path.normpath(OUT), len(mesh(SOLIDS, False)[3]) // 3))


if __name__ == "__main__":
    main()
