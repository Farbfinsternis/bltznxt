#!/usr/bin/env python3
"""Wandelt ein MD2-Modell (Quake II) in eine .glb-Datei fuer LoadAnimMesh (3D-24).

MD2 ist Vertex-Animation: jedes Bild ist ein vollstaendiger Satz Vertices. In
glTF wird daraus ein Netz mit Morph Targets - Bild 0 ist die Ruhelage, jedes
weitere Bild ein Ziel (Versatz von Lage und Normale gegen Bild 0) - und eine
Animation, deren Kanal "weights" bei Bild i genau Ziel i auf 1 stellt. Das
lineare Mischen zwischen zwei Schluesseln ist dann dasselbe wie das Mischen
zweier MD2-Bilder.

Damit das Modell in BLTZNXT genau dort liegt, wo LoadMD2 es hinlegt, folgt
die Umrechnung dem am Original gemessenen MD2-Lader (src/compiler/bb_md2.h):
Blitz (x,y,z) = MD2 (y,z,x), auch fuer die 162 Normalen der Tabelle; die
Vertices sind die Paare (Punkt, UV) in der Reihenfolge ihres ersten
Auftretens; UV = s/skinWidth, t/skinHeight. Von Blitz nach glTF wird z
gespiegelt (der glTF-Lader spiegelt es zurueck), und die Dreiecke behalten
die Reihenfolge der Datei - der Lader dreht sie wie bb_md2.h auf 0,2,1.

Zeit: der glTF-Lader rechnet 60 Bilder je Sekunde, also steht MD2-Bild i bei
i/60 s und wird Blitz-Bild i. `Animate m,1,speed` laeuft damit so schnell wie
`AnimateMD2 m,1,speed,0,letztes`. Wie bei AnimateMD2 im Loop wird das letzte
Bild dabei nicht gezeigt (es steht an der Stelle des ersten); fuer andere
Bereiche gibt es ExtractAnimSeq.

MD2 bringt keine Textur mit (LoadMD2 nimmt die der Entity). Mit --texture
wird ein Bild (BMP mit 8 oder 24 Bit, oder PNG) als Grundtextur eingebettet;
sonst setzt das Programm sie wie bisher mit EntityTexture - dann aber auf das
Kind mit dem Netz, nicht auf die Wurzel.

Aufruf:

    python scripts/md2_to_gltf.py Bird.md2 Bird.glb --texture Textures/Bird.bmp
"""

import argparse
import json
import os
import re
import struct
import zlib

HERE = os.path.dirname(os.path.abspath(__file__))


def md2_normals():
    """Die Normalentabelle aus bb_md2.h, dort noch in MD2-Achsen."""
    src = open(os.path.join(HERE, "..", "src", "compiler", "bb_md2.h"), encoding="utf-8").read()
    body = src[src.index("static const float raw[162][3]"):]
    vals = re.findall(r"\{(-?[\d.]+)f, (-?[\d.]+)f, (-?[\d.]+)f\}", body)[:162]
    assert len(vals) == 162
    return [tuple(float(c) for c in v) for v in vals]


def read_bmp(path):
    """Unkomprimiertes BMP mit 8 (Palette) oder 24 Bit -> (w, h, RGBA von oben)."""
    d = open(path, "rb").read()
    if d[:2] != b"BM":
        raise ValueError("kein BMP: " + path)
    off = struct.unpack_from("<I", d, 10)[0]
    hsize, w, h, planes, bpp, comp = struct.unpack_from("<IiiHHI", d, 14)
    if comp != 0 or bpp not in (8, 24):
        raise ValueError("nur unkomprimierte BMP mit 8 oder 24 Bit: " + path)
    ncol = struct.unpack_from("<I", d, 46)[0] or (256 if bpp == 8 else 0)
    pal = [d[14 + hsize + 4 * i:14 + hsize + 4 * i + 3] for i in range(ncol)]
    top_down = h < 0
    h = abs(h)
    stride = (w * bpp // 8 + 3) & ~3
    px = bytearray()
    for y in range(h):
        row = off + (y if top_down else h - 1 - y) * stride
        for x in range(w):
            if bpp == 8:
                b, g, r = pal[d[row + x]]
            else:
                b, g, r = d[row + 3 * x:row + 3 * x + 3]
            px += bytes((r, g, b, 255))
    return w, h, px


def png(w, h, rgba):
    raw = b"".join(b"\0" + bytes(rgba[y * w * 4:(y + 1) * w * 4]) for y in range(h))

    def chunk(tag, data):
        c = tag + data
        return struct.pack(">I", len(data)) + c + struct.pack(">I", zlib.crc32(c) & 0xFFFFFFFF)

    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0)) +
            chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b""))


