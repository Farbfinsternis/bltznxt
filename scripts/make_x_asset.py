#!/usr/bin/env python3
"""Erzeugt die .x-Testdatei fuer tests/assets/.

Wie bei make_3ds_asset.py gilt: die Modelldateien der Blitz3D-Installation
sind fremdes Material und gehoeren nicht ins Repository, und ihre erwarteten
Zahlen muesste man erst ablesen. Diese hier ist so gebaut, dass die Erwartung
feststeht und dass sie genau die Stellen trifft, an denen ein .x-Leser
danebengreifen kann:

    test_frames.x
      * zwei Netze, eines davon in einem Frame mit Verschiebung um +10 in x.
        Damit ist nachweisbar, dass die Frame-Matrizen angewandt werden -
        ohne sie waere das Ergebnis nur 2 breit statt 11.
      * die Matrix steht zeilenweise und wirkt auf Zeilenvektoren.
      * ein Viereck je Netz, also Zerlegung in je zwei Dreiecke.
      * zwei Materialien: eines steht **inline**, das andere wird als
        Verweis `{name}` gefuehrt - die Form, an der ein Parser die
        schliessende Klammer leicht dem falschen Objekt zuordnet.
      * die Vorlage heisst absichtlich `TextureFileName` mit grossem N, wie
        in interior.X der Installation. Das Original vergleicht GUIDs, dem
        ist die Schreibweise egal; ein namensbasierter Leser muss ohne
        Ruecksicht auf Gross- und Kleinschreibung vergleichen.
      * MeshNormals und MeshTextureCoords mit genau passender Anzahl - nur
        dann uebernimmt das Original sie.

    Erwartet: w=11, h=2, d=0, zwei Flaechen, vier Dreiecke.

    test_tri_cw.x / test_tri_ccw.x
      ein Dreieck ohne MeshNormals, von vorn im Uhrzeigersinn bzw.
      andersherum - BUG-126.

    test_frames_bin.x
      dieselbe Szene in der Binaerkodierung. Sie muss Zahl fuer Zahl
      dasselbe ergeben - siehe den Abschnitt weiter unten.

Aufruf aus dem Projektwurzelverzeichnis:

    python scripts/make_x_asset.py
"""

import os

HEADER = """xof 0302txt 0032

template Header {
 <3D82AB43-62DA-11cf-AB39-0020AF71E433>
 WORD major;
 WORD minor;
 DWORD flags;
}

Header {
 1;
 0;
 1;
}

Material rot {
 1.000000;0.000000;0.000000;1.000000;;
 38.400000;
 1.000000;1.000000;1.000000;;
 0.000000;0.000000;0.000000;;
 TextureFileName {
  "gibtsnicht.bmp";
 }
}
"""


def quad(x0, x1, y0, y1):
    """Vier Ecken eines Vierecks in der z=0-Ebene, gegen den Uhrzeigersinn."""
    return [(x0, y0, 0.0), (x1, y0, 0.0), (x1, y1, 0.0), (x0, y1, 0.0)]


