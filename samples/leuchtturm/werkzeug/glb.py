"""Gemeinsames fuer die Werkzeuge des Leuchtturm-Spiels: Koerper bauen und als
glTF Binary (.glb) schreiben.

Alles hier rechnet in Blitz-Koordinaten (y oben, z nach vorn, linkshaendig);
erst beim Schreiben wird z gespiegelt, weil glTF rechtshaendig ist - der
Lader spiegelt zurueck. Masse in Metern.

Ein Koerper ist eine Liste von Flaechen, jede eine Liste von Ecken. Ein Netz
entsteht aus Koerpern mit Farbe; die Normalen zeigen nach aussen (vom
Mittelpunkt des Koerpers weg), die Flaechen sind flach schattiert.
"""

import json
import math
import os
import struct


def box(x0, y0, z0, x1, y1, z1):
    """Quader als Liste von Flaechen."""
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


def cylinder(x, y, z0, z1, r, n=12):
    """Zylinder entlang z, Mittelachse bei (x, y)."""
    ring = [(x + r * math.cos(2 * math.pi * k / n), y + r * math.sin(2 * math.pi * k / n)) for k in range(n)]
    faces = []
    for k in range(n):
        (ax, ay), (bx, by) = ring[k], ring[(k + 1) % n]
        faces.append([(ax, ay, z0), (bx, by, z0), (bx, by, z1), (ax, ay, z1)])
    faces.append([(px, py, z0) for px, py in ring])
    faces.append([(px, py, z1) for px, py in ring])
    return faces


def octahedron(r):
    """Doppelpyramide um den Ursprung, acht Flaechen."""
    top, bot = (0, r, 0), (0, -r, 0)
    ring = [(r, 0, 0), (0, 0, r), (-r, 0, 0), (0, 0, -r)]
    faces = []
    for k in range(4):
        a, b = ring[k], ring[(k + 1) % 4]
        faces.append([top, a, b])
        faces.append([bot, b, a])
    return faces


def linear(c):
    """Farben sind so gemeint, wie man sie sieht (sRGB); glTF-Vertexfarben
    sind linear, und der Lader wandelt sie wieder zurueck."""
    return tuple(v / 12.92 if v <= 0.04045 else ((v + 0.055) / 1.055) ** 2.4 for v in c)


def sub(a, b): return tuple(a[i] - b[i] for i in range(3))
def cross(a, b): return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])
def dot(a, b): return sum(a[i] * b[i] for i in range(3))


def mesh(solids, with_color=True):
    """solids: Liste von (Flaechen, Farbe) oder (Flaechen, [Farbe je Flaeche]).
    Liefert (pos, nrm, col, idx) in glTF-Koordinaten."""
    pos, nrm, col, idx = [], [], [], []
    for faces, color in solids:
        corners = [v for f in faces for v in f]
        center = tuple(sum(v[i] for v in corners) / len(corners) for i in range(3))
        for fi, f in enumerate(faces):
            fcol = color[fi % len(color)] if isinstance(color, list) else color
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
                col.append(linear(fcol) + (1.0,))
            for k in range(1, len(g) - 1):
                idx += [base, base + k, base + k + 1]
    return pos, nrm, (col if with_color else None), idx


def pos(x, y, z):
    """Eine Lage in Blitz-Koordinaten als glTF-translation."""
    return (x, y, -z)


def quat(ax, ay, az, deg):
    """Drehung um die Achse (ax, ay, az) in Blitz-Koordinaten, als glTF-Quaternion.
    Positiv dreht wie Blitz3D: um x hebt sich die Spitze (+z) nach unten, wie
    ein positiver Nickwinkel; um z dreht es im Uhrzeigersinn, von hinten gesehen.
    Der Lader spiegelt z: aus glTF (x, y, z, w) wird Blitz (-x, -y, z, w)."""
    l = (ax * ax + ay * ay + az * az) ** 0.5
    s, c = math.sin(math.radians(deg) / 2), math.cos(math.radians(deg) / 2)
    return (-ax / l * s, -ay / l * s, az / l * s, c)


