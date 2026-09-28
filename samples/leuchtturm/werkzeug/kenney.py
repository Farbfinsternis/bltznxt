#!/usr/bin/env python3
"""Modelle fuer das Leuchtturm-Spiel aus Paketen von Kenney (www.kenney.nl).

Kenney stellt seine Pakete unter CC0 (gemeinfrei). Dieses Werkzeug laedt die
Pakete beim ersten Aufruf nach werkzeug/kenney/ (nicht im Repository), nimmt
die Modelle, bringt sie auf Spielgroesse und schreibt sie so, wie das Spiel
sie erwartet - mit eingebetteter Textur, damit jede .glb fuer sich steht:

    daten/mg.glb       Blaster Kit  blaster-p   "mg"   > "lauf", "muendung"
    daten/rl.glb       Blaster Kit  blaster-h   "rl"   > "muendung"
    daten/rail.glb     Blaster Kit  blaster-f   "rail" > "lauf", "muendung"
    daten/ziel.glb     Blaster Kit  target-large, zweimal Ruecken an Ruecken
    daten/items/ammo_*.glb     Blaster Kit  crate-small in den Farben der Waffen
    daten/items/armor_*.glb    Mini Dungeon shield-round: gruen, gelb, rot
    daten/items/health_*.glb   Platformer Kit heart: gelb, orange, gross und blau

Die Waffen folgen den Regeln aus LEUCHTTURM.md ("Waffen in Blender"): Ursprung
nahe am Griff, Lauf nach Blitz +z, "muendung" ein leeres Objekt an der Spitze
des Laufs. "lauf" ist der vordere Teil des Modells als eigener Knoten, damit
er sich beim Schuss bewegen kann. Zwei Animationen: feuern (Sequenz 0) und
heben (Sequenz 1), wie in waffen.py.

Umfaerben: Kenneys Textur ist eine Farbtafel aus Feldern zu 32 x 128 Pixeln
(16 Spalten, 4 Zeilen), jede Flaeche liegt in einem Feld. Eine andere Farbe
heisst: die Texturkoordinaten in ein anderes Feld verschieben.

Aufruf aus dem Projektwurzelverzeichnis:

    py samples/leuchtturm/werkzeug/kenney.py
"""

import io
import json
import math
import os
import struct
import urllib.request
import zipfile

from glb import pos, quat, write

HERE = os.path.dirname(os.path.abspath(__file__))
DATEN = os.path.join(HERE, "..", "daten")
CACHE = os.path.join(HERE, "kenney")
GEN = "BLTZNXT samples/leuchtturm/werkzeug/kenney.py - Modelle von Kenney (www.kenney.nl), CC0"

PAKETE = {
    "blaster-kit": "https://kenney.nl/media/pages/assets/blaster-kit/261d80a716-1753959510/kenney_blaster-kit_2.1.zip",
    "platformer-kit": "https://kenney.nl/media/pages/assets/platformer-kit/1585cf62b4-1775122253/kenney_platformer-kit.zip",
    "mini-dungeon": "https://kenney.nl/media/pages/assets/mini-dungeon/6cd72dc849-1785314274/kenney_mini-dungeon.zip",
}


def paket(name):
    """Ordner mit den .glb des Pakets; laedt es beim ersten Mal herunter."""
    ziel = os.path.join(CACHE, name)
    if not os.path.isdir(ziel):
        print("lade %s ..." % PAKETE[name])
        with urllib.request.urlopen(PAKETE[name]) as r:
            zipfile.ZipFile(io.BytesIO(r.read())).extractall(ziel)
    return os.path.join(ziel, "Models", "GLB format")


def qrot(q, v):
    """Vektor v mit dem Quaternion q = (x, y, z, w) drehen."""
    x, y, z, w = q
    tx, ty, tz = 2 * (y * v[2] - z * v[1]), 2 * (z * v[0] - x * v[2]), 2 * (x * v[1] - y * v[0])
    return (v[0] + w * tx + y * tz - z * ty, v[1] + w * ty + z * tx - x * tz, v[2] + w * tz + x * ty - y * tx)


