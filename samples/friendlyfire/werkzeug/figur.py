#!/usr/bin/env python3
"""Die Spielerfigur fuer Friendly Fire: ein Roboter aus wenigen Teilen.

Die Figur hat keine Knochen und keine Animationen in der Datei. Sie besteht aus
Teilen an festen Gelenken; das Programm (figuren.bb) dreht sie - Laufen aus der
Geschwindigkeit, Zielen aus dem Blickwinkel, Sterben als Umfallen. Die Namen der
Teile sind die Schnittstelle: wer eine andere Figur baut (frei aus einem Paket
oder von Hand), muss sie nur wieder so benennen, dann laeuft sie im Spiel.

    figur        Wurzel, Fuesse bei y = 0, blickt nach +z
      bein_l, bein_r     Drehpunkt in der Huefte (Nicken = Schritt)
      torso              Drehpunkt in der Taille (Nicken = Vorbeugen)
        kopf             Drehpunkt am Hals
        arm_l, arm_r     Drehpunkt in der Schulter (Nicken = Arm heben)
        waffe            leerer Knoten: Anker fuer das Waffenmodell, vor der Brust
        name             leerer Knoten ueber dem Kopf: Anker fuer das Namensschild

Farbe: Jedes Teil, das ein Spieler einfaerben soll, ist ein Knoten mit weissem Netz
(das Spiel setzt EntityColor darauf). Was immer dunkel bleibt (Stiefel, Visier,
Guertel, Handschuhe, Rucksack), hat als Kind den Knoten "<teil>_dunkel" mit
Vertexfarben.

Masse wie die Spielerhuelle des Spiels: 1,8 m hoch (Ellipsoid 0,8 x 1,8).

Aufruf aus dem Projektwurzelverzeichnis:

    py samples/friendlyfire/werkzeug/figur.py
"""

import os

from glb import box, mesh, pos, write

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "..", "daten", "figur.glb")
GEN = "BLTZNXT samples/friendlyfire/werkzeug/figur.py"

DUNKEL, STAHL, VISIER, GLANZ = (0.16, 0.17, 0.20), (0.32, 0.34, 0.38), (0.05, 0.08, 0.10), (0.55, 0.85, 0.95)
WEISS = (1.0, 1.0, 1.0)


class Bau:
    """Sammelt Knoten und Netze; ein Teil ist ein Knoten mit weissem Netz und
    optional einem Kind "<name>_dunkel"."""

    def __init__(self):
        self.nodes, self.meshes = [], []

    def knoten(self, name, eltern=None, p=(0, 0, 0), farbe=None, dunkel=None):
        """farbe: Liste von box(...), weiss (einfaerbbar); dunkel: Liste von (box, Farbe).
        Liefert den Index des Knotens."""
        n = {"name": name}
        if p != (0, 0, 0):
            n["translation"] = list(pos(*p))
        if farbe:
            n["mesh"] = len(self.meshes)
            self.meshes.append(mesh([(f, WEISS) for f in farbe], False))
        self.nodes.append(n)
        i = len(self.nodes) - 1
        if eltern is not None:
            self.nodes[eltern].setdefault("children", []).append(i)
        if dunkel:
            d = {"name": name + "_dunkel", "mesh": len(self.meshes)}
            self.meshes.append(mesh([(b, c) for b, c in dunkel], True))
            self.nodes.append(d)
            self.nodes[i].setdefault("children", []).append(len(self.nodes) - 1)
        return i


def bauen():
    b = Bau()
    wurzel = b.knoten("figur")

    # Beine: Drehpunkt in der Huefte, 0,85 m ueber dem Boden
    for seite, x in (("l", -0.11), ("r", 0.11)):
        b.knoten("bein_" + seite, wurzel, (x, 0.85, 0),
                 farbe=[box(-0.06, -0.72, -0.065, 0.06, 0.0, 0.07)],
                 dunkel=[(box(-0.07, -0.85, -0.08, 0.07, -0.72, 0.17), STAHL),       # Stiefel
                         (box(-0.065, -0.45, -0.07, 0.065, -0.38, 0.075), DUNKEL)])  # Knie

    # Rumpf: Drehpunkt in der Taille
    torso = b.knoten("torso", wurzel, (0, 0.85, 0),
                     farbe=[box(-0.22, 0.05, -0.125, 0.22, 0.52, 0.125)],
                     dunkel=[(box(-0.23, 0.0, -0.135, 0.23, 0.07, 0.135), DUNKEL),          # Guertel
                             (box(-0.15, 0.14, -0.22, 0.15, 0.46, -0.125), STAHL),          # Rucksack
                             (box(-0.12, 0.30, 0.125, 0.12, 0.42, 0.14), VISIER)])          # Brustplatte

    # Kopf: Drehpunkt am Hals; Visier vorn
    b.knoten("kopf", torso, (0, 0.52, 0),
             farbe=[box(-0.115, 0.03, -0.115, 0.115, 0.27, 0.115)],
             dunkel=[(box(-0.09, 0.11, 0.115, 0.09, 0.21, 0.13), VISIER),
                     (box(-0.085, 0.14, 0.13, 0.085, 0.145, 0.135), GLANZ),
                     (box(-0.02, 0.27, -0.03, 0.02, 0.36, 0.01), STAHL)])                   # Antenne

    # Arme: Drehpunkt in der Schulter; haengen nach unten
    for seite, x in (("l", -0.30), ("r", 0.30)):
        b.knoten("arm_" + seite, torso, (x, 0.46, 0),
                 farbe=[box(-0.05, -0.50, -0.05, 0.05, 0.05, 0.05)],
                 dunkel=[(box(-0.055, -0.60, -0.055, 0.055, -0.50, 0.06), DUNKEL),           # Handschuh
                         (box(-0.06, 0.0, -0.06, 0.06, 0.06, 0.06), STAHL)])                 # Schultergelenk

    b.knoten("waffe", torso, (0.16, 0.36, 0.22))        # Anker fuer das Waffenmodell
    b.knoten("name", wurzel, (0, 2.05, 0))              # Anker fuer das Namensschild
    return b


def main():
    b = bauen()
    write(OUT, b.nodes, b.meshes, GEN)
    tris = sum(len(m[3]) for m in b.meshes) // 3
    print("%s: %d Dreiecke, %d Knoten" % (os.path.normpath(OUT), tris, len(b.nodes)))


if __name__ == "__main__":
    main()
