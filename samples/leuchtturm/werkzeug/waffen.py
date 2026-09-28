#!/usr/bin/env python3
"""Platzhalter-Modelle fuer das Leuchtturm-Spiel (Schritt 4 und 5: Waffen).

Bis Modelle aus Blender kommen, stehen hier einfache Koerper, gebaut nach
denselben Regeln wie eine Datei aus Blender (LEUCHTTURM.md, "Waffen in
Blender"):

    daten/mg.glb     Maschinengewehr    "mg"   > "laeufe", "muendung"
    daten/rl.glb     Raketenwerfer      "rl"   > "muendung"
    daten/rail.glb   Railgun            "rail" > "ringe", "muendung"
    daten/ziel.glb   Zielscheibe: Doppelpyramide, rot und weiss

Die Waffen sind so gebaut, wie man sie in der Hand sieht: Ursprung am
Griff, der Lauf zeigt nach Blitz +z. "muendung" ist ein leeres Objekt an
der Spitze des Laufs - von dort gehen Muendungsfeuer und Railspur aus; das
Spiel sucht es mit FindChild. Masse in Metern.

Jede Waffe hat zwei Animationen, in dieser Reihenfolge (Sequenz 0 und 1):

    feuern   ein Schuss: Rueckstoss, beim MG drehen sich die Laeufe um ein
             Drittel, bei der Railgun drehen und blaehen sich die Ringe
    heben    aus der Ruhe unten (Bild 0) in die Hand (letztes Bild); das
             Spiel spielt sie zum Senken rueckwaerts

Aufruf aus dem Projektwurzelverzeichnis:

    python samples/leuchtturm/werkzeug/waffen.py
"""

import os

from glb import box, cylinder, mesh, octahedron, pos, quat, write

HERE = os.path.dirname(os.path.abspath(__file__))
DATEN = os.path.join(HERE, "..", "daten")

DUNKEL, GRAU, GELB = (0.15, 0.15, 0.17), (0.38, 0.38, 0.42), (0.80, 0.65, 0.20)
OLIV, ROT = (0.25, 0.35, 0.22), (0.75, 0.20, 0.15)
NACHT, CYAN, WEISS = (0.16, 0.18, 0.35), (0.30, 0.90, 1.00), (0.92, 0.92, 0.92)

NULL, EINS = pos(0, 0, 0), (1, 1, 1)
RUHE = quat(1, 0, 0, 0)

# heben: 15 Bilder (0.25 s); unten und mit der Spitze nach unten gekippt
HEBEN = [(0, pos(0, -0.25, 0), quat(1, 0, 0, 40)), (15, NULL, RUHE)]


def heben(knoten):
    return ("heben", [(knoten, "translation", [(f, t) for f, t, r in HEBEN]),
                      (knoten, "rotation", [(f, r) for f, t, r in HEBEN])])