def mesh(name, verts, material_inline):
    v = ",\n  ".join("%f;%f;%f;" % p for p in verts)
    n = len(verts)
    # ein Viereck ueber alle vier Ecken
    face = "1;\n  4;0,1,2,3;;\n"
    normals = ",\n  ".join("0.000000;0.000000;1.000000;" for _ in verts)
    uvs = ",\n  ".join("%f;%f;" % (i % 2, i // 2) for i in range(n))
    if material_inline:
        mats = """  MeshMaterialList {
   1;
   1;
   0;;
   Material {
    0.000000;1.000000;0.000000;1.000000;;
    5.000000;
    1.000000;1.000000;1.000000;;
    0.000000;0.000000;0.000000;;
   }
  }
"""
    else:
        mats = """  MeshMaterialList {
   1;
   1;
   0;;
   {rot}
  }
"""
    return """ Mesh %s {
  %d;
  %s;
  %s
%s  MeshNormals {
   %d;
   %s;
   1;
   4;0,1,2,3;;
  }
  MeshTextureCoords {
   %d;
   %s;
  }
 }
""" % (name, n, v, face, mats, n, normals, n, uvs)


# ============================================================
#  Dieselbe Szene noch einmal, binaer kodiert
# ============================================================
#
# Das Binaerformat ist keine andere Sprache, sondern dieselbe mit anderen
# Marken: ein Strom aus 16-Bit-Marken, an denen je nach Marke Daten haengen.
# Deshalb muss test_frames_bin.x Zahl fuer Zahl dasselbe ergeben wie
# test_frames.x - genau das prueft tests/test_3d14_x_binaer.bb.
#
# Absichtlich enthalten, weil dort die Fehler sitzen:
#   * eine Vorlage mit GUID und Typwoertern, die uebersprungen werden muss,
#   * ein Text (TextureFileName) - nach einem Text steht KEINE weitere
#     Laengenangabe, wer dort vier Byte verbraucht, verschluckt die zwei
#     folgenden Marken,
#   * ein Verweis in eigenen Klammern, {rot},
#   * beide Zahlenformen: die einzelne ganze Zahl und die Liste.

import struct

T_NAME, T_STRING, T_INTEGER, T_GUID = 1, 2, 3, 5
T_INTLIST, T_FLOATLIST = 6, 7
T_OBRACE, T_CBRACE, T_COMMA, T_SEMI = 10, 11, 19, 20
T_TEMPLATE, T_WORD, T_DWORD = 31, 40, 41


def _t(n):
    return struct.pack("<H", n)


def b_name(s):
    return _t(T_NAME) + struct.pack("<I", len(s)) + s.encode("ascii")


def b_string(s):
    # ohne abschliessendes Wort - das ist der Punkt
    return _t(T_STRING) + struct.pack("<I", len(s)) + s.encode("ascii") + _t(T_SEMI)


def b_int(v):
    return _t(T_INTEGER) + struct.pack("<i", v) + _t(T_SEMI)


def b_ints(vals):
    return (_t(T_INTLIST) + struct.pack("<I", len(vals))
            + struct.pack("<%di" % len(vals), *vals) + _t(T_SEMI))


def b_floats(vals):
    return (_t(T_FLOATLIST) + struct.pack("<I", len(vals))
            + struct.pack("<%df" % len(vals), *vals) + _t(T_SEMI))


def b_obj(typ, name, body):
    head = b_name(typ) + (b_name(name) if name else b"")
    return head + _t(T_OBRACE) + body + _t(T_CBRACE)


def b_ref(name):
    return _t(T_OBRACE) + b_name(name) + _t(T_CBRACE)


def b_material(r, g, b, a, power, spec, emis):
    return b_floats([r, g, b, a, power] + list(spec) + list(emis))


def b_mesh(name, verts, material_inline):
    n = len(verts)
    body = b_ints([n])
    body += b_floats([c for p in verts for c in p])
    body += b_ints([1, 4, 0, 1, 2, 3])                 # ein Viereck
    if material_inline:
        mats = b_ints([1, 1, 0]) + b_obj(
            "Material", None,
            b_material(0.0, 1.0, 0.0, 1.0, 5.0, (1.0, 1.0, 1.0), (0.0, 0.0, 0.0)))
    else:
        mats = b_ints([1, 1, 0]) + b_ref("rot")
    body += b_obj("MeshMaterialList", None, mats)
    body += b_obj("MeshNormals", None,
                  b_ints([n]) + b_floats([c for _ in verts for c in (0.0, 0.0, 1.0)])
                  + b_ints([1, 4, 0, 1, 2, 3]))
    body += b_obj("MeshTextureCoords", None,
                  b_ints([n]) + b_floats([float(v) for i in range(n)
                                          for v in (i % 2, i // 2)]))
    return b_obj("Mesh", name, body)


def binary_scene():
    out = b"xof 0303bin 0032"

    # Vorlage: muss uebersprungen werden, ohne den Strom zu verlieren
    out += _t(T_TEMPLATE) + b_name("Header") + _t(T_OBRACE)
    out += (_t(T_GUID)
            + struct.pack("<IHH", 0x3D82AB43, 0x62DA, 0x11cf)
            + bytes([0xAB, 0x39, 0x00, 0x20, 0xAF, 0x71, 0xE4, 0x33]))
    out += _t(T_WORD) + b_name("major") + _t(T_SEMI)
    out += _t(T_WORD) + b_name("minor") + _t(T_SEMI)
    out += _t(T_DWORD) + b_name("flags") + _t(T_SEMI)
    out += _t(T_CBRACE)

    out += b_obj("Header", None, b_int(1) + b_int(0) + b_int(1))

    out += b_obj("Material", "rot",
                 b_material(1.0, 0.0, 0.0, 1.0, 38.4, (1.0, 1.0, 1.0), (0.0, 0.0, 0.0))
                 + b_obj("TextureFileName", None, b_string("gibtsnicht.bmp")))

    ident = [1.0, 0.0, 0.0, 0.0,
             0.0, 1.0, 0.0, 0.0,
             0.0, 0.0, 1.0, 0.0,
             0.0, 0.0, 0.0, 1.0]
    versetzt = list(ident)
    versetzt[12] = 10.0

    innen = (b_obj("FrameTransformMatrix", None, b_floats(versetzt))
             + b_mesh("netz2", quad(0.0, 1.0, 0.0, 1.0), material_inline=False))
    wurzel = (b_obj("FrameTransformMatrix", None, b_floats(ident))
              + b_mesh("netz1", quad(0.0, 2.0, 0.0, 2.0), material_inline=True)
              + b_obj("Frame", "versetzt", innen))
    out += b_obj("Frame", "wurzel", wurzel)
    return out


def main():
    out = os.path.join("tests", "assets")
    os.makedirs(out, exist_ok=True)

    # Netz 1: Viereck 0..2, im Wurzelframe
    m1 = mesh("netz1", quad(0.0, 2.0, 0.0, 2.0), material_inline=True)
    # Netz 2: Viereck 0..1, in einem Frame um +10 in x verschoben
    m2 = mesh("netz2", quad(0.0, 1.0, 0.0, 1.0), material_inline=False)

    body = HEADER + """
Frame wurzel {
 FrameTransformMatrix {
  1.000000,0.000000,0.000000,0.000000,
  0.000000,1.000000,0.000000,0.000000,
  0.000000,0.000000,1.000000,0.000000,
  0.000000,0.000000,0.000000,1.000000;;
 }
%s
 Frame versetzt {
  FrameTransformMatrix {
   1.000000,0.000000,0.000000,0.000000,
   0.000000,1.000000,0.000000,0.000000,
   0.000000,0.000000,1.000000,0.000000,
   10.000000,0.000000,0.000000,1.000000;;
  }
%s
 }
}
""" % (m1, m2)

    path = os.path.join(out, "test_frames.x")
    with open(path, "w", newline="\n") as f:
        f.write(body)
    print("%s: %d Bytes" % (path, len(body)))
    print("  erwartet: w=11 h=2 d=0, zwei Flaechen, vier Dreiecke")

    blob = binary_scene()
    path = os.path.join(out, "test_frames_bin.x")
    with open(path, "wb") as f:
        f.write(blob)
    print("%s: %d Bytes" % (path, len(blob)))
    print("  erwartet: dieselben Zahlen wie test_frames.x")

    # BUG-126: ein Dreieck in beiden Reihenfolgen, ohne MeshNormals, damit
    # der Loader UpdateNormals rufen muss. Von vorn (Kamera bei -z) laeuft
    # test_tri_cw.x im Uhrzeigersinn und ist im Original sichtbar.
    ecken = [(-1.0, 1.0, 0.0), (1.0, 1.0, 0.0), (1.0, -1.0, 0.0)]
    for name, face in (("test_tri_cw.x", (0, 1, 2)),
                       ("test_tri_ccw.x", (0, 2, 1))):
        body = "xof 0302txt 0032\nMesh dreieck {\n 3;\n %s;\n 1;\n 3;%d,%d,%d;;\n}\n" % (
            ",\n ".join("%f;%f;%f;" % p for p in ecken), face[0], face[1], face[2])
        path = os.path.join(out, name)
        with open(path, "w", newline="\n") as f:
            f.write(body)
        print("%s: %d Bytes" % (path, len(body)))


if __name__ == "__main__":
    main()