def read_md2(path):
    d = open(path, "rb").read()
    (magic, version, skin_w, skin_h, frame_size, n_skins, n_xyz, n_st, n_tris, n_gl, n_frames,
     o_skins, o_st, o_tris, o_frames, o_gl, o_end) = struct.unpack_from("<4s16i", d, 0)
    if magic != b"IDP2" or version != 8:
        raise ValueError("kein MD2 der Version 8: " + path)
    st = [struct.unpack_from("<hh", d, o_st + 4 * i) for i in range(n_st)]
    tris = [struct.unpack_from("<6H", d, o_tris + 12 * i) for i in range(n_tris)]
    frames = []
    for f in range(n_frames):
        o = o_frames + f * frame_size
        s = struct.unpack_from("<3f", d, o)
        t = struct.unpack_from("<3f", d, o + 12)
        name = d[o + 24:o + 40].split(b"\0")[0].decode("latin-1")
        v = [tuple(d[o + 40 + 4 * k:o + 44 + 4 * k]) for k in range(n_xyz)]
        frames.append((s, t, name, v))
    return skin_w, skin_h, st, tris, frames


def convert(md2_path, out_path, texture=None):
    norms = md2_normals()
    skin_w, skin_h, st, tris, frames = read_md2(md2_path)

    # Vertices wie bb_md2.h: (Punkt, UV) in der Reihenfolge des ersten Auftretens.
    seen, verts, uvs, idx = {}, [], [], []
    for t in tris:
        for j in range(3):
            key = (t[3 + j], t[j])
            if key not in seen:
                seen[key] = len(verts)
                verts.append(t[j])
                u, v = st[t[3 + j]] if t[3 + j] < len(st) else (0, 0)
                uvs.append((u / skin_w, v / skin_h))
            idx.append(seen[key])

    def frame_data(f):
        s, t, _, raw = frames[f]
        pos, nrm = [], []
        for k in verts:
            mv = raw[k % len(raw)]
            # Blitz = MD2 (y, z, x); glTF = Blitz mit gespiegeltem z
            bx = mv[1] * s[1] + t[1]
            by = mv[2] * s[2] + t[2]
            bz = mv[0] * s[0] + t[0]
            pos.append((bx, by, -bz))
            n = norms[mv[3] if mv[3] < 162 else 0]
            nrm.append((n[1], n[2], -n[0]))
        return pos, nrm

    data = bytearray()
    views, accessors = [], []

    def acc(fmt, ctype, typ, values, minmax=False):
        n = {"SCALAR": 1, "VEC2": 2, "VEC3": 3}[typ]
        flat = [c for v in values for c in (v if isinstance(v, tuple) else (v,))]
        while len(data) % 4:
            data.append(0)
        blob = struct.pack("<%d%s" % (len(flat), fmt), *flat)
        views.append({"buffer": 0, "byteOffset": len(data), "byteLength": len(blob)})
        data.extend(blob)
        a = {"bufferView": len(views) - 1, "componentType": ctype, "count": len(values), "type": typ}
        if minmax:
            a["min"] = [min(v[i] for v in values) for i in range(n)]
            a["max"] = [max(v[i] for v in values) for i in range(n)]
        accessors.append(a)
        return len(accessors) - 1

    base_p, base_n = frame_data(0)
    targets = []
    for f in range(1, len(frames)):
        p, n = frame_data(f)
        dp = [tuple(a[c] - b[c] for c in range(3)) for a, b in zip(p, base_p)]
        dn = [tuple(a[c] - b[c] for c in range(3)) for a, b in zip(n, base_n)]
        targets.append({"POSITION": acc("f", 5126, "VEC3", dp, minmax=True),
                        "NORMAL": acc("f", 5126, "VEC3", dn)})

    prim = {
        "attributes": {"POSITION": acc("f", 5126, "VEC3", base_p, minmax=True),
                       "NORMAL": acc("f", 5126, "VEC3", base_n),
                       "TEXCOORD_0": acc("f", 5126, "VEC2", uvs)},
        "indices": acc("H", 5123, "SCALAR", idx),
        "material": 0,
    }
    if targets:
        prim["targets"] = targets

    name = os.path.splitext(os.path.basename(md2_path))[0]
    nt = len(targets)
    doc = {
        "asset": {"version": "2.0", "generator": "BLTZNXT md2_to_gltf.py"},
        "scene": 0,
        "scenes": [{"nodes": [0]}],
        "nodes": [{"name": name, "mesh": 0}],
        "meshes": [{"name": name, "primitives": [prim], "weights": [0.0] * nt,
                    "extras": {"targetNames": [frames[f][2] for f in range(1, len(frames))]}}],
        "materials": [{"name": name,
                       "pbrMetallicRoughness": {"metallicFactor": 0.0, "roughnessFactor": 1.0}}],
    }
    if texture:
        if texture.lower().endswith(".png"):
            img = open(texture, "rb").read()
        else:
            img = png(*read_bmp(texture))
        while len(data) % 4:
            data.append(0)
        views.append({"buffer": 0, "byteOffset": len(data), "byteLength": len(img)})
        data.extend(img)
        doc["images"] = [{"name": os.path.basename(texture), "bufferView": len(views) - 1,
                          "mimeType": "image/png"}]
        doc["textures"] = [{"source": 0}]
        doc["materials"][0]["pbrMetallicRoughness"]["baseColorTexture"] = {"index": 0}

    if nt:
        # Bild i: Ziel i auf 1 (Bild 0 = alle 0), linear dazwischen.
        times = [f / 60.0 for f in range(len(frames))]
        weights = []
        for f in range(len(frames)):
            weights += [1.0 if t == f - 1 else 0.0 for t in range(nt)]
        inp = acc("f", 5126, "SCALAR", times)
        accessors[inp]["min"] = [times[0]]
        accessors[inp]["max"] = [times[-1]]
        out = acc("f", 5126, "SCALAR", weights)
        doc["animations"] = [{"name": "frames",
                              "samplers": [{"input": inp, "output": out, "interpolation": "LINEAR"}],
                              "channels": [{"sampler": 0, "target": {"node": 0, "path": "weights"}}]}]

    doc["bufferViews"] = views
    doc["accessors"] = accessors
    doc["buffers"] = [{"byteLength": len(data)}]
    js = json.dumps(doc, separators=(",", ":")).encode()
    js += b" " * (-len(js) % 4)
    bn = bytes(data) + b"\0" * (-len(data) % 4)
    body = struct.pack("<II", len(js), 0x4E4F534A) + js + struct.pack("<II", len(bn), 0x004E4942) + bn
    with open(out_path, "wb") as f:
        f.write(b"glTF" + struct.pack("<II", 2, 12 + len(body)) + body)
    return len(verts), len(tris), len(frames)


if __name__ == "__main__":
    ap = argparse.ArgumentParser(description="MD2 -> glTF (.glb) mit Morph Targets")
    ap.add_argument("md2")
    ap.add_argument("glb")
    ap.add_argument("--texture", help="Grundtextur zum Einbetten (BMP 8/24 Bit oder PNG)")
    a = ap.parse_args()
    nv, nt, nf = convert(a.md2, a.glb, a.texture)
    print("%s: %d Vertices, %d Dreiecke, %d Bilder" % (a.glb, nv, nt, nf))
