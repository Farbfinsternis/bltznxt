#!/usr/bin/env python3
"""Erzeugt die .b3d-Testdateien fuer tests/assets/ (3D-19).

In der Blitz3D-Installation gibt es keine einzige .b3d-Datei, und fremde
Modelle gehoeren nicht ins Repository. Diese hier sind klein und so gebaut,
dass sie die Stellen treffen, an denen ein .b3d-Leser danebengreifen kann:

    test_skin.b3d   ein Streifen aus zehn Vertices (x = -2..2, y = +-0.5)
                    unter dem Knoten "koerper" mit ANIM (20 Bilder). Zwei
                    Knochen: "knochen1" bei x=-2 traegt die linken Vertices
                    ganz und die mittleren halb und dreht sich bis Bild 20
                    um 90 Grad; "knochen2" (Kind von knochen1, lokal +4 in x)
                    traegt die rechten ganz und die mittleren halb und hebt
                    sich zu Bild 10 um 1. Zwei Brushes (rot, gruen) teilen
                    den Streifen in zwei Flaechen; ohne Normalen in der
                    Datei.

    test_nodes.b3d  Hierarchie ohne Knochen: "wurzel" mit ANIM (10 Bilder),
                    darunter "a" (Netz mit eigenem Brush und
                    Schluesseln fuer die Lage), "b" (ohne Netz, nur
                    Drehschluessel) und darunter "c" (Netz). Vertices mit
                    Normalen, Farben (eine ausserhalb 0..1) und zwei
                    Koordinatensaetzen zu je drei Werten. Eine Textur mit
                    Blend 3 und Koordinatensatz 1 (Flag 0x10000).

    test_nodes_seq.b3d  zweite Sequenz fuer test_nodes.b3d (LoadAnimSeq):
                    gleiche Namen, "a" mit anderen Schluesseln, 6 Bilder.

Aufruf aus dem Projektwurzelverzeichnis:

    python scripts/make_b3d_asset.py

Zum Format: jeder Chunk ist vier Zeichen Kennung und vier Byte Laenge (ohne
den Kopf), alles little-endian; Zeichenketten enden mit einer Null.
"""

import math
import os
import struct

LE = "<"


def chunk(tag, body):
    return tag.encode("ascii") + struct.pack(LE + "i", len(body)) + body


def cstr(s):
    return s.encode("latin-1") + b"\0"


def f(*v):
    return struct.pack(LE + "%df" % len(v), *v)


def i(*v):
    return struct.pack(LE + "%di" % len(v), *v)


def node(name, pos=(0, 0, 0), scl=(1, 1, 1), rot=(1, 0, 0, 0), subs=b""):
    return chunk("NODE", cstr(name) + f(*pos) + f(*scl) + f(*rot) + subs)


def keys(flags, entries):
    body = i(flags)
    for e in entries:
        body += i(e[0]) + b"".join(f(*x) for x in e[1:])
    return chunk("KEYS", body)


def anim(frames):
    return chunk("ANIM", i(0, frames) + f(30.0))


def rotz(deg):
    """Quaternion (w,x,y,z) wie in der Datei - um die z-Achse."""
    h = math.radians(deg) / 2
    return (math.cos(h), 0.0, 0.0, math.sin(h))


def roty(deg):
    h = math.radians(deg) / 2
    return (math.cos(h), 0.0, math.sin(h), 0.0)


