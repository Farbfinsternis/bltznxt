#!/usr/bin/env python3
"""Platzhalter-Modelle fuer das Leuchtturm-Spiel (Schritt 4: Waffen).

Bis Modelle aus Blender kommen (Schritt 5), stehen hier einfache Koerper,
gebaut nach denselben Regeln wie eine Datei aus Blender:

    daten/mg.glb     Maschinengewehr    Knoten "mg"   mit Kind "muendung"
    daten/rl.glb     Raketenwerfer      Knoten "rl"   mit Kind "muendung"
    daten/rail.glb   Railgun            Knoten "rail" mit Kind "muendung"
    daten/ziel.glb   Zielscheibe: Doppelpyramide, rot und weiss

Die Waffen sind so gebaut, wie man sie in der Hand sieht: Ursprung am
Griff, der Lauf zeigt nach Blitz +z. "muendung" ist ein leeres Objekt an
der Spitze des Laufs - von dort gehen Rauch, Rakete und Railspur aus; das
Spiel sucht es mit FindChild. Masse in Metern.

Aufruf aus dem Projektwurzelverzeichnis:

    python samples/leuchtturm/werkzeug/waffen.py
"""

import os

from glb import box, cylinder, mesh, octahedron, write

HERE = os.path.dirname(os.path.abspath(__file__))
DATEN = os.path.join(HERE, "..", "daten")

DUNKEL, GRAU, GELB = (0.15, 0.15, 0.17), (0.38, 0.38, 0.42), (0.80, 0.65, 0.20)
OLIV, ROT = (0.25, 0.35, 0.22), (0.75, 0.20, 0.15)
NACHT, CYAN, WEISS = (0.16, 0.18, 0.35), (0.30, 0.90, 1.00), (0.92, 0.92, 0.92)

WAFFEN = {
    "mg": ([
        (box(-0.03, -0.04, -0.10, 0.03, 0.03, 0.08), GRAU),               # Gehaeuse
        (cylinder(0.0, 0.012, 0.08, 0.24, 0.009, 8), DUNKEL),             # drei Laeufe
        (cylinder(-0.011, -0.007, 0.08, 0.24, 0.009, 8), DUNKEL),
        (cylinder(0.011, -0.007, 0.08, 0.24, 0.009, 8), DUNKEL),
        (box(-0.02, -0.10, -0.02, 0.02, -0.04, 0.03), GELB),              # Magazin
        (box(-0.015, -0.10, -0.09, 0.015, -0.04, -0.06), DUNKEL),         # Griff
    ], 0.25),
    "rl": ([
        (cylinder(0, 0, -0.14, 0.18, 0.04), OLIV),                        # Rohr
        (cylinder(0, 0, 0.18, 0.21, 0.047), ROT),                         # Muendungsring
        (box(-0.015, -0.12, -0.02, 0.015, -0.03, 0.03), DUNKEL),          # Griff
        (box(-0.01, 0.035, 0.0, 0.01, 0.07, 0.06), DUNKEL),               # Visier
    ], 0.21),
    "rail": ([
        (box(-0.03, -0.035, -0.14, 0.03, 0.035, 0.10), NACHT),            # Koerper
        (cylinder(0, 0, 0.10, 0.26, 0.014), GRAU),                        # Lauf
        (cylinder(0, 0, 0.13, 0.142, 0.028), CYAN),                       # Ringe
        (cylinder(0, 0, 0.17, 0.182, 0.028), CYAN),
        (cylinder(0, 0, 0.21, 0.222, 0.028), CYAN),
        (box(-0.015, -0.11, -0.10, 0.015, -0.035, -0.06), DUNKEL),        # Griff
    ], 0.27),
}


def main():
    gen = "BLTZNXT samples/leuchtturm/werkzeug/waffen.py"
    for name, (solids, spitze) in WAFFEN.items():
        nodes = [{"name": name, "mesh": 0, "children": [1]},
                 {"name": "muendung", "translation": [0, 0, -spitze]}]    # glTF: z gespiegelt
        out = os.path.join(DATEN, name + ".glb")
        write(out, nodes, [mesh(solids)], gen)
        print("%s: %d Dreiecke" % (os.path.normpath(out), len(mesh(solids)[3]) // 3))

    # Zielscheibe: je zwei Nachbarflaechen verschieden - ein Schachbrett
    solids = [(octahedron(0.45), [ROT, WEISS, WEISS, ROT])]
    out = os.path.join(DATEN, "ziel.glb")
    write(out, [{"name": "ziel", "mesh": 0}], [mesh(solids)], gen)
    print("%s: %d Dreiecke" % (os.path.normpath(out), len(mesh(solids)[3]) // 3))


if __name__ == "__main__":
    main()
