#!/usr/bin/env python3
"""Erzeugt die .3ds-Testdateien fuer tests/assets/.

Die Modelldateien der Blitz3D-Installation sind fremdes Material und gehoeren
nicht ins Repository; ausserdem muesste man ihre erwarteten Zahlen erst
ablesen. Diese hier ist eigens gebaut, damit die Erwartung feststeht:

    test_box.3ds     zwei Quader, zwei Materialien, keine Textur.
                     In 3DS-Koordinaten misst das Ganze x=12, y=4, z=6.
                     Weil der Loader y und z tauscht (3D Studio ist
                     rechtshaendig mit z nach oben), muss Blitz3D daraus
                     w=12, h=6, d=4 machen - und zwei Flaechen, eine je
                     Material. Genau das meldet auch das Original fuer
                     diese Datei.

    test_broken.3ds  richtige Endung, unbrauchbarer Inhalt: der Hauptchunk
                     gibt eine Laenge weit hinter dem Dateiende an. Der
                     Parser muss in seinen Grenzpruefungen haengenbleiben
                     und 0 liefern, statt hinter den Puffer zu lesen.

    test_tri_cw.3ds  ein Dreieck, von vorn im Uhrzeigersinn (nach dem
    test_tri_ccw.3ds y/z-Tausch des Loaders) bzw. andersherum - BUG-126.

Aufruf aus dem Projektwurzelverzeichnis:

    python scripts/make_3ds_asset.py

Zum Format: eine .3ds-Datei ist ein Baum aus Chunks. Jeder Chunk hat einen
Kopf aus 2 Byte Kennung und 4 Byte Laenge, die den Kopf mitzaehlt, alles
little-endian. Die Kennungen unten stehen so in bb_loader.h.
"""

import os
import struct

LE = "<"


def chunk(cid, payload=b"", subs=b""):
    body = payload + subs
    return struct.pack(LE + "HI", cid, 6 + len(body)) + body


def cstr(s):
    return s.encode("latin-1") + b"\0"


def material(name, rgb):
    """0xAFFF mit Name (0xA000) und Diffusfarbe (0xA020 -> 0x0011)."""
    return chunk(0xAFFF, b"",
                 chunk(0xA000, cstr(name)) +
                 chunk(0xA020, b"", chunk(0x0011, bytes(rgb))))


def box(x0, x1, y0, y1, z0, z1):
    """Acht Ecken und zwoelf Dreiecke eines achsenparallelen Quaders."""
    v = [(x0, y0, z0), (x1, y0, z0), (x1, y1, z0), (x0, y1, z0),
         (x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1)]
    f = [(0, 2, 1), (0, 3, 2), (4, 5, 6), (4, 6, 7),
         (0, 1, 5), (0, 5, 4), (1, 2, 6), (1, 6, 5),
         (2, 3, 7), (2, 7, 6), (3, 0, 4), (3, 4, 7)]
    return v, f


def trimesh(name, verts, faces, matname, uvs):
    vb = struct.pack(LE + "H", len(verts)) + \
        b"".join(struct.pack(LE + "fff", *p) for p in verts)
    ub = struct.pack(LE + "H", len(uvs)) + \
        b"".join(struct.pack(LE + "ff", *p) for p in uvs)
    fb = struct.pack(LE + "H", len(faces)) + \
        b"".join(struct.pack(LE + "HHHH", a, b, c, 0) for a, b, c in faces)
    # 0x4130: alle Dreiecke gehoeren zu diesem Material
    grp = chunk(0x4130, cstr(matname) + struct.pack(LE + "H", len(faces)) +
                b"".join(struct.pack(LE + "H", i) for i in range(len(faces))))
    axes = struct.pack(LE + "12f", 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0)
    tri = chunk(0x4100, b"",
                chunk(0x4110, vb) + chunk(0x4140, ub) +
                chunk(0x4120, fb, grp) + chunk(0x4160, axes))
    return chunk(0x4000, cstr(name), tri)


def main():
    out = os.path.join("tests", "assets")
    os.makedirs(out, exist_ok=True)

    quad_uv = [(0.0, 0.0), (1.0, 0.0), (1.0, 1.0), (0.0, 1.0)] * 2
    va, fa = box(-1, 1, -2, 2, -3, 3)      # x=2, y=4, z=6
    vb, fb = box(10, 11, 0, 1, 0, 1)       # verschoben, zweites Material

    data = chunk(0x4D4D, b"",
                 chunk(0x0002, struct.pack(LE + "I", 3)) +
                 chunk(0x3D3D, b"",
                       material("orange", (255, 128, 0)) +
                       material("blau", (0, 0, 255)) +
                       trimesh("kasten", va, fa, "orange", quad_uv) +
                       trimesh("wuerfel", vb, fb, "blau", quad_uv)))
    path = os.path.join(out, "test_box.3ds")
    with open(path, "wb") as f:
        f.write(data)

    allv = va + vb
    ext = [max(p[k] for p in allv) - min(p[k] for p in allv) for k in range(3)]
    print("%s: %d Bytes" % (path, len(data)))
    print("  3DS-Ausdehnung  x=%g y=%g z=%g" % tuple(ext))
    print("  erwartet Blitz  w=%g h=%g d=%g" % (ext[0], ext[2], ext[1]))
    print("  Dreiecke %d, Flaechen 2" % (len(fa) + len(fb)))

    broken = struct.pack(LE + "HI", 0x4D4D, 999999) + b"kaputt"
    path = os.path.join(out, "test_broken.3ds")
    with open(path, "wb") as f:
        f.write(broken)
    print("%s: %d Bytes (Laengenangabe zeigt hinter das Dateiende)"
          % (path, len(broken)))

    # BUG-126: ein Dreieck in beiden Reihenfolgen. In Blitz-Koordinaten liegen
    # die Ecken bei (-1,1,0), (1,1,0), (1,-1,0); in der Datei stehen y und z
    # getauscht. Der Tausch kehrt den Umlaufsinn um, deshalb ist hier die
    # Datei mit (0,2,1) die im Original von vorn sichtbare.
    ecken = [(-1.0, 0.0, 1.0), (1.0, 0.0, 1.0), (1.0, 0.0, -1.0)]
    for name, face in (("test_tri_cw.3ds", (0, 2, 1)),
                       ("test_tri_ccw.3ds", (0, 1, 2))):
        data = chunk(0x4D4D, b"",
                     chunk(0x0002, struct.pack(LE + "I", 3)) +
                     chunk(0x3D3D, b"",
                           material("weiss", (255, 255, 255)) +
                           trimesh("dreieck", ecken, [face], "weiss",
                                   [(0.0, 0.0), (1.0, 0.0), (1.0, 1.0)])))
        path = os.path.join(out, name)
        with open(path, "wb") as f:
            f.write(data)
        print("%s: %d Bytes" % (path, len(data)))


if __name__ == "__main__":
    main()
