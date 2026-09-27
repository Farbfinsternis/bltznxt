#!/usr/bin/env python3
"""Erzeugt die glTF-Testdateien fuer tests/assets/ (3D-24).

Blender gehoert nicht zur Testumgebung; diese Dateien sind klein, von Hand
gebaut und treffen die Stellen, an denen ein glTF-Leser danebengreifen kann:

    test_gltf_szene.glb   Binaer, alles im BIN-Chunk. Szene mit vier
                          Wurzelknoten:
                          "rahmen"  T(1,2,3), 90 Grad um +Y (glTF)
                            "quad"  T(2,0,0), Netz mit drei Primitiven:
                                    0: Dreieck, Indizes u16, Material "rot",
                                       COLOR_0 float (1, 0.0512695, 0, 1),
                                       zwei UV-Saetze, Normalen +Z
                                    1: Streifen (mode 5) ohne Indizes, vier
                                       Vertices, Material "grau", ohne Farben
                                    2: Dreieck, Indizes u8, Material "rot",
                                       COLOR_0 u8 normiert (255,13,0,255),
                                       POSITION mit sparse-Ersetzung von
                                       Vertex 0 auf (0,0,0.5)
                          "leer"    T(0,0,5), ohne Netz
                          "spawn"   als matrix, Verschiebung (4,5,6)
                          "bild"    ohne Netz, nutzt das eingebettete Bild
                                    ueber ein Material, das kein Primitive
                                    verwendet (darf nicht stoeren)

    test_gltf_bild.gltf   JSON; der Puffer ist eine data:-URI (Base64), zwei
                          Bilder liegen als PNG daneben, eines ist eine
                          data:-URI. Drei Quadrate bei glTF-z = -5, von +Z
                          aus sichtbar (in Blitz3D bei z = +5 vor der
                          Kamera):
                          links  x -3..-1  Lightmap: weisse Grundtextur auf
                                 TEXCOORD_0 = (0.25,0.5), Occlusion-Textur
                                 blau|rot auf TEXCOORD_1 = (0.75,0.5) - rot,
                                 wenn Satz 1 ankommt, blau, wenn nicht
                          Mitte  x -1..1   MASK: gruen|durchsichtig ueber u
                          rechts x 1..3    BLEND: weiss mit Alpha 0.5
                          Alle unlit (KHR_materials_unlit).
    test_gltf_licht.png, test_gltf_maske.png   die beiden Bilder dazu

    test_gltf_kaputt.gltf kein JSON
    test_gltf_draco.gltf  verlangt KHR_draco_mesh_compression

Aufruf aus dem Projektwurzelverzeichnis:

    python scripts/make_gltf_asset.py
"""

import base64
import json
import math
import os
import struct
import zlib

OUT = os.path.join("tests", "assets")


def png(w, h, rgba):
    """RGBA-Pixel (Zeilen von oben) als PNG-Bytes."""
    raw = b"".join(b"\0" + bytes(rgba[y * w * 4:(y + 1) * w * 4]) for y in range(h))

    def chunk(tag, data):
        c = tag + data
        return struct.pack(">I", len(data)) + c + struct.pack(">I", zlib.crc32(c) & 0xFFFFFFFF)

    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0)) +
            chunk(b"IDAT", zlib.compress(raw)) + chunk(b"IEND", b""))


class Buf:
    """Ein Puffer mit bufferViews und accessors."""

    def __init__(self):
        self.data = bytearray()
        self.views = []
        self.accessors = []

    def view(self, blob):
        while len(self.data) % 4:
            self.data += b"\0"
        self.views.append({"buffer": 0, "byteOffset": len(self.data), "byteLength": len(blob)})
        self.data += blob
        return len(self.views) - 1

    def acc(self, fmt, ctype, typ, values, normalized=False, minmax=False, sparse=None):
        n = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4}[typ]
        flat = [c for v in values for c in (v if isinstance(v, (list, tuple)) else [v])]
        a = {"bufferView": self.view(struct.pack("<%d%s" % (len(flat), fmt), *flat)),
             "componentType": ctype, "count": len(values), "type": typ}
        if normalized:
            a["normalized"] = True
        if minmax:
            a["min"] = [min(v[i] for v in values) for i in range(n)]
            a["max"] = [max(v[i] for v in values) for i in range(n)]
        if sparse:
            idx, vals = sparse
            a["sparse"] = {
                "count": len(idx),
                "indices": {"bufferView": self.view(struct.pack("<%dH" % len(idx), *idx)),
                            "componentType": 5123},
                "values": {"bufferView": self.view(struct.pack("<%df" % (len(vals) * n),
                                                               *[c for v in vals for c in v]))},
            }
        self.accessors.append(a)
        return len(self.accessors) - 1