class Modell:
    """Alle Dreiecke eines Kenney-Modells, flach in einem Netz, in
    Blitz-Koordinaten (z gespiegelt wie beim Lader)."""

    def __init__(self, pfad=None):
        self.p, self.n, self.uv, self.idx = [], [], [], []
        self.textur = None
        if pfad:
            self._lade(pfad)

    def _lade(self, pfad):
        d = open(pfad, "rb").read()
        jl = struct.unpack_from("<I", d, 12)[0]
        g = json.loads(d[20:20 + jl])
        b = d[20 + jl + 8:]
        self.textur = open(os.path.join(os.path.dirname(pfad), g["images"][0]["uri"]), "rb").read()

        def lies(ai):
            a = g["accessors"][ai]
            v = g["bufferViews"][a["bufferView"]]
            off = v.get("byteOffset", 0) + a.get("byteOffset", 0)
            if a["componentType"] == 5126:
                k = {"VEC2": 2, "VEC3": 3, "VEC4": 4}[a["type"]]
                st = v.get("byteStride", 4 * k)
                return [struct.unpack_from("<%df" % k, b, off + i * st) for i in range(a["count"])]
            f = {5121: "B", 5123: "H", 5125: "I"}[a["componentType"]]
            return list(struct.unpack_from("<%d%s" % (a["count"], f), b, off))

        def knoten(i, trs):
            nd = g["nodes"][i]
            t, r, s = nd.get("translation", (0, 0, 0)), nd.get("rotation", (0, 0, 0, 1)), nd.get("scale", (1, 1, 1))
            kette = [(t, r, s)] + trs                       # innen zuerst
            if "mesh" in nd:
                for pr in g["meshes"][nd["mesh"]]["primitives"]:
                    at = pr["attributes"]
                    base = len(self.p)
                    for v, n, uv in zip(lies(at["POSITION"]), lies(at["NORMAL"]), lies(at["TEXCOORD_0"])):
                        for t_, r_, s_ in kette:
                            v = qrot(r_, (v[0] * s_[0], v[1] * s_[1], v[2] * s_[2]))
                            v = (v[0] + t_[0], v[1] + t_[1], v[2] + t_[2])
                            n = qrot(r_, n)
                        self.p.append((v[0], v[1], -v[2]))
                        self.n.append((n[0], n[1], -n[2]))
                        self.uv.append(tuple(uv))
                    self.idx += [base + i for i in lies(pr["indices"])]
            for c in nd.get("children", ()):
                knoten(c, kette)

        for i in g["scenes"][g.get("scene", 0)]["nodes"]:
            knoten(i, [])

    def kopie(self):
        m = Modell()
        m.p, m.n, m.uv, m.idx, m.textur = list(self.p), list(self.n), list(self.uv), list(self.idx), self.textur
        return m

    def box(self):
        return (tuple(min(v[i] for v in self.p) for i in range(3)),
                tuple(max(v[i] for v in self.p) for i in range(3)))

    def skaliere(self, s):
        self.p = [(x * s, y * s, z * s) for x, y, z in self.p]
        return self

    def verschiebe(self, dx, dy, dz):
        self.p = [(x + dx, y + dy, z + dz) for x, y, z in self.p]
        return self

    def drehe_y(self, grad):
        """Um die y-Achse drehen, wie TurnEntity mit Gierwinkel."""
        c, s = math.cos(math.radians(grad)), math.sin(math.radians(grad))
        rot = lambda v: (v[0] * c - v[2] * s, v[1], v[0] * s + v[2] * c)
        self.p = [rot(v) for v in self.p]
        self.n = [rot(v) for v in self.n]
        return self

    def mittig(self, y=0.0):
        """Mitte der Huelle auf (0, y, 0)."""
        lo, hi = self.box()
        return self.verschiebe(-(lo[0] + hi[0]) / 2, y - (lo[1] + hi[1]) / 2, -(lo[2] + hi[2]) / 2)

    def faerbe(self, felder):
        """felder: {(spalte, zeile): (spalte, zeile)} der Farbtafel."""
        neu = []
        for u, v in self.uv:
            f = felder.get((int(u * 16), int(v * 4)))
            if f:
                u, v = u + (f[0] - int(u * 16)) / 16.0, v + (f[1] - int(v * 4)) / 4.0
            neu.append((u, v))
        self.uv = neu
        return self

    def dazu(self, m):
        base = len(self.p)
        self.p += m.p; self.n += m.n; self.uv += m.uv
        self.idx += [base + i for i in m.idx]
        return self

    def teile(self, z):
        """Die Dreiecke ganz vor der Ebene z werden ein eigenes Modell."""
        vorn, hinten = Modell(), Modell()
        vorn.textur = hinten.textur = self.textur
        for k in range(0, len(self.idx), 3):
            tri = self.idx[k:k + 3]
            teil = vorn if min(self.p[i][2] for i in tri) >= z else hinten
            teil.idx += [len(teil.p) + j for j in range(3)]
            for i in tri:
                teil.p.append(self.p[i]); teil.n.append(self.n[i]); teil.uv.append(self.uv[i])
        return vorn, hinten

    def netz(self):
        """Fuer glb.write, in glTF-Koordinaten."""
        return ([(x, y, -z) for x, y, z in self.p], [(x, y, -z) for x, y, z in self.n], None,
                self.idx, self.uv)