def waffe(name, teile, muendung, feuern):
    """teile: [(knotenname, solids)], das erste ist die Wurzel der Waffe, die
    anderen haengen an ihr. feuern: Kanaele der Animation, Knoten per Name."""
    nodes = [{"name": teile[0][0], "mesh": 0, "children": list(range(1, len(teile) + 1))}]
    for i, (n, _) in enumerate(teile[1:], 1):
        nodes.append({"name": n, "mesh": i})
    nodes.append({"name": "muendung", "translation": list(pos(0, 0, muendung))})
    index = {n: i for i, (n, _) in enumerate(teile)}
    anims = [("feuern", [(index[k], p, keys) for k, p, keys in feuern]), heben(0)]
    out = os.path.join(DATEN, name + ".glb")
    meshes = [mesh(s) for _, s in teile]
    write(out, nodes, meshes, "BLTZNXT samples/leuchtturm/werkzeug/waffen.py", anims)
    print("%s: %d Dreiecke" % (os.path.normpath(out), sum(len(m[3]) for m in meshes) // 3))


def main():
    waffe("mg", [
        ("mg", [
            (box(-0.03, -0.04, -0.10, 0.03, 0.03, 0.08), GRAU),           # Gehaeuse
            (box(-0.02, -0.10, -0.02, 0.02, -0.04, 0.03), GELB),          # Magazin
            (box(-0.015, -0.10, -0.09, 0.015, -0.04, -0.06), DUNKEL),     # Griff
        ]),
        ("laeufe", [                                                       # drei Laeufe um die z-Achse
            (cylinder(0.0, 0.013, 0.08, 0.24, 0.009, 8), DUNKEL),
            (cylinder(-0.011, -0.0065, 0.08, 0.24, 0.009, 8), DUNKEL),
            (cylinder(0.011, -0.0065, 0.08, 0.24, 0.009, 8), DUNKEL),
        ]),
    ], 0.25, [
        # 6 Bilder = ein Schuss; die Laeufe drehen sich um 120 Grad, so
        # sieht jeder Schuss wie der vorige aus und Dauerfeuer dreht durch
        ("mg", "translation", [(0, NULL), (1, pos(0, 0, -0.012)), (6, NULL)]),
        ("laeufe", "rotation", [(0, RUHE), (3, quat(0, 0, 1, 60)), (6, quat(0, 0, 1, 120))]),
    ])

    waffe("rl", [
        ("rl", [
            (cylinder(0, 0, -0.14, 0.18, 0.04), OLIV),                    # Rohr
            (cylinder(0, 0, 0.18, 0.21, 0.047), ROT),                     # Muendungsring
            (box(-0.015, -0.12, -0.02, 0.015, -0.03, 0.03), DUNKEL),      # Griff
            (box(-0.01, 0.035, 0.0, 0.01, 0.07, 0.06), DUNKEL),           # Visier
        ]),
    ], 0.21, [
        # Stoss nach hinten, die Spitze steigt, dann langsam zurueck
        ("rl", "translation", [(0, NULL), (2, pos(0, 0.01, -0.06)), (30, NULL)]),
        ("rl", "rotation", [(0, RUHE), (2, quat(1, 0, 0, -10)), (30, RUHE)]),
    ])

    waffe("rail", [
        ("rail", [
            (box(-0.03, -0.035, -0.14, 0.03, 0.035, 0.10), NACHT),        # Koerper
            (cylinder(0, 0, 0.10, 0.26, 0.014), GRAU),                    # Lauf
            (box(-0.015, -0.11, -0.10, 0.015, -0.035, -0.06), DUNKEL),    # Griff
        ]),
        ("ringe", [                                                        # Ringe um den Lauf
            (cylinder(0, 0, 0.13, 0.142, 0.028, 6), CYAN),
            (cylinder(0, 0, 0.17, 0.182, 0.028, 6), CYAN),
            (cylinder(0, 0, 0.21, 0.222, 0.028, 6), CYAN),
        ]),
    ], 0.27, [
        # Stoss, und die Ringe laden eine Sekunde lang nach: sie blaehen
        # sich auf und drehen sich einmal herum
        ("rail", "translation", [(0, NULL), (2, pos(0, 0, -0.07)), (40, NULL)]),
        ("ringe", "rotation", [(0, RUHE), (20, quat(0, 0, 1, 120)), (40, quat(0, 0, 1, 240)),
                               (60, quat(0, 0, 1, 360))]),
        ("ringe", "scale", [(0, EINS), (2, (1.5, 1.5, 1.0)), (60, EINS)]),
    ])

    # Zielscheibe: je zwei Nachbarflaechen verschieden - ein Schachbrett
    solids = [(octahedron(0.45), [ROT, WEISS, WEISS, ROT])]
    out = os.path.join(DATEN, "ziel.glb")
    write(out, [{"name": "ziel", "mesh": 0}], [mesh(solids)], "BLTZNXT samples/leuchtturm/werkzeug/waffen.py")
    print("%s: %d Dreiecke" % (os.path.normpath(out), len(mesh(solids)[3]) // 3))


if __name__ == "__main__":
    main()