F32, U8, U16 = 5126, 5121, 5123


def szene():
    b = Buf()
    # Primitive 0: ein Dreieck in der xy-Ebene, von +Z aus gegen den Uhrzeigersinn.
    p0 = {
        "attributes": {
            "POSITION": b.acc("f", F32, "VEC3", [(0, 0, 0.25), (1, 0, 0.25), (0, 1, 0.25)], minmax=True),
            "NORMAL": b.acc("f", F32, "VEC3", [(0, 0, 1)] * 3),
            "TEXCOORD_0": b.acc("f", F32, "VEC2", [(0.1, 0.2), (0.5, 0.2), (0.1, 0.6)]),
            "TEXCOORD_1": b.acc("f", F32, "VEC2", [(0.3, 0.4), (0.7, 0.4), (0.3, 0.8)]),
            "COLOR_0": b.acc("f", F32, "VEC4", [(1, 0.0512695, 0, 1)] * 3),
        },
        "indices": b.acc("H", U16, "SCALAR", [0, 1, 2]),
        "material": 0,
    }
    # Primitive 1: Streifen aus vier Vertices, ohne Indizes.
    p1 = {
        "attributes": {
            "POSITION": b.acc("f", F32, "VEC3", [(0, 0, 0), (1, 0, 0), (0, -1, 0), (1, -1, 0)], minmax=True),
            "NORMAL": b.acc("f", F32, "VEC3", [(0, 0, 1)] * 4),
        },
        "mode": 5,
        "material": 1,
    }
    # Primitive 2: Farben als normierte Bytes, Vertex 0 per sparse ersetzt.
    p2 = {
        "attributes": {
            "POSITION": b.acc("f", F32, "VEC3", [(0, 0, 0), (-1, 0, 0), (0, -1, 0)],
                              sparse=([0], [(0, 0, 0.5)])),
            "NORMAL": b.acc("f", F32, "VEC3", [(0, 0, 1)] * 3),
            "COLOR_0": b.acc("B", U8, "VEC4", [(255, 13, 0, 255)] * 3, normalized=True),
        },
        "indices": b.acc("B", U8, "SCALAR", [0, 2, 1]),
        "material": 0,
    }
    img = b.view(png(1, 1, [255, 255, 255, 255]))
    s = math.sqrt(0.5)
    doc = {
        "asset": {"version": "2.0", "generator": "make_gltf_asset.py"},
        "extensionsUsed": ["KHR_materials_unlit"],
        "scene": 0,
        "scenes": [{"nodes": [0, 2, 3, 4]}],
        "nodes": [
            {"name": "rahmen", "translation": [1, 2, 3], "rotation": [0, s, 0, s], "children": [1]},
            {"name": "quad", "translation": [2, 0, 0], "mesh": 0},
            {"name": "leer", "translation": [0, 0, 5]},
            {"name": "spawn", "matrix": [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 4, 5, 6, 1]},
            {"name": "bild"},
        ],
        "meshes": [{"name": "quad", "primitives": [p0, p1, p2]}],
        "materials": [
            {"name": "rot", "pbrMetallicRoughness": {"baseColorFactor": [1, 0, 0, 1]},
             "extensions": {"KHR_materials_unlit": {}}},
            {"name": "grau", "pbrMetallicRoughness": {"baseColorFactor": [0.2140, 0.2140, 0.2140, 1]}},
            {"name": "ungenutzt", "pbrMetallicRoughness": {"baseColorTexture": {"index": 0}}},
        ],
        "textures": [{"source": 0}],
        "images": [{"bufferView": img, "mimeType": "image/png"}],
        "bufferViews": b.views,
        "accessors": b.accessors,
        "buffers": [{"byteLength": len(b.data)}],
    }
    js = json.dumps(doc, separators=(",", ":")).encode()
    js += b" " * (-len(js) % 4)
    bn = bytes(b.data) + b"\0" * (-len(b.data) % 4)
    body = (struct.pack("<II", len(js), 0x4E4F534A) + js + struct.pack("<II", len(bn), 0x004E4942) + bn)
    with open(os.path.join(OUT, "test_gltf_szene.glb"), "wb") as f:
        f.write(b"glTF" + struct.pack("<II", 2, 12 + len(body)) + body)