def write(path, nodes, meshes, generator, animations=(), texture=None, texture_mime="image/png"):
    """nodes: glTF-Knoten (dicts, "mesh" verweist in meshes); in der Szene stehen
    die, die nicht Kind eines anderen sind.
    meshes: Liste von (pos, nrm, col, idx) aus mesh(), oder (pos, nrm, col,
    idx, uv) mit Texturkoordinaten. Ein Material fuer alle; mit `texture`
    (Inhalt einer PNG- oder JPEG-Datei, texture_mime) liegt die als Textur in der Datei.
    animations: Liste von (name, [(knoten, pfad, [(bild, wert), ...]), ...]) -
    pfad "translation" (Wert aus pos()), "rotation" (aus quat()) oder "scale";
    Bilder zu 60 je Sekunde, wie der Lader sie zaehlt. Linear gemischt."""
    data = bytearray()
    views, accessors = [], []

    def acc(fmt, ctype, typ, values):
        n = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4}[typ]
        flat = [c for v in values for c in (v if isinstance(v, tuple) else (v,))]
        while len(data) % 4:
            data.append(0)
        blob = struct.pack("<%d%s" % (len(flat), fmt), *flat)
        views.append({"buffer": 0, "byteOffset": len(data), "byteLength": len(blob)})
        data.extend(blob)
        a = {"bufferView": len(views) - 1, "componentType": ctype, "count": len(values), "type": typ}
        if typ == "SCALAR" and fmt == "f":                          # Zeiten einer Animation
            a["min"], a["max"] = [min(values)], [max(values)]
        if typ == "VEC3" and fmt == "f":
            a["min"] = [min(v[i] for v in values) for i in range(n)]
            a["max"] = [max(v[i] for v in values) for i in range(n)]
        accessors.append(a)
        return len(accessors) - 1

    gmeshes = []
    for m in meshes:
        pos, nrm, col, idx = m[:4]
        at = {"POSITION": acc("f", 5126, "VEC3", pos), "NORMAL": acc("f", 5126, "VEC3", nrm)}
        if col:
            at["COLOR_0"] = acc("f", 5126, "VEC4", col)
        if len(m) > 4:
            at["TEXCOORD_0"] = acc("f", 5126, "VEC2", m[4])
        gmeshes.append({"primitives": [{"attributes": at, "indices": acc("H", 5123, "SCALAR", idx),
                                        "material": 0}]})

    ganims = []
    for name, channels in animations:
        samplers, chans = [], []
        for node, kanal, keys in channels:
            typ = "VEC4" if kanal == "rotation" else "VEC3"
            samplers.append({"input": acc("f", 5126, "SCALAR", [k[0] / 60.0 for k in keys]),
                             "output": acc("f", 5126, typ, [tuple(k[1]) for k in keys]),
                             "interpolation": "LINEAR"})
            chans.append({"sampler": len(samplers) - 1, "target": {"node": node, "path": kanal}})
        ganims.append({"name": name, "samplers": samplers, "channels": chans})

    doc = {
        "asset": {"version": "2.0", "generator": generator},
        "scene": 0,
        "scenes": [{"nodes": [i for i in range(len(nodes))
                              if not any(i in n.get("children", ()) for n in nodes)]}],
        "nodes": nodes,
        "meshes": gmeshes,
        "materials": [{"name": "platzhalter",
                       "pbrMetallicRoughness": {"metallicFactor": 0.0, "roughnessFactor": 1.0}}],
        "bufferViews": views,
        "accessors": accessors,
        "buffers": [{"byteLength": len(data)}],
    }
    if ganims:
        doc["animations"] = ganims
    if texture:
        while len(data) % 4:
            data.append(0)
        views.append({"buffer": 0, "byteOffset": len(data), "byteLength": len(texture)})
        data.extend(texture)
        doc["buffers"][0]["byteLength"] = len(data)
        doc["images"] = [{"bufferView": len(views) - 1, "mimeType": texture_mime}]
        doc["samplers"] = [{"magFilter": 9729, "minFilter": 9987}]
        doc["textures"] = [{"sampler": 0, "source": 0}]
        doc["materials"] = [{"name": "colormap", "pbrMetallicRoughness": {
            "baseColorTexture": {"index": 0}, "metallicFactor": 0.0, "roughnessFactor": 1.0}}]
    js = json.dumps(doc, separators=(",", ":")).encode()
    js += b" " * (-len(js) % 4)
    bn = bytes(data) + b"\0" * (-len(data) % 4)
    body = struct.pack("<II", len(js), 0x4E4F534A) + js + struct.pack("<II", len(bn), 0x004E4942) + bn
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as f:
        f.write(b"glTF" + struct.pack("<II", 2, 12 + len(body)) + body)
