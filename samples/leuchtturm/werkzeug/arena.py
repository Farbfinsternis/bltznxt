#!/usr/bin/env python3
"""Platzhalter-Arena fuer das Leuchtturm-Spiel (Schritt 3: Bewegung und Kollision).

Bis es eine Karte aus Blender gibt, steht hier eine kleine Arena, gebaut nach
denselben Regeln (LEUCHTTURM.md, "Die Karte in Blender"): eine .glb, alles
Wissen steckt in den Objektnamen.

    arena       sichtbare Geometrie, kollidiert nicht
    arena-col   dieselben Koerper als Kollisionsgeometrie (unsichtbar)
    spawn       leeres Objekt: Startpunkt, Blick nach Blitz +z

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

Aufruf aus dem Projektwurzelverzeichnis:

    python samples/leuchtturm/werkzeug/arena.py
"""

import json
import os
import struct

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "..", "daten", "arena.glb")


def box(x0, y0, z0, x1, y1, z1):
    """Quader als Liste von Flaechen (je eine Liste von Ecken), Blitz-Koordinaten."""
    p = [(x, y, z) for x in (x0, x1) for y in (y0, y1) for z in (z0, z1)]
    # Index = 4*ix + 2*iy + iz
    faces = [(0, 1, 3, 2), (4, 6, 7, 5), (0, 4, 5, 1), (2, 3, 7, 6), (0, 2, 6, 4), (1, 5, 7, 3)]
    return [[p[i] for i in f] for f in faces]


def wedge(x0, x1, z0, z1, h):
    """Rampe: steigt in x von 0 (bei x0) auf h (bei x1)."""
    a, b = (x0, 0, z0), (x1, 0, z0)
    c, d = (x1, 0, z1), (x0, 0, z1)
    e, f = (x1, h, z0), (x1, h, z1)
    return [[a, b, c, d], [a, e, b], [d, c, f], [b, e, f, c], [a, d, f, e]]


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


def linear(c):
    """Farben oben sind so gemeint, wie man sie sieht (sRGB); glTF-Vertexfarben
    sind linear, und der Lader wandelt sie wieder zurueck."""
    return tuple(v / 12.92 if v <= 0.04045 else ((v + 0.055) / 1.055) ** 2.4 for v in c)


def sub(a, b): return tuple(a[i] - b[i] for i in range(3))
def cross(a, b): return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])
def dot(a, b): return sum(a[i] * b[i] for i in range(3))


def mesh_data(with_color):
    """Flache Dreiecke, Normalen nach aussen, schon in glTF-Koordinaten."""
    pos, nrm, col, idx = [], [], [], []
    for faces, color in SOLIDS:
        corners = [v for f in faces for v in f]
        center = tuple(sum(v[i] for v in corners) / len(corners) for i in range(3))
        for f in faces:
            g = [(v[0], v[1], -v[2]) for v in f]                  # Blitz -> glTF
            gc = (center[0], center[1], -center[2])
            n = cross(sub(g[1], g[0]), sub(g[2], g[0]))
            fc = tuple(sum(v[i] for v in g) / len(g) for i in range(3))
            if dot(n, sub(fc, gc)) < 0:                            # nach aussen, gegen den Uhrzeigersinn
                g.reverse()
                n = tuple(-c for c in n)
            l = dot(n, n) ** 0.5
            n = tuple(c / l for c in n)
            base = len(pos)
            for v in g:
                pos.append(v)
                nrm.append(n)
                col.append(linear(color) + (1.0,))
            for k in range(1, len(g) - 1):
                idx += [base, base + k, base + k + 1]
    return pos, nrm, (col if with_color else None), idx


def main():
    data = bytearray()
    views, accessors = [], []

    def acc(fmt, ctype, typ, values):
        n = {"SCALAR": 1, "VEC3": 3, "VEC4": 4}[typ]
        flat = [c for v in values for c in (v if isinstance(v, tuple) else (v,))]
        while len(data) % 4:
            data.append(0)
        blob = struct.pack("<%d%s" % (len(flat), fmt), *flat)
        views.append({"buffer": 0, "byteOffset": len(data), "byteLength": len(blob)})
        data.extend(blob)
        a = {"bufferView": len(views) - 1, "componentType": ctype, "count": len(values), "type": typ}
        if typ == "VEC3" and fmt == "f":
            a["min"] = [min(v[i] for v in values) for i in range(n)]
            a["max"] = [max(v[i] for v in values) for i in range(n)]
        accessors.append(a)
        return len(accessors) - 1

    meshes = []
    for with_color in (True, False):
        pos, nrm, col, idx = mesh_data(with_color)
        at = {"POSITION": acc("f", 5126, "VEC3", pos), "NORMAL": acc("f", 5126, "VEC3", nrm)}
        if col:
            at["COLOR_0"] = acc("f", 5126, "VEC4", col)
        meshes.append({"primitives": [{"attributes": at, "indices": acc("H", 5123, "SCALAR", idx),
                                       "material": 0}]})

    doc = {
        "asset": {"version": "2.0", "generator": "BLTZNXT samples/leuchtturm/werkzeug/arena.py"},
        "scene": 0,
        "scenes": [{"nodes": [0, 1, 2]}],
        "nodes": [
            {"name": "arena", "mesh": 0},
            {"name": "arena-col", "mesh": 1},
            {"name": "spawn", "translation": [0, 0, 15]},
        ],
        "meshes": meshes,
        "materials": [{"name": "platzhalter",
                       "pbrMetallicRoughness": {"metallicFactor": 0.0, "roughnessFactor": 1.0}}],
        "bufferViews": views,
        "accessors": accessors,
        "buffers": [{"byteLength": len(data)}],
    }
    js = json.dumps(doc, separators=(",", ":")).encode()
    js += b" " * (-len(js) % 4)
    bn = bytes(data) + b"\0" * (-len(data) % 4)
    body = struct.pack("<II", len(js), 0x4E4F534A) + js + struct.pack("<II", len(bn), 0x004E4942) + bn
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "wb") as f:
        f.write(b"glTF" + struct.pack("<II", 2, 12 + len(body)) + body)
    print("%s: %d Dreiecke" % (os.path.normpath(OUT), len(mesh_data(False)[3]) // 3))


if __name__ == "__main__":
    main()