def skin():
    # zehn Vertices: Paare (x,-0.5), (x,+0.5) fuer x = -2..2
    verts = []
    for x in range(-2, 3):
        verts += [(x, -0.5, 0.0), (x, 0.5, 0.0)]
    vrts = i(0, 1, 2) + b"".join(f(*p) + f((p[0] + 2) / 4, p[1] + 0.5) for p in verts)
    tris_l, tris_r = [], []
    for k in range(4):
        a, b, c, d = 2 * k, 2 * k + 1, 2 * k + 2, 2 * k + 3
        # von vorn (-z) gesehen im Uhrzeigersinn
        quad = [(a, b, d), (a, d, c)]
        (tris_l if k < 2 else tris_r).extend(quad)
    tris = chunk("TRIS", i(0) + b"".join(i(*t) for t in tris_l)) + \
        chunk("TRIS", i(1) + b"".join(i(*t) for t in tris_r))
    mesh = chunk("MESH", i(-1) + chunk("VRTS", vrts) + tris)

    # Knochen: Vertexnummern 0..9, x = -2,-2,-1,-1,0,0,1,1,2,2
    b1 = chunk("BONE", b"".join(i(v) + f(1.0) for v in (0, 1, 2, 3)) +
               b"".join(i(v) + f(0.5) for v in (4, 5)))
    b2 = chunk("BONE", b"".join(i(v) + f(0.5) for v in (4, 5)) +
               b"".join(i(v) + f(1.0) for v in (6, 7, 8, 9)))
    k1 = keys(4, [(0, rotz(0)), (20, rotz(90))])
    k2 = keys(1, [(0, (4, 0, 0)), (10, (4, 1, 0)), (20, (4, 0, 0))])
    n2 = node("knochen2", pos=(4, 0, 0), subs=b2 + k2)
    n1 = node("knochen1", pos=(-2, 0, 0), subs=b1 + k1 + n2)
    root = node("koerper", subs=mesh + anim(20) + n1)

    brus = chunk("BRUS", i(1) +
                 cstr("rot") + f(1, 0, 0, 1) + f(0.0) + i(1, 1) + i(-1) +
                 cstr("gruen") + f(0, 1, 0, 1) + f(0.0) + i(1, 1) + i(-1))
    return chunk("BB3D", i(1) + brus + root)


def tri_mesh(brush, color):
    # ein Dreieck mit Normalen, Farbe und zwei Koordinatensaetzen zu je drei
    # Werten; VRTS-Flags 3
    vs = [((-1, 0, 0), (0, 0, -1), color, (0, 0, 9), (0.5, 0.5, 9)),
          ((0, 1, 0), (0, 0, -1), color, (0.5, 1, 9), (1, 1, 9)),
          ((1, 0, 0), (0, 0, -1), color, (1, 0, 9), (1, 0.5, 9))]
    vrts = i(3, 2, 3) + b"".join(f(*p) + f(*n) + f(*c) + f(*t0) + f(*t1)
                                 for p, n, c, t0, t1 in vs)
    return chunk("MESH", i(brush) + chunk("VRTS", vrts) +
                 chunk("TRIS", i(0) + i(0, 1, 2)))


def nodes():
    texs = chunk("TEXS", cstr("quad.bmp") + i(1 | 0x10000, 3) + f(0, 0) + f(1, 1) + f(0))
    brus = chunk("BRUS", i(1) +
                 cstr("tex") + f(1, 1, 1, 1) + f(0.25) + i(1, 0) + i(0) +
                 cstr("blau") + f(0, 0, 1, 0.5) + f(0.0) + i(1, 16) + i(-1))
    a = node("a", pos=(2, 0, 0),
             subs=tri_mesh(1, (1.5, 0.5, -1, 1)) +
             keys(1, [(0, (2, 0, 0)), (10, (2, 5, 0))]))
    c = node("c", pos=(0, 0, 3), subs=tri_mesh(-1, (1, 1, 1, 1)))
    b = node("b", pos=(-2, 0, 0), subs=keys(4, [(0, roty(0)), (10, roty(90))]) + c)
    root = node("wurzel", subs=anim(10) + a + b)
    return chunk("BB3D", i(1) + texs + brus + root)


def nodes_seq():
    a = node("a", pos=(0, 0, 0), subs=keys(1, [(0, (0, 0, 0)), (6, (0, -3, 0))]))
    b = node("b")
    root = node("wurzel", subs=anim(6) + a + b)
    return chunk("BB3D", i(1) + root)


def main():
    out = os.path.join("tests", "assets")
    os.makedirs(out, exist_ok=True)
    for name, data in (("test_skin.b3d", skin()),
                       ("test_nodes.b3d", nodes()),
                       ("test_nodes_seq.b3d", nodes_seq())):
        path = os.path.join(out, name)
        with open(path, "wb") as fh:
            fh.write(data)
        print("%s: %d Bytes" % (path, len(data)))


if __name__ == "__main__":
    main()
