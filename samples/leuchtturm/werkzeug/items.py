#!/usr/bin/env python3
"""Platzhalter-Modelle fuer die Items des Leuchtturm-Spiels (Schritt 7).

Die Waffen liegen als ihre eigenen Modelle (daten/mg.glb ...) in der Arena;
hier entsteht der Rest, jeweils um den Ursprung, nach daten/items/:

    ammo_mg.glb, ammo_rl.glb, ammo_rail.glb
        Munitionskisten in den Farben der Waffen
    armor_25.glb, armor_50.glb, armor_100.glb
        Weste mit Schulterstuecken: gruen, gelb, rot
    health_25.glb, health_50.glb, health_100.glb
        Kreuz aus drei Balken: gelb, orange, und gross und blau

Ein Item ohne passende Datei (armor_75 aus einer Karte) nimmt das Spiel mit
dem naechstkleineren Wert.

Aufruf aus dem Projektwurzelverzeichnis:

    python samples/leuchtturm/werkzeug/items.py
"""

import os

from glb import box, mesh, write

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "..", "daten", "items")

DUNKEL, WEISS = (0.15, 0.15, 0.17), (0.92, 0.92, 0.92)
GELB, ROT, OLIV = (0.85, 0.70, 0.20), (0.80, 0.20, 0.15), (0.25, 0.40, 0.22)
NACHT, CYAN = (0.16, 0.18, 0.40), (0.30, 0.90, 1.00)
GRUEN, ORANGE, BLAU = (0.25, 0.75, 0.30), (0.95, 0.55, 0.15), (0.25, 0.45, 0.95)


def kiste(farbe, deckel):
    return [(box(-0.2, -0.13, -0.13, 0.2, 0.1, 0.13), farbe),
            (box(-0.21, 0.1, -0.14, 0.21, 0.15, 0.14), deckel)]


def weste(farbe, s=1.0):
    return [(box(-0.22 * s, -0.25 * s, -0.08 * s, 0.22 * s, 0.2 * s, 0.08 * s), farbe),
            (box(-0.3 * s, 0.12 * s, -0.1 * s, -0.1 * s, 0.26 * s, 0.1 * s), farbe),
            (box(0.1 * s, 0.12 * s, -0.1 * s, 0.3 * s, 0.26 * s, 0.1 * s), farbe),
            (box(-0.05 * s, -0.25 * s, -0.09 * s, 0.05 * s, 0.2 * s, 0.09 * s), DUNKEL)]


def kreuz(farbe, s):
    a, b = 0.25 * s, 0.08 * s
    return [(box(-a, -b, -b, a, b, b), farbe), (box(-b, -a, -b, b, a, b), farbe),
            (box(-b, -b, -a, b, b, a), WEISS)]


ITEMS = {
    "ammo_mg": kiste(GELB, DUNKEL),
    "ammo_rl": kiste(OLIV, ROT),
    "ammo_rail": kiste(NACHT, CYAN),
    "armor_25": weste(GRUEN, 0.8),
    "armor_50": weste(GELB),
    "armor_100": weste(ROT, 1.15),
    "health_25": kreuz(GELB, 0.8),
    "health_50": kreuz(ORANGE, 1.0),
    "health_100": kreuz(BLAU, 1.4),
}


def main():
    for name, solids in ITEMS.items():
        out = os.path.join(OUT, name + ".glb")
        write(out, [{"name": name, "mesh": 0}], [mesh(solids)],
              "BLTZNXT samples/leuchtturm/werkzeug/items.py")
        print("%s: %d Dreiecke" % (os.path.normpath(out), len(mesh(solids)[3]) // 3))


if __name__ == "__main__":
    main()