def speichere(out, nodes, modelle, anims=()):
    write(out, nodes, [m.netz() for m in modelle], GEN, anims, texture=modelle[0].textur)
    print("%s: %d Dreiecke" % (os.path.normpath(out), sum(len(m.idx) for m in modelle) // 3))


# --- Waffen -----------------------------------------------------------------

NULL, EINS = pos(0, 0, 0), (1, 1, 1)
RUHE = quat(1, 0, 0, 0)

# heben: 15 Bilder (0.25 s); unten und mit der Spitze nach unten gekippt
HEBEN = [(0, pos(0, -0.25, 0), quat(1, 0, 0, 40)), (15, NULL, RUHE)]


def waffe(name, datei, laenge, lauf_ab, feuern):
    """datei aus dem Blaster Kit, auf `laenge` Meter gebracht, hinten bei
    z = -0.10. lauf_ab: ab diesem z (im Kenney-Modell) ist es der Knoten
    "lauf", None ohne. feuern(lauf) liefert die Kanaele der Animation, lauf
    ist die Ruhelage des Laufs als pos()."""
    m = Modell(os.path.join(paket("blaster-kit"), datei + ".glb"))
    lo, hi = m.box()
    s = laenge / (hi[2] - lo[2])
    teile = [m] if lauf_ab is None else list(reversed(m.teile(lauf_ab)))
    for t in teile:
        t.verschiebe(-(lo[0] + hi[0]) / 2, -(lo[1] + hi[1]) / 2, -lo[2]).skaliere(s).verschiebe(0, -0.02, -0.10)

    # Muendung: vorderste Ecken, deren Mitte
    alle = [v for t in teile for v in t.p]
    zmax = max(v[2] for v in alle)
    spitze = [v for v in alle if v[2] > zmax - 0.004]
    muendung = (sum(v[0] for v in spitze) / len(spitze), sum(v[1] for v in spitze) / len(spitze), zmax)

    nodes = [{"name": name, "mesh": 0, "children": []}]
    lauf = NULL
    if len(teile) > 1:
        # Drehpunkt des Laufs: Mitte seines Querschnitts, an seinem hinteren Ende
        (llo, lhi) = teile[1].box()
        piv = ((llo[0] + lhi[0]) / 2, (llo[1] + lhi[1]) / 2, llo[2])
        teile[1].verschiebe(-piv[0], -piv[1], -piv[2])
        lauf = pos(*piv)
        nodes[0]["children"].append(1)
        nodes.append({"name": "lauf", "mesh": 1, "translation": list(lauf)})
    nodes[0]["children"].append(len(nodes))
    nodes.append({"name": "muendung", "translation": list(pos(*muendung))})

    index = {name: 0, "lauf": 1}
    anims = [("feuern", [(index[k], p, keys) for k, p, keys in feuern(lauf)]),
             ("heben", [(0, "translation", [(f, t) for f, t, r in HEBEN]),
                        (0, "rotation", [(f, r) for f, t, r in HEBEN])])]
    speichere(os.path.join(DATEN, name + ".glb"), nodes, teile, anims)


def zurueck(p, dz):
    """Die Lage p (aus pos()) um dz Meter nach hinten, Blitz -z = glTF +z."""
    return (p[0], p[1], p[2] + dz)


def waffen():
    # MG: 6 Bilder = ein Schuss; die Laeufe stossen zurueck wie ein Kolben
    waffe("mg", "blaster-p", 0.36, 0.376, lambda lauf: [
        ("mg", "translation", [(0, NULL), (1, pos(0, 0, -0.012)), (6, NULL)]),
        ("lauf", "translation", [(0, lauf), (2, zurueck(lauf, 0.02)), (6, lauf)]),
    ])
    # Raketenwerfer: Stoss nach hinten, die Spitze steigt, dann langsam zurueck
    waffe("rl", "blaster-h", 0.36, None, lambda lauf: [
        ("rl", "translation", [(0, NULL), (2, pos(0, 0.01, -0.06)), (30, NULL)]),
        ("rl", "rotation", [(0, RUHE), (2, quat(1, 0, 0, -10)), (30, RUHE)]),
    ])
    # Railgun: Stoss, der Lauf faehrt zurueck und laedt eine Sekunde lang nach -
    # er schiebt sich langsam wieder vor und dreht sich dabei einmal herum
    waffe("rail", "blaster-f", 0.50, 0.34, lambda lauf: [
        ("rail", "translation", [(0, NULL), (2, pos(0, 0, -0.07)), (40, NULL)]),
        ("lauf", "translation", [(0, lauf), (2, zurueck(lauf, 0.04)), (60, lauf)]),
        ("lauf", "rotation", [(0, RUHE), (20, quat(0, 0, 1, 120)), (40, quat(0, 0, 1, 240)),
                              (60, quat(0, 0, 1, 360))]),
    ])


# --- Zielscheibe und Items ---------------------------------------------------

def ziel():
    """Zwei Scheiben Ruecken an Ruecken, damit sie von beiden Seiten rot ist;
    so gross wie der Platzhalter (0.9 m)."""
    a = Modell(os.path.join(paket("blaster-kit"), "target-large.glb")).mittig()
    b = a.kopie().drehe_y(180)
    a.verschiebe(-0.002, 0, 0)             # die rote Seite schaut nach -x
    b.verschiebe(0.002, 0, 0)
    m = a.dazu(b).skaliere(0.9 / 0.34)
    speichere(os.path.join(DATEN, "ziel.glb"), [{"name": "ziel", "mesh": 0}], [m])


def item(name, m):
    speichere(os.path.join(DATEN, "items", name + ".glb"), [{"name": name, "mesh": 0}], [m])


def items():
    # Munition: der Koffer des Blaster Kit, gruen wie das MG, orange wie der
    # Raketenwerfer, lila wie die Railgun
    kiste = Modell(os.path.join(paket("blaster-kit"), "crate-small.glb")).mittig().skaliere(0.55)
    for name, feld in (("ammo_mg", (5, 2)), ("ammo_rl", (9, 2)), ("ammo_rail", (1, 1))):
        item(name, kiste.kopie().faerbe({(5, 2): feld}))

    # Ruestung: Rundschild, Flaeche und Ring gruen, gelb, rot
    schild = Modell(os.path.join(paket("mini-dungeon"), "shield-round.glb")).mittig()
    for menge, spalte, s in ((25, 8, 0.8), (50, 10, 1.0), (100, 14, 1.15)):
        farbe = {(3, 3): (spalte + 1, 2), (9, 3): (spalte, 2)}
        item("armor_%d" % menge, schild.kopie().faerbe(farbe).skaliere(1.3 * s))

    # Gesundheit: Herz, gelb, orange, gross und blau
    herz = Modell(os.path.join(paket("platformer-kit"), "heart.glb")).mittig()
    for menge, feld, s in ((25, (1, 1), 0.8), (50, (3, 1), 1.0), (100, (7, 1), 1.4)):
        item("health_%d" % menge, herz.kopie().faerbe({(5, 1): feld}).skaliere(1.2 * s))


if __name__ == "__main__":
    waffen()
    ziel()
    items()