def bild():
    b = Buf()

    def quad(x0, x1, uv0, uv1=None):
        pos = [(x0, -1, -5), (x1, -1, -5), (x1, 1, -5), (x0, 1, -5)]
        at = {"POSITION": b.acc("f", F32, "VEC3", pos, minmax=True),
              "NORMAL": b.acc("f", F32, "VEC3", [(0, 0, 1)] * 4),
              "TEXCOORD_0": b.acc("f", F32, "VEC2", uv0)}
        if uv1:
            at["TEXCOORD_1"] = b.acc("f", F32, "VEC2", uv1)
        return at, b.acc("H", U16, "SCALAR", [0, 1, 2, 0, 2, 3])

    a0, i0 = quad(-3, -1, [(0.25, 0.5)] * 4, [(0.75, 0.5)] * 4)
    a1, i1 = quad(-1, 1, [(0, 1), (1, 1), (1, 0), (0, 0)])
    a2, i2 = quad(1, 3, [(0, 1), (1, 1), (1, 0), (0, 0)])
    unlit = {"KHR_materials_unlit": {}}
    doc = {
        "asset": {"version": "2.0", "generator": "make_gltf_asset.py"},
        "extensionsUsed": ["KHR_materials_unlit"],
        "scenes": [{"nodes": [0]}],
        "nodes": [{"name": "tafeln", "mesh": 0}],
        "meshes": [{"primitives": [
            {"attributes": a0, "indices": i0, "material": 0},
            {"attributes": a1, "indices": i1, "material": 1},
            {"attributes": a2, "indices": i2, "material": 2},
        ]}],
        "materials": [
            {"name": "licht", "extensions": unlit,
             "pbrMetallicRoughness": {"baseColorTexture": {"index": 0}},
             "occlusionTexture": {"index": 1, "texCoord": 1}},
            {"name": "maske", "extensions": unlit, "alphaMode": "MASK", "doubleSided": True,
             "pbrMetallicRoughness": {"baseColorTexture": {"index": 2}}},
            {"name": "glas", "extensions": unlit, "alphaMode": "BLEND",
             "pbrMetallicRoughness": {"baseColorFactor": [1, 1, 1, 0.5]}},
        ],
        "samplers": [{"magFilter": 9728, "minFilter": 9728, "wrapS": 33071, "wrapT": 33071}],
        "textures": [{"source": 0}, {"source": 1, "sampler": 0}, {"source": 2, "sampler": 0}],
        "images": [
            {"uri": "data:image/png;base64," + base64.b64encode(png(1, 1, [255, 255, 255, 255])).decode()},
            {"uri": "test_gltf_licht.png"},
            {"uri": "test_gltf_maske.png"},
        ],
        "bufferViews": b.views,
        "accessors": b.accessors,
        "buffers": [{"byteLength": len(b.data),
                     "uri": "data:application/octet-stream;base64," + base64.b64encode(bytes(b.data)).decode()}],
    }
    with open(os.path.join(OUT, "test_gltf_bild.gltf"), "w", newline="\n") as f:
        json.dump(doc, f, indent=1)
    with open(os.path.join(OUT, "test_gltf_licht.png"), "wb") as f:
        f.write(png(2, 1, [0, 0, 255, 255, 255, 0, 0, 255]))
    with open(os.path.join(OUT, "test_gltf_maske.png"), "wb") as f:
        f.write(png(2, 1, [0, 255, 0, 255, 0, 255, 0, 0]))


def fehler():
    with open(os.path.join(OUT, "test_gltf_kaputt.gltf"), "w", newline="\n") as f:
        f.write('{ "asset": { "version": "2.0" }, nope }\n')
    with open(os.path.join(OUT, "test_gltf_draco.gltf"), "w", newline="\n") as f:
        json.dump({"asset": {"version": "2.0"},
                   "extensionsUsed": ["KHR_draco_mesh_compression"],
                   "extensionsRequired": ["KHR_draco_mesh_compression"]}, f)
        f.write("\n")


if __name__ == "__main__":
    szene()
    bild()
    fehler()
